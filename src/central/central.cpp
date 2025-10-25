#include "central.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CENTRAL, LOG_LEVEL_DBG);

// Static registry for Central instances
Central *Central::registry[MAX_CENTRALS] = {nullptr};

Central::Central() : _index(0), _connectionCount(0), _scanner(this) {
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
        return 0; // Already connected
      }
    }
  }

  // Check if we have available connection slots
  if (_connectionCount >= MAX_CENTRAL_CONNECTIONS) {
    LOG_WRN("Central %d: No available connection slots (%d/%d)", 
            _index, _connectionCount, MAX_CENTRAL_CONNECTIONS);
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

  bt_conn_ref(conn);
  addConnection(conn);

  LOG_INF("Central %d initiating connection to device", _index);
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
      _connections[i] = nullptr;
      _connectionCount--;
      return;
    }
  }
  LOG_WRN("No connection found for central %d", _index);
}

void Central::onConnected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Central %d connection failed (err %d)", _index, err);
    // Force restart scanning on connection failure
    int scan_err = bt_le_scan_start(&Scanner::scanParameters, &Scanner::scanCallback);
    if (scan_err < 0) {
      LOG_ERR("Central %d: Failed to restart Zephyr scanning (err %d)", _index, scan_err);
    } else {
      LOG_INF("Central %d: Force restarted Zephyr scanning after connection failure", _index);
      _scanner._isScanning = true;
    }
    return;
  }
  
  LOG_INF("Central %d connected! conn=%p, total connections: %d", _index, conn, _connectionCount);
  
  // Stop scanning on successful connection
  onConnectionSuccess();
}

void Central::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_DBG("Central %d disconnected (reason %u)\n", _index, reason);
  removeConnection(conn);
  
  // Force restart scanning when disconnected
  int scan_err = bt_le_scan_start(&Scanner::scanParameters, &Scanner::scanCallback);
  if (scan_err < 0) {
    LOG_ERR("Central %d: Failed to restart Zephyr scanning (err %d)", _index, scan_err);
  } else {
    LOG_INF("Central %d: Force restarted Zephyr scanning after disconnection", _index);
    _scanner._isScanning = true;
  }
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
