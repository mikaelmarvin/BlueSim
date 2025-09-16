#pragma once

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

// Callback function pointer types
typedef ssize_t (*ReadCallback)(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr, void *buf,
                                uint16_t len, uint16_t offset, uint8_t flags);
typedef ssize_t (*WriteCallback)(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr,
                                 const void *buf, uint16_t len, uint16_t offset,
                                 uint8_t flags);
typedef void (*CCCCallback)(const struct bt_gatt_attr *attr, uint16_t value);

// Characteristic properties
enum class CharProperty : uint8_t {
  READ = BT_GATT_CHRC_READ,
  WRITE = BT_GATT_CHRC_WRITE,
  WRITE_WITHOUT_RESP = BT_GATT_CHRC_WRITE_WITHOUT_RESP,
  NOTIFY = BT_GATT_CHRC_NOTIFY,
  INDICATE = BT_GATT_CHRC_INDICATE,
  BROADCAST = BT_GATT_CHRC_BROADCAST,
  AUTH = BT_GATT_CHRC_AUTH,
  EXT_PROP = BT_GATT_CHRC_EXT_PROP
};

class Characteristic {
public:
  Characteristic() = default;
  ~Characteristic() = default;
  void init(const bt_uuid *uuid, CharProperty properties,
            const char *name = "");

  void setReadCallback(ReadCallback callback);
  void setWriteCallback(WriteCallback callback);
  void setCCCCallback(CCCCallback callback);
  void setPermissions(bt_gatt_perm_t permissions);
  void setUserData(void *data);

  const bt_uuid *getUuid() const { return _uuid; }
  CharProperty getProperties() const { return _properties; }
  const char *getName() const { return _name; }
  uint16_t getPermissions() const { return _permissions; }

  int notify(struct bt_conn *conn, const void *data, uint16_t len);
  int indicate(struct bt_conn *conn, const void *data, uint16_t len);

private:
  const bt_uuid *_uuid = nullptr;
  CharProperty _properties = 0;
  char _name[32];
  ReadCallback _readCallback = nullptr;
  WriteCallback _writeCallback = nullptr;
  CCCCallback _cccCallback = nullptr;
  uint16_t _permissions = 0;
  void *_userData = nullptr;

  static ssize_t _readWrapper(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr, void *buf,
                              uint16_t len, uint16_t offset, uint8_t flags);
  static ssize_t _writeWrapper(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr, const void *buf,
                               uint16_t len, uint16_t offset, uint8_t flags);
  static void _cccWrapper(const struct bt_gatt_attr *attr, uint16_t value);
};