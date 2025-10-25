#include "peripheral.hpp"
#include "service.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PERIPHERAL, LOG_LEVEL_DBG);

Peripheral *Peripheral::registry[MAX_PERIPHERALS] = {nullptr};

Peripheral::Peripheral()
    : _index(0), _serviceCount(0), _connectionCount(0),
      _advertisement(nullptr) {
  memset(_services, 0, sizeof(_services));

  for (uint8_t i = 0; i < MAX_PERIPHERAL_CONNECTIONS; i++) {
    _connections[i] = nullptr;
  }

  for (uint8_t i = 0; i < MAX_PERIPHERALS; i++) {
    if (!registry[i]) {
      _index = i;
      registry[_index] = this;
      return;
    }
  }

  LOG_ERR("Peripheral registry is full! Maximum %d peripherals allowed.",
          MAX_PERIPHERALS);
  __ASSERT(false, "Failed to create a Peripheral - registry full");
}

Peripheral::~Peripheral() {
  Peripheral::registry[_index] = nullptr;
  for (uint8_t i = 0; i < MAX_PERIPHERAL_CONNECTIONS; i++) {
    if (_connections[i]) {
      bt_conn_unref(_connections[i]);
      _connections[i] = nullptr;
    }
  }
}

void Peripheral::addAdvertisement(Advertisement *advertisement) {
  if (_advertisement) {
    LOG_ERR("Advertisement already added");
    return;
  }
  _advertisement = advertisement;
}

void Peripheral::addService(Service *service) {
  service->_peripheral = this;
  _services[_serviceCount++] = service;
}

void Peripheral::registerServices() {
  LOG_INF("Registering %d services for peripheral %d", _serviceCount, _index);

  for (int i = 0; i < _serviceCount; ++i) {
    if (_services[i] == nullptr) {
      LOG_ERR("Service %d is null, skipping", i);
      continue;
    }

    if (_services[i]->_gattService.attrs == nullptr ||
        _services[i]->_gattService.attr_count == 0) {
      LOG_ERR("Service '%s' not properly built (attrs=%p, count=%d)",
              _services[i]->_name, _services[i]->_gattService.attrs,
              _services[i]->_gattService.attr_count);
      continue;
    }

    LOG_INF("Registering service '%s' with %d attributes", _services[i]->_name,
            _services[i]->_gattService.attr_count);

    int err = bt_gatt_service_register(&_services[i]->_gattService);
    if (err < 0) {
      LOG_ERR("Failed to register service '%s' (err %d)", _services[i]->_name,
              err);
      // Continue with other services instead of breaking
    } else {
      LOG_INF("Service '%s' registered successfully", _services[i]->_name);
    }
  }
}

void Peripheral::addConnection(struct bt_conn *conn) {
  for (uint8_t i = 0; i < MAX_PERIPHERAL_CONNECTIONS; i++) {
    if (_connections[i] == nullptr) {
      _connections[i] = conn;
      _connectionCount++;
      return;
    }
  }

  LOG_WRN("No available connection slot found for peripheral %d", _index);
}

void Peripheral::removeConnection(struct bt_conn *conn) {
  for (uint8_t i = 0; i < MAX_PERIPHERAL_CONNECTIONS; i++) {
    if (_connections[i] == conn) {
      _connections[i] = nullptr;
      _connectionCount--;
      return;
    }
  }

  LOG_WRN("No connection found for peripheral %d", _index);
}

void Peripheral::onConnected(struct bt_conn *conn, uint8_t err) {
  LOG_DBG("Peripheral %d connected! conn=%p\n", _index, conn);
  addConnection(conn);

  if (_connectionCount >= MAX_PERIPHERAL_CONNECTIONS) {
    _advertisement->stopAdvertising();
    LOG_INF("Peripheral %d reached maximum number of connections %d ", _index,
            MAX_PERIPHERAL_CONNECTIONS);
    return;
  }

  // Restart advertising
  _advertisement->startAdvertising();
}

void Peripheral::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_DBG("Peripheral %d disconnected (reason %u)\n", _index, reason);
  removeConnection(conn);

  // Restart advertising
  _advertisement->startAdvertising();
}

Peripheral *Peripheral::fromConn(struct bt_conn *conn) {
  for (Peripheral *peripheral : Peripheral::registry) {
    if (!peripheral)
      continue;
    for (uint8_t i = 0; i < MAX_PERIPHERAL_CONNECTIONS; i++) {
      if (peripheral->_connections[i] == conn) {
        return peripheral;
      }
    }
  }

  LOG_WRN("Failed to find Peripheral for connection %p", conn);
  return nullptr;
}
