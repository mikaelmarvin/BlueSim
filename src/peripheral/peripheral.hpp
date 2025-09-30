#pragma once

extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

#define MAX_SERVICES_PER_PERIPHERAL 3

class Service;

class Peripheral {
public:
  Peripheral();
  virtual ~Peripheral();

  virtual void onConnected(struct bt_conn *conn);
  virtual void onDisconnected(struct bt_conn *conn, uint8_t reason);

  void addService(Service *service);
  void registerServices();

  // Static functions and variables
  static Peripheral *registry[8]; // Reduced size since we're not using advertising IDs
  static void bt_conn_cb_connected(struct bt_conn *conn, uint8_t err);
  static void bt_conn_cb_disconnected(struct bt_conn *conn, uint8_t reason);

  // Map bt_conn back to Peripheral
  static Peripheral *fromConn(struct bt_conn *conn);

private:
  uint8_t _id;
  struct bt_conn *_conn;
  Service *_services[MAX_SERVICES_PER_PERIPHERAL];
  uint8_t _serviceCount;
};
