#include "peripheral/characteristic.hpp"
#include "peripheral/peripheral.hpp"
#include "peripheral/service.hpp"
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

  Peripheral p1;
  // Peripheral p2;

  Service service;
  Characteristic readChar, writeChar;

  bt_uuid_128 service_uuid = BT_UUID_INIT_128(
      BT_UUID_128_ENCODE(0x6e2f84f2, 0x6f5a, 0x48c4, 0x9873, 0xc77acce33964));
  bt_uuid_128 read_uuid = BT_UUID_INIT_128(
      BT_UUID_128_ENCODE(0x03ba1869, 0xeb49, 0x48de, 0x9213, 0x508d2304e80b));
  bt_uuid_128 write_uuid = BT_UUID_INIT_128(
      BT_UUID_128_ENCODE(0x4bb53da6, 0x42bc, 0x430f, 0x95fa, 0xc99a44b38e14));

  service.init(&service_uuid.uuid, "Custom Service 1");
  readChar.init(&read_uuid.uuid, CharProperty::READ | CharProperty::NOTIFY,
                CharPermission::PERM_READ, "Read-notify characteristic");
  writeChar.init(&write_uuid.uuid, CharProperty::WRITE,
                 CharPermission::PERM_WRITE, "Write-only characteristic");

  service.addCharacteristic(&readChar);
  service.addCharacteristic(&writeChar);

  // Add services to peripherals
  p1.addService(&service);
  p1.registerServices();

  p1.start();
  // p2.start();

  // Main loop (Zephyr threads will handle advertising & connections)
  while (true) {
    k_sleep(K_SECONDS(1));
  }

  return 0;
}
