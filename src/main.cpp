#include "central/central.hpp"
#include "peripheral/advertisement.hpp"
#include "peripheral/characteristic.hpp"
#include "peripheral/peripheral.hpp"
#include "peripheral/service.hpp"
#include <zephyr/logging/log.h>

extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

// Global connection callbacks that dispatch to appropriate instances
static void global_bt_conn_cb_connected(struct bt_conn *conn, uint8_t err) {
  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    if (info.role == BT_CONN_ROLE_PERIPHERAL) {
      for (uint8_t i = 0; i < MAX_PERIPHERALS; i++) {
        if (Peripheral::registry[i]) {
          if (Peripheral::registry[i]->_advertisement->_id == info.id) {
            Peripheral::registry[i]->onConnected(conn, err);
            return;
          }
        }

        LOG_WRN("No available peripheral found for connection %d", info.id);
      }
    } else if (info.role == BT_CONN_ROLE_CENTRAL) {
      for (uint8_t i = 0; i < MAX_CENTRALS; i++) {
        if (Central::registry[i]) {
          if (Central::fromConn(conn) == Central::registry[i]) {
            Central::registry[i]->onConnected(conn, err);
            return;
          }
        }
      }

      LOG_WRN("No available central found for connection");
    }
  }
}

static void global_bt_conn_cb_disconnected(struct bt_conn *conn,
                                           uint8_t reason) {
  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    if (info.role == BT_CONN_ROLE_PERIPHERAL) {
      Peripheral *peripheral = Peripheral::fromConn(conn);
      if (peripheral) {
        peripheral->onDisconnected(conn, reason);
      } else {
        LOG_ERR("Disconnected connection not associated with any peripheral");
      }
    } else if (info.role == BT_CONN_ROLE_CENTRAL) {
      Central *central = Central::fromConn(conn);
      if (central) {
        if (Central::fromConn(conn) == central) {
          central->onDisconnected(conn, reason);
          return;
        }
      } else {
        LOG_ERR("Disconnected connection not associated with any central");
      }
    }
  }
}

static struct bt_conn_cb global_conn_cb = {
    .connected = global_bt_conn_cb_connected,
    .disconnected = global_bt_conn_cb_disconnected,
};

int main() {
  int err = bt_enable(nullptr);
  if (err < 0) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return err;
  }

  // Register global connection callbacks
  bt_conn_cb_register(&global_conn_cb);

  LOG_INF("Bluetooth initialized");

  // Wait for BLE stack to be fully ready
  k_sleep(K_MSEC(100));

  // Peripheral p1;
  Central c1;

  Filter filter;
  filter.addGroup();
  filter.addCriterion(FilterCriterionType::LOCAL_NAME, "Mikael1");
  filter.addGroup();
  filter.addCriterion(FilterCriterionType::LOCAL_NAME, "Mikael2");
  c1.addFilter(filter);

  c1.scheduleScanningStart();

  // // Initialize advertisement
  // Advertisement advertisement;
  // int init_result = advertisement.init("Simulator");
  // if (init_result) {
  //   LOG_ERR("Failed to initialize advertisement (err %d)", init_result);
  //   return init_result;
  // }

  // Service service;
  // Characteristic readChar, writeChar;

  // bt_uuid_128 service_uuid = BT_UUID_INIT_128(
  //     BT_UUID_128_ENCODE(0x6e2f84f2, 0x6f5a, 0x48c4, 0x9873,
  //     0xc77acce33964));
  // bt_uuid_128 read_uuid = BT_UUID_INIT_128(
  //     BT_UUID_128_ENCODE(0x03ba1869, 0xeb49, 0x48de, 0x9213,
  //     0x508d2304e80b));
  // bt_uuid_128 write_uuid = BT_UUID_INIT_128(
  //     BT_UUID_128_ENCODE(0x4bb53da6, 0x42bc, 0x430f, 0x95fa,
  //     0xc99a44b38e14));

  // service.init(&service_uuid.uuid, "Custom Service 1");
  // readChar.init(&read_uuid.uuid, CharProperty::READ | CharProperty::NOTIFY,
  //               CharPermission::PERM_READ, "Read-notify characteristic");
  // writeChar.init(&write_uuid.uuid, CharProperty::WRITE,
  //                CharPermission::PERM_WRITE, "Write-only characteristic");

  // int result = service.addCharacteristic(&readChar);
  // if (result != 1) {
  //   LOG_ERR("Failed to add read characteristic (err %d)", result);
  //   return -1;
  // }

  // result = service.addCharacteristic(&writeChar);
  // if (result != 1) {
  //   LOG_ERR("Failed to add write characteristic (err %d)", result);
  //   return -1;
  // }

  // // Build the service before registering
  // result = service.buildService();
  // if (result != 0) {
  //   LOG_ERR("Failed to build service (err %d)", result);
  //   return -1;
  // }

  // // Add services and the advertisement to the peripheral
  // p1.addAdvertisement(&advertisement);
  // p1.addService(&service);
  // p1.registerServices();

  // // Start advertising
  // int advertising_result = advertisement.startAdvertising();
  // if (advertising_result) {
  //   LOG_ERR("Failed to start advertising (err %d)", advertising_result);
  //   return advertising_result;
  // }

  // Main loop (Zephyr threads will handle advertising & connections)
  while (true) {
    k_sleep(K_SECONDS(1));
  }

  return 0;
}
