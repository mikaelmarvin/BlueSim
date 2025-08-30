#include "BtManager.hpp"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BT_MANAGER, LOG_LEVEL_INF);

// Static instance pointer (only one BtManager active)
BtManager *BtManager::instance_ = nullptr;

BtManager::BtManager() { instance_ = this; }

BtManager::~BtManager() { instance_ = nullptr; }

bool BtManager::init() {
  int err = bt_enable(nullptr);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return false;
  }

  LOG_INF("Bluetooth initialized");

  // Register connection callbacks
  static struct bt_conn_cb conn_callbacks = {
      .connected = general_connected_cb,
      .disconnected = general_disconnected_cb,
      .security_changed = general_security_changed_cb,
  };
  bt_conn_cb_register(&conn_callbacks);

  return true;
}

// Static callback wrappers
void BtManager::general_connected_cb(struct bt_conn *conn, uint8_t err) {
  if (!instance_)
    return;

  struct bt_conn_info info;
  int get_info_err = bt_conn_get_info(conn, &info);
  if (get_info_err) {
    LOG_ERR("Failed to get connection info (err %d)", get_info_err);
    return;
  }

  // Call the appropriate role callback
  if (info.role == BT_CONN_ROLE_CENTRAL) {
    if (auto central = dynamic_cast<CentralManager *>(instance_)) {
      central->on_connected(conn, err);
    }
  } else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    if (auto peripheral = dynamic_cast<PeripheralManager *>(instance_)) {
      peripheral->on_connected(conn, err);
    }
  }
}

void BtManager::general_disconnected_cb(struct bt_conn *conn, uint8_t reason) {
  if (!instance_)
    return;

  struct bt_conn_info info;
  int get_info_err = bt_conn_get_info(conn, &info);
  if (get_info_err) {
    LOG_ERR("Failed to get connection info (err %d)", get_info_err);
    return;
  }

  if (info.role == BT_CONN_ROLE_CENTRAL) {
    if (auto central = dynamic_cast<CentralManager *>(instance_)) {
      central->on_disconnected(conn, reason);
    }
  } else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    if (auto peripheral = dynamic_cast<PeripheralManager *>(instance_)) {
      peripheral->on_disconnected(conn, reason);
    }
  }
}

void BtManager::general_security_changed_cb(struct bt_conn *conn,
                                            bt_security_t level,
                                            enum bt_security_err err) {
  if (!instance_)
    return;

  struct bt_conn_info info;
  int get_info_err = bt_conn_get_info(conn, &info);
  if (get_info_err) {
    LOG_ERR("Failed to get connection info (err %d)", get_info_err);
    return;
  }

  if (info.role == BT_CONN_ROLE_CENTRAL) {
    if (auto central = dynamic_cast<CentralManager *>(instance_)) {
      central->on_security_changed(conn, level, err);
    }
  } else if (info.role == BT_CONN_ROLE_PERIPHERAL) {
    if (auto peripheral = dynamic_cast<PeripheralManager *>(instance_)) {
      peripheral->on_security_changed(conn, level, err);
    }
  }
}

// ---- Utility ----
std::string BtManager::addrToString(const bt_addr_le_t *addr) {
  char buf[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(addr, buf, sizeof(buf));
  return std::string(buf);
}
