#pragma once

#include "advertisement.hpp"

extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

#define MAX_PERIPHERALS (CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT / 2)
#define MAX_PERIPHERAL_CONNECTIONS ((CONFIG_BT_MAX_CONN / 2) / MAX_PERIPHERALS)
#define MAX_SERVICES_PER_PERIPHERAL 3

class Service;
class Advertisement;

class Peripheral {
public:
  Peripheral();
  virtual ~Peripheral();

  virtual void onConnected(struct bt_conn *conn, uint8_t err);
  virtual void onDisconnected(struct bt_conn *conn, uint8_t reason);

  void addAdvertisement(Advertisement *advertisement);
  void addService(Service *service);
  void registerServices();
  void addConnection(struct bt_conn *conn);
  void removeConnection(struct bt_conn *conn);

  static Peripheral *registry[MAX_PERIPHERALS];

  // Map bt_conn back to Peripheral
  static Peripheral *fromConn(struct bt_conn *conn);

  uint8_t _index;
  uint8_t _serviceCount;
  uint8_t _connectionCount;
  struct bt_conn *_connections[MAX_PERIPHERAL_CONNECTIONS];
  Service *_services[MAX_SERVICES_PER_PERIPHERAL];
  Advertisement *_advertisement;
};
