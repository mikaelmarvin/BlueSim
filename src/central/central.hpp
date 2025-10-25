#pragma once

#include "scanner.hpp"

extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
}

#define MAX_CENTRALS                                                           \
  (CONFIG_BT_MAX_CONN - CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT) / 2
#define MAX_CENTRAL_CONNECTIONS CONFIG_BT_MAX_CONN / 2

class Central {
public:
  Central();
  virtual ~Central();

  int startScanning() { return _scanner.startScanning(); }
  int stopScanning() { return _scanner.stopScanning(); }
  void onConnectionSuccess() { _scanner.stopScanning(); }
  int connectToDevice(const bt_addr_le_t *addr);
  int disconnectFromDevice(const bt_addr_le_t *addr);
  void addConnection(struct bt_conn *conn);
  void removeConnection(struct bt_conn *conn);
  void addFilter(Filter &filter) { _scanner.addFilter(filter); }
  bool isScanning() const { return _scanner._isScanning; }

  virtual void onConnected(struct bt_conn *conn, uint8_t err);
  virtual void onDisconnected(struct bt_conn *conn, uint8_t reason);

  static Central *registry[MAX_CENTRALS];
  static Central *fromConn(struct bt_conn *conn);

  uint8_t _index;
  uint8_t _connectionCount;
  struct bt_conn *_connections[MAX_CENTRAL_CONNECTIONS];

private:
  Scanner _scanner;
};