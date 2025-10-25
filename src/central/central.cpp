#include "central.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CENTRAL, LOG_LEVEL_DBG);

// Static registry for Central instances
Central *Central::registry[MAX_CENTRALS] = {nullptr};

Central::Central()
    : _index(0), _connectionCount(0), _scanner(this),
      _shouldStartScanning(false) {
  // Initialize the work item
  k_work_init_delayable(&_scanWork, scanWorkAction);
  for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
    _connections[i] = nullptr;
  }

  for (uint8_t i = 0; i < MAX_CENTRALS; i++) {
    if (!registry[i]) {
      _index = i;
      registry[_index] = this;
      return;
    }
  }

  LOG_ERR("Central registry is full! Maximum %d centrals allowed.",
          MAX_CENTRALS);
  __ASSERT(false, "Failed to create a Central - registry full");
}

Central::~Central() {
  Central::registry[_index] = nullptr;
  for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
    if (_connections[i]) {
      bt_conn_unref(_connections[i]);
      _connections[i] = nullptr;
    }
  }
}

int Central::connectToDevice(const bt_addr_le_t *addr) {
  // Check if we already have a connection to this device
  for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
    if (_connections[i]) {
      const bt_addr_le_t *device_addr = bt_conn_get_dst(_connections[i]);
      if (bt_addr_le_cmp(device_addr, addr) == 0) {
        LOG_DBG("Central %d: Already connected to device", _index);
        return 0;
      }
    }
  }

  // Check if we have available connection slots
  if (_connectionCount >= MAX_CENTRAL_CONNECTIONS) {
    LOG_WRN("Central %d: No available connection slots (%d/%d)", _index,
            _connectionCount, MAX_CENTRAL_CONNECTIONS);
    return -ENOMEM;
  }

  struct bt_conn *conn;

  // Create parameters as variables (C++ doesn't allow taking address of
  // temporaries)
  struct bt_conn_le_create_param create_param = {
      .options = BT_CONN_LE_OPT_NONE,
      .interval = BT_GAP_SCAN_FAST_INTERVAL,
      .window = BT_GAP_SCAN_FAST_WINDOW,
      .interval_coded = 0,
      .window_coded = 0,
      .timeout = 0,
  };

  struct bt_le_conn_param conn_param = {
      .interval_min = BT_GAP_INIT_CONN_INT_MIN,
      .interval_max = BT_GAP_INIT_CONN_INT_MAX,
      .latency = 0,
      .timeout = 400,
  };

  int err = bt_conn_le_create(addr, &create_param, &conn_param, &conn);
  if (err < 0) {
    LOG_ERR("Failed to create connection (err %d)", err);
    return err;
  }

  addConnection(conn);

  LOG_INF("Central %d created connection stack connection object", _index);
  return 0;
}

int Central::disconnectFromDevice(const bt_addr_le_t *addr) {
  for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
    // Check if connection exists first
    if (!_connections[i]) {
      continue;
    }

    // Get the device address and compare
    const bt_addr_le_t *device_addr = bt_conn_get_dst(_connections[i]);
    if (bt_addr_le_cmp(device_addr, addr) == 0) {
      int err =
          bt_conn_disconnect(_connections[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
      if (err < 0) {
        LOG_ERR("Central %d: Failed to disconnect (err %d)", _index, err);
        return err;
      }

      LOG_INF("Central %d: Disconnecting from device", _index);
      bt_conn_unref(_connections[i]);
      _connections[i] = nullptr;
      _connectionCount--;
      return 0;
    }
  }

  LOG_WRN("Central %d: Device not found in connections", _index);
  return -1;
}

void Central::addConnection(struct bt_conn *conn) {
  for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
    if (_connections[i] == nullptr) {
      bt_conn_ref(conn);
      _connections[i] = conn;
      _connectionCount++;
      LOG_INF("Central %d: Added connection %p to slot %d (total: %d/%d)",
              _index, conn, i, _connectionCount, MAX_CENTRAL_CONNECTIONS);
      return;
    }
  }
  LOG_WRN("No available connection slot found for central %d", _index);
}

void Central::removeConnection(struct bt_conn *conn) {
  for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
    if (_connections[i] == conn) {
      bt_conn_unref(_connections[i]);
      _connections[i] = nullptr;
      _connectionCount--;
      LOG_INF("Central %d: Removed connection %p from slot %d (total: %d/%d)",
              _index, conn, i, _connectionCount, MAX_CENTRAL_CONNECTIONS);
      return;
    }
  }
  LOG_WRN("No connection found for central %d", _index);
}

void Central::onConnected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Central %d connection failed (err %d)", _index, err);
    removeConnection(conn);
    // Schedule scanning start after connection failure
    scheduleScanningStart();
    return;
  }

  LOG_INF("Central %d connected! conn=%p, total connections: %d", _index, conn,
          _connectionCount);

  // Schedule scanning stop after successful connection
  scheduleScanningStop();
}

void Central::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_DBG("Central %d disconnected (reason %u)\n", _index, reason);
  removeConnection(conn);

  // Schedule scanning start after disconnection
  scheduleScanningStart();
}

Central *Central::fromConn(struct bt_conn *conn) {
  for (Central *central : Central::registry) {
    if (!central)
      continue;

    for (uint8_t i = 0; i < MAX_CENTRAL_CONNECTIONS; i++) {
      if (central->_connections[i] == conn) {
        return central;
      }
    }
  }

  LOG_WRN("Failed to find Central for connection %p", conn);
  return nullptr;
}

void Central::scheduleScanningStart() {
  _shouldStartScanning = true;
  int err = k_work_reschedule(&_scanWork, K_MSEC(200));
  if (err < 0) {
    LOG_ERR("Central %d: Failed to schedule scanning start work item (err %d)",
            _index, err);
  } else {
    LOG_INF("Central %d: Scheduled scanning start work item", _index);
  }
}

void Central::scheduleScanningStop() {
  _shouldStartScanning = false;
  int err = k_work_reschedule(&_scanWork, K_MSEC(200));
  if (err < 0) {
    LOG_ERR("Central %d: Failed to schedule scanning stop work item (err %d)",
            _index, err);
  } else {
    LOG_INF("Central %d: Scheduled scanning stop work item", _index);
  }
}

void Central::scanWorkAction(struct k_work *work) {
  // Recover the Central instance from the k_work pointer
  Central *self = CONTAINER_OF(work, Central, _scanWork);

  if (self->_shouldStartScanning) {
    LOG_INF("Central %d: Executing deferred scanning start", self->_index);
    int err = self->startScanning();
    if (err < 0) {
      LOG_ERR("Central %d: Failed to start scanning (err %d)", self->_index,
              err);
    } else {
      LOG_INF("Central %d: Successfully started scanning", self->_index);
    }
  } else {
    LOG_INF("Central %d: Executing deferred scanning stop", self->_index);
    int err = self->stopScanning();
    if (err < 0) {
      LOG_ERR("Central %d: Failed to stop scanning (err %d)", self->_index,
              err);
    } else {
      LOG_INF("Central %d: Successfully stopped scanning", self->_index);
    }
  }
}
