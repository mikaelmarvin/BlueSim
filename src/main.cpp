#include "peripheral/peripheral.hpp"
#include "peripheral/service.hpp"
#include <cstring>
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
  Peripheral p1;
  Peripheral p2;

  // Create custom services at runtime using static allocation
  static GattService service1;
  static GattService uartService;
  static GattService service2;

  // Initialize service 1
  bt_uuid_128 ble_edrive_param_char = BT_UUID_INIT_128(
      BT_UUID_128_ENCODE(0x6e2f84f2, 0x6f5a, 0x48c4, 0x9873, 0xc77acce33964));
      
  service1.init(&BtUuids::CUSTOM_SERVICE_1.uuid, "Custom Service 1");

  // Add characteristics to service 1
  static Characteristic readChar1, writeChar1, notifyChar1;
  readChar1.init(&BtUuids::CUSTOM_CHAR_1.uuid, CharProperty::READ,
                 "Read-only characteristic");
  readChar1.setReadCallback(
      [](struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
         uint16_t len, uint16_t offset, uint8_t flags) -> ssize_t {
        const char *data = "Hello from read-only char!";
        memcpy(buf, data, std::min(len, (uint16_t)strlen(data)));
        return strlen(data);
      });
  service1.addCharacteristic(&readChar1);

  writeChar1.init(&BtUuids::CUSTOM_CHAR_2.uuid, CharProperty::WRITE,
                  "Write-only characteristic");
  writeChar1.setWriteCallback(
      [](struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
         uint16_t len, uint16_t offset, uint8_t flags) -> ssize_t {
        LOG_INF("Received %d bytes: %.*s", len, len, (const char *)buf);
        return len;
      });
  service1.addCharacteristic(&writeChar1);

  notifyChar1.init(&BtUuids::CUSTOM_CHAR_3.uuid, CharProperty::NOTIFY,
                   "Notify characteristic");
  notifyChar1.setCCCCallback(
      [](const struct bt_gatt_attr *attr, uint16_t value) {
        LOG_INF("Notifications %s",
                value == BT_GATT_CCC_NOTIFY ? "enabled" : "disabled");
      });
  service1.addCharacteristic(&notifyChar1);

  // Initialize Nordic UART service
  uartService.init(&BtUuids::NUS_SERVICE.uuid, "Nordic UART Service");

  static Characteristic txChar, rxChar;
  txChar.init(&BtUuids::NUS_TX_CHAR.uuid, CharProperty::NOTIFY,
              "TX Characteristic");
  txChar.setCCCCallback([](const struct bt_gatt_attr *attr, uint16_t value) {
    LOG_INF("UART TX notifications %s",
            value == BT_GATT_CCC_NOTIFY ? "enabled" : "disabled");
  });
  uartService.addCharacteristic(&txChar);

  rxChar.init(&BtUuids::NUS_RX_CHAR.uuid, CharProperty::WRITE,
              "RX Characteristic");
  rxChar.setWriteCallback(
      [](struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
         uint16_t len, uint16_t offset, uint8_t flags) -> ssize_t {
        LOG_INF("UART RX received %d bytes: %.*s", len, len, (const char *)buf);
        return len;
      });
  uartService.addCharacteristic(&rxChar);

  // Initialize service 2 for p2
  service2.init(&BtUuids::CUSTOM_SERVICE_2.uuid, "Custom Service 2");

  static Characteristic readWriteChar;
  readWriteChar.init(
      &BtUuids::CUSTOM_CHAR_1.uuid,
      static_cast<CharProperty>(CharProperty::READ | CharProperty::WRITE),
      "Read/Write characteristic");
  readWriteChar.setReadCallback(
      [](struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
         uint16_t len, uint16_t offset, uint8_t flags) -> ssize_t {
        const char *data = "Data from p2!";
        memcpy(buf, data, std::min(len, (uint16_t)strlen(data)));
        return strlen(data);
      });
  readWriteChar.setWriteCallback(
      [](struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
         uint16_t len, uint16_t offset, uint8_t flags) -> ssize_t {
        LOG_INF("P2 received %d bytes: %.*s", len, len, (const char *)buf);
        return len;
      });
  service2.addCharacteristic(&readWriteChar);

  // Add services to peripherals
  p1.addService(&service1);
  p1.addService(&uartService);
  p2.addService(&service2);

  p1.start();
  p2.start();

  // Main loop (Zephyr threads will handle advertising & connections)
  while (true) {
    k_sleep(K_SECONDS(1));
  }

  return 0;
}
