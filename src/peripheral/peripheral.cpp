#include "peripheral.hpp"
#include "service.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PERIPHERAL, LOG_LEVEL_DBG);

// Static array - using fixed size since advertising is decoupled
Peripheral *Peripheral::registry[8] = {nullptr};

Peripheral::Peripheral() : _conn(nullptr), _serviceCount(0) {
  memset(_services, 0, sizeof(_services));

  // Find next available slot in registry
  for (uint8_t i = 0; i < 8; i++) {
    if (!registry[i]) {
      _id = i;
      registry[_id] = this;
      break;
    }
  }
}

Peripheral::~Peripheral() { 
  if (_id < 8) {
    Peripheral::registry[_id] = nullptr; 
  }
}

void Peripheral::addService(Service *service) {
  // Add checks later
  service->_peripheral = this;
  _services[_serviceCount++] = service;
}

void Peripheral::registerServices() {
  LOG_INF("Registering %d services for peripheral %d", _serviceCount, _id);

  for (int i = 0; i < _serviceCount; ++i) {
    if (_services[i] == nullptr) {
      LOG_ERR("Service %d is null, skipping", i);
      continue;
    }

    // Validate that the service has been built
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
    if (err) {
      LOG_ERR("Failed to register service '%s' (err %d)", _services[i]->_name,
              err);
      // Continue with other services instead of breaking
    } else {
      LOG_INF("Service '%s' registered successfully", _services[i]->_name);
    }
  }
}

void Peripheral::onConnected(struct bt_conn *conn) {
  LOG_INF("Peripheral %d connected! conn=%p\n", _id, conn);
}

void Peripheral::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_WRN("Peripheral %d disconnected (reason %u)\n", _id, reason);
}

// Utility function to map the peripheral from which the connection is coming
// from
Peripheral *Peripheral::fromConn(struct bt_conn *conn) {
  for (Peripheral *p : Peripheral::registry) {
    if (!p)
      continue;

    if (p->_conn == conn) {
      return p;
    }
  }
  return nullptr;
}

void Peripheral::bt_conn_cb_connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Connection failed (err %d)", err);
    return;
  }

  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    // For peripheral connections, try to find any available peripheral
    // This is a simplified approach - in a real application you might want
    // to match based on advertising data or other criteria
    for (uint8_t i = 0; i < 8; i++) {
      if (registry[i] && !registry[i]->_conn) {
        registry[i]->_conn = bt_conn_ref(conn);
        registry[i]->onConnected(conn);
        return;
      }
    }
    LOG_WRN("No available peripheral found for connection");
  }
}

void Peripheral::bt_conn_cb_disconnected(struct bt_conn *conn, uint8_t reason) {
  Peripheral *p = Peripheral::fromConn(conn);
  if (p) {
    bt_conn_unref(p->_conn);
    p->_conn = nullptr;
    p->onDisconnected(conn, reason);
  } else {
    LOG_WRN("Disconnected connection not associated with any peripheral");
  }
}
