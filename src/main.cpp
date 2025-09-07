#include "peripheral.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

static struct bt_conn_cb global_conn_cb = {
    .connected = Peripheral::bt_conn_cb_connected,
    .disconnected = Peripheral::bt_conn_cb_disconnected,
};

int main() {
  int err = bt_enable(nullptr);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return err;
  }

  // Register global connection callbacks
  bt_conn_cb_register(&global_conn_cb);

  LOG_INF("Bluetooth initialized");

  // Create your virtual peripherals
  Peripheral p1(0);
  Peripheral p2(1);

  p1.start();
  p2.start();

  // Main loop (Zephyr threads will handle advertising & connections)
  while (true) {
    k_sleep(K_SECONDS(1));
  }

  return 0;
}
