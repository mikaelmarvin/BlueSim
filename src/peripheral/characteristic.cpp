#include "characteristic.hpp"

void Characteristic::init(const bt_uuid *uuid, CharProperty properties,
                          const char *name = "") {
  _uuid = uuid;
  _properties = properties;
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
    self->_readCallback(conn, attr, buf, len, offset);
  } else {
    LOG_INF("This is the default READ callback");
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
    self->_writeCallback(conn, attr, buf, len, offset, flags);
  } else {
    LOG_INF("This is the default WRITE callback");
  }
}

void Characteristic::_cccDispatcher(const struct bt_gatt_attr *attr,
                                    uint16_t value) {
  if (!attr || !attr->user_data) {
    LOG_ERR("The CCC dispatcher got an unexpected nullptr");
    return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
  }
  Characteristic *self = static_cast<Characteristic *>(attr->user_data);

  if (self->_cccCallback) {
    self->_cccCallback(attr, value);
  } else {
    LOG_INF("This is the default CCC callback");
  }
}
