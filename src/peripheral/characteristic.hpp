#pragma once

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

using ReadCallback = ssize_t (*)(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
using WriteCallback = ssize_t (*)(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
// This declaration was taken from bt_gatt_ccc_managed_user_data type
using CCCCallback = void (*)(const struct bt_gatt_attr *attr, uint16_t value);

// Characteristic properties
enum CharProperty : uint8_t {
  READ = BT_GATT_CHRC_READ,
  WRITE = BT_GATT_CHRC_WRITE,
  WRITE_WITHOUT_RESP = BT_GATT_CHRC_WRITE_WITHOUT_RESP,
  NOTIFY = BT_GATT_CHRC_NOTIFY,
  INDICATE = BT_GATT_CHRC_INDICATE,
  BROADCAST = BT_GATT_CHRC_BROADCAST,
  AUTH = BT_GATT_CHRC_AUTH,
  EXT_PROP = BT_GATT_CHRC_EXT_PROP
};

// Characteristic permissions
enum CharPermission : uint16_t {
  PERM_READ = BT_GATT_PERM_READ,
  PERM_WRITE = BT_GATT_PERM_WRITE,
  PERM_READ_ENCRYPT = BT_GATT_PERM_READ_ENCRYPT,
  PERM_WRITE_ENCRYPT = BT_GATT_PERM_WRITE_ENCRYPT,
  PERM_READ_AUTHEN = BT_GATT_PERM_READ_AUTHEN,
  PERM_WRITE_AUTHEN = BT_GATT_PERM_WRITE_AUTHEN,
};

class Characteristic {
public:
  Characteristic() = default;
  ~Characteristic() = default;
  void init(const bt_uuid *uuid, uint8_t properties, uint16_t permissions,
            const char *name = "");

  ReadCallback _readCallback = nullptr;
  WriteCallback _writeCallback = nullptr;
  CCCCallback _cccCallback = nullptr;

  uint16_t getPermissions() const { return _permissions; }
  uint8_t getProperties() const { return _properties; }
  const bt_uuid *getUuid() const { return _uuid; }

  // int notify(struct bt_conn *conn, const void *data, uint16_t len);
  // int indicate(struct bt_conn *conn, const void *data, uint16_t len);

  static ssize_t _readDispatcher(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset);
  static ssize_t _writeDispatcher(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  const void *buf, uint16_t len,
                                  uint16_t offset, uint8_t flags);
  static void _cccDispatcher(const struct bt_gatt_attr *attr, uint16_t value);

private:
  const bt_uuid *_uuid = nullptr;
  uint8_t _properties = 0;
  char _name[32];
  uint16_t _permissions = 0;
  void *_userData = nullptr;
  bool _notifications_enabled = false;
  bool _indications_enabled = false;
};