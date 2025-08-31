#include "Peripheral.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PERIPHERAL, LOG_LEVEL_INF);

void MyPeripheral::on_connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Failed to accept connection (err %u)", err);
    return;
  }

  char addr_str[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

  LOG_INF("Peripheral connected: %s", addr_str);

  // Application-specific logic (e.g., notify application layers)
}

void MyPeripheral::on_disconnected(struct bt_conn *conn, uint8_t reason) {
  char addr_str[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

  LOG_INF("Peripheral disconnected: %s (reason %u)", addr_str, reason);

  // Application-specific logic (e.g., restart advertising)
}

void MyPeripheral::on_security_changed(struct bt_conn *conn,
                                       bt_security_t level,
                                       enum bt_security_err err) {
  char addr_str[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

  if (err) {
    LOG_ERR("Peripheral security failed for %s level %u (err %d)", addr_str,
            level, err);
  } else {
    LOG_INF("Peripheral security changed: %s level %u", addr_str, level);
  }

  // Application-specific logic (e.g., grant access to encrypted attributes)
}
