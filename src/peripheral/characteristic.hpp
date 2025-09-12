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
  Characteristic();
  void init(const bt_uuid *uuid, CharProperty properties,
            const char *description = "");

  // Set callbacks
  void setReadCallback(ReadCallback callback);
  void setWriteCallback(WriteCallback callback);
  void setCCCCallback(CCCCallback callback);

  // Set permissions
  void setPermissions(bt_gatt_perm_t permissions);

  // Set user data
  void setUserData(void *data);

  // Build the characteristic attributes
  int buildAttributes(bt_gatt_attr *attrs, bt_gatt_chrc *chrcs,
                      _bt_gatt_ccc *cccs, uint8_t *attr_count,
                      uint8_t *chrc_count, uint8_t *ccc_count);

  // Get characteristic info
  const bt_uuid *getUuid() const { return _uuid; }
  CharProperty getProperties() const { return _properties; }
  const char *getDescription() const { return _description; }
  bool isInitialized() const { return _initialized; }

  // Notification/Indication support
  int notify(struct bt_conn *conn, const void *data, uint16_t len);
  int indicate(struct bt_conn *conn, const void *data, uint16_t len);

private:
  const bt_uuid *_uuid;
  CharProperty _properties;
  char _description[32];
  ReadCallback _readCallback;
  WriteCallback _writeCallback;
  CCCCallback _cccCallback;
  bt_gatt_perm_t _permissions;
  void *_userData;
  bool _initialized;

  // Internal state for attribute building
  uint8_t _valueAttrIndex;
  bool _hasCCC;

  // Static callback wrappers
  static ssize_t _readWrapper(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr, void *buf,
                              uint16_t len, uint16_t offset, uint8_t flags);
  static ssize_t _writeWrapper(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr, const void *buf,
                               uint16_t len, uint16_t offset, uint8_t flags);
  static void _cccWrapper(const struct bt_gatt_attr *attr, uint16_t value);
};