#include "characteristic.hpp"
#include "peripheral.hpp"
#include "service.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CHARACTERISTIC, LOG_LEVEL_DBG);

void Characteristic::init(const bt_uuid *uuid, uint8_t properties,
                          uint16_t permissions, const char *name) {
  _uuid = uuid;
  _properties = properties;
  _permissions = permissions;
  memcpy(_name, name, sizeof(_name));
  _name[sizeof(_name) - 1] = '\0';
}

// Definitions of static functions
ssize_t Characteristic::_readDispatcher(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr,
                                        void *buf, uint16_t len,
                                        uint16_t offset) {
  if (!attr || !attr->user_data) {
    LOG_ERR("The READ dispatcher got an unexpected nullptr");
    return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
  }

  Characteristic *self = static_cast<Characteristic *>(attr->user_data);

  if (self->_readCallback) {
    return self->_readCallback(conn, attr, buf, len, offset);
  } else {
    LOG_INF("This is the default READ callback - returning empty data");
    // Return 0 to indicate no data to read, but don't write to buf
    return 0;
  }
}

ssize_t Characteristic::_writeDispatcher(struct bt_conn *conn,
                                         const struct bt_gatt_attr *attr,
                                         const void *buf, uint16_t len,
                                         uint16_t offset, uint8_t flags) {
  if (!attr || !attr->user_data) {
    LOG_ERR("The WRITE dispatcher got an unexpected nullptr");
    return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
  }

  Characteristic *self = static_cast<Characteristic *>(attr->user_data);

  if (self->_writeCallback) {
    return self->_writeCallback(conn, attr, buf, len, offset, flags);
  } else {
    LOG_INF("This is the default WRITE callback");
  }

  return len;
}

void Characteristic::_cccDispatcher(const struct bt_gatt_attr *attr,
                                    uint16_t value) {
  LOG_INF("CCC dispatcher called with value: 0x%04x", value);

  if (!attr || !attr->user_data) {
    LOG_ERR("The CCC dispatcher got an unexpected nullptr");
    return;
  }

  bt_gatt_ccc *ccc = static_cast<bt_gatt_ccc *>(attr->user_data);
  CccWrapper *wrapper = CONTAINER_OF(ccc, CccWrapper, ccc);
  Characteristic *self = wrapper->chr;

  if (self->_cccCallback) {
    return self->_cccCallback(attr, value);
  }

  // Since for now the Peripheral managed only a single connection, this works
  // If for example, the peripheral is connected to multiple centrals, the
  // enabled attributes will be changed by any connection
  // TODO: Expand for multiple connections in the future
  self->_notificationsEnabled = (value & BT_GATT_CCC_NOTIFY);
  self->_indicationsEnabled = (value & BT_GATT_CCC_INDICATE);

  if (self->_notificationsEnabled) {
    LOG_INF("Default CCC callback: Notifications enabled");
  }

  if (self->_indicationsEnabled) {
    LOG_INF("Default CCC callback: Indications enabled");
  }

  if (value == 0) {
    LOG_INF("Default CCC callback: Disabled");
  }
}
