#pragma once

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include <iostream>
#include <string>

class BtManager {
public:
  BtManager();
  virtual ~BtManager();

  bool init();

protected:
  static std::string addrToString(const bt_addr_le_t *addr);

private:
  static void general_connected_cb(struct bt_conn *conn, uint8_t err);
  static void general_disconnected_cb(struct bt_conn *conn, uint8_t reason);
  static void general_security_changed_cb(struct bt_conn *conn,
                                          bt_security_t level,
                                          enum bt_security_err err);

  static BtManager *instance_;
};

// For central devices
class CentralManager : public BtManager {
protected:
  virtual void on_connected(struct bt_conn *conn, uint8_t err) = 0;
  virtual void on_disconnected(struct bt_conn *conn, uint8_t reason) = 0;
  virtual void on_security_changed(struct bt_conn *conn, bt_security_t level,
                                   enum bt_security_err err) = 0;
};

// For peripheral devices
class PeripheralManager : public BtManager {
protected:
  virtual void on_connected(struct bt_conn *conn, uint8_t err) = 0;
  virtual void on_disconnected(struct bt_conn *conn, uint8_t reason) = 0;
  virtual void on_security_changed(struct bt_conn *conn, bt_security_t level,
                                   enum bt_security_err err) = 0;
};
