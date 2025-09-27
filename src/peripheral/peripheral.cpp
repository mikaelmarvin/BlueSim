#include "peripheral.hpp"
#include "service.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PERIPHERAL, LOG_LEVEL_DBG);

// Static array
Peripheral *Peripheral::registry[CONFIG_BT_EXT_ADV_MAX_ADV_SET] = {nullptr};

Peripheral::Peripheral() : _conn(nullptr), _serviceCount(0) {
  memset(_services, 0, sizeof(_services));
  struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
      BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_2,
      BT_GAP_ADV_FAST_INT_MAX_2, nullptr);

  _id = bt_id_create(NULL, NULL);
  adv_param.id = _id;
  Peripheral::registry[_id] = this;
  k_work_init(&_advert.work, Peripheral::workHandler);

  int err = bt_le_ext_adv_create(&adv_param, NULL, &_advert.adv);
  if (err) {
    LOG_ERR("Failed to create adv for id %d (err %d)\n", _id, err);
  } else {
    LOG_INF("Peripheral %d created adv %p\n", _id, _advert.adv);
  }

  static const uint8_t device_name[8 + 1] = "BlueSimI";
  // device_name[7] = '0' + id;
  _advert.adv_data[0] =
      BT_DATA(BT_DATA_NAME_COMPLETE, device_name, sizeof(device_name) - 1);

  err = bt_le_ext_adv_set_data(_advert.adv, _advert.adv_data, 1, nullptr, 0);
  if (err) {
    LOG_ERR("Peripheral %d failed to set adv data (err %d)\n", _id, err);
  }
}

Peripheral::~Peripheral() { Peripheral::registry[_id] = nullptr; }

int Peripheral::start() {
  int err = k_work_submit(&_advert.work);

  if (!err) {
    LOG_ERR("Peripheral %d failed to submit k work item (err %d)\n", _id, err);
  }

  return err;
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
    if (_services[i]->_gattService.attrs == nullptr || _services[i]->_gattService.attr_count == 0) {
      LOG_ERR("Service '%s' not properly built (attrs=%p, count=%d)", 
              _services[i]->_name, 
              _services[i]->_gattService.attrs, 
              _services[i]->_gattService.attr_count);
      continue;
    }

    LOG_INF("Registering service '%s' with %d attributes", 
            _services[i]->_name, 
            _services[i]->_gattService.attr_count);

    int err = bt_gatt_service_register(&_services[i]->_gattService);
    if (err) {
      LOG_ERR("Failed to register service '%s' (err %d)", _services[i]->_name, err);
      // Continue with other services instead of breaking
    } else {
      LOG_INF("Service '%s' registered successfully", _services[i]->_name);
    }
  }
}

void Peripheral::submitAdvertiser() {

  if (_advert.adv) {
    int err = bt_le_ext_adv_start(_advert.adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err) {
      LOG_ERR("Peripheral %d failed to start advertising set (err %d)\n", _id,
              err);
      return;
    }
  } else {
    LOG_ERR("Advertiser for peripheral %d not setup correctly\n", _id);
    return;
  }

  LOG_INF("Advertiser %d successfully started\n", _id);
}

void Peripheral::onConnected(struct bt_conn *conn) {
  LOG_INF("Peripheral %d connected! conn=%p\n", _id, conn);
}

void Peripheral::onDisconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_WRN("Peripheral %d disconnected (reason %u)\n", _id, reason);
}

void Peripheral::workHandler(struct k_work *work) {
  // Recover the Peripheral instance from the k_work pointer
  Peripheral *self = CONTAINER_OF(work, Peripheral, _advert.work);

  // Call the instance method to perform work
  self->submitAdvertiser();
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
  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    uint8_t id = info.id; // This is the local identity (same as adv_param.id)
    Peripheral *p = Peripheral::registry[id];
    if (p) {
      p->_conn = bt_conn_ref(conn);
      p->onConnected(conn);
      return;
    }
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
