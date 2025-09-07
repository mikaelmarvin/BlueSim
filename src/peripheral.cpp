#include "peripheral.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PERIPHERAL, LOG_LEVEL_DBG);

// Static array
Peripheral *Peripheral::registry[CONFIG_BT_EXT_ADV_MAX_ADV_SET] = {nullptr};

Peripheral::Peripheral(uint8_t id) : _id(id), _adv(nullptr), _conn(nullptr) {
  struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
      BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_2,
      BT_GAP_ADV_FAST_INT_MAX_2, nullptr);

  adv_param.id = id;
  Peripheral::registry[_id] = this;

  int err = bt_le_ext_adv_create(&adv_param, NULL, &_adv);
  if (err) {
    LOG_ERR("Failed to create adv for id %d (err %d)\n", id, err);
  } else {
    LOG_INF("Peripheral %d created adv %p\n", id, _adv);
  }

  static const uint8_t device_name[8 + 1] = "BlueSimI";
  // device_name[7] = '0' + id;
  _adv_data[0] =
      BT_DATA(BT_DATA_NAME_COMPLETE, device_name, sizeof(device_name) - 1);

  err = bt_le_ext_adv_set_data(_adv, _adv_data, 1, nullptr, 0);
  if (err) {
    LOG_ERR("Peripheral %d failed to set adv data (err %d)\n", _id, err);
  }
}

Peripheral::~Peripheral() { Peripheral::registry[_id] = nullptr; }

int Peripheral::start() {
  if (!_adv)
    return -1;

  int err = bt_le_ext_adv_start(_adv, BT_LE_EXT_ADV_START_DEFAULT);
  if (err) {
    LOG_ERR("Peripheral %d failed to start (err %d)\n", _id, err);
  } else {
    LOG_INF("Peripheral %d started advertising\n", _id);
  }
  return err;
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
  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    uint8_t id = info.id; // This is the local identity (same as adv_param.id)
    Peripheral *p = Peripheral::registry[id];
    if (p) {
      p->_conn = conn;
      p->onConnected(conn);
      return;
    }
  }
}

void Peripheral::bt_conn_cb_disconnected(struct bt_conn *conn, uint8_t reason) {
  Peripheral *p = Peripheral::fromConn(conn);
  if (p) {
    p->onDisconnected(conn, reason);
  } else {
    LOG_WRN("Disconnected connection not associated with any peripheral");
  }
}
