#include "service.hpp"

Service::Service() {
  _uuid = nullptr;
  memset(_name, 0, sizeof(_name));
  memset(&_gattService, 0, sizeof(_gattService));
  memset(_attrs, 0, sizeof(_attrs));
  memset(_chrcs, 0, sizeof(_chrcs));
  memset(_cccs, 0, sizeof(_cccs));
  memset(_characteristics, 0, sizeof(_characteristics));

  _characteristicCount = 0;
  _attrCount = 0;
  _chrcCount = 0;
  _cccCount = 0;
}

Service::init(const struct bt_uuid *uuid, const char *name = "") {
  _uuid = uuid;
  memcpy(_name, name, sizeof(_name));
  _name[sizeof(_name) - 1] = '\0';

  // Primary Service declaration
  _attrs[_attrCount++] = {
      /* Primary Service */
      .uuid = BT_UUID_GATT_PRIMARY,
      .perm = BT_GATT_PERM_READ,
      .read = bt_gatt_attr_read_service,
      .write = NULL,
      .user_data = &_uuid,
  };
}

int Service::addCharacteristic(Characteristic *characteristic) {
  _characteristics[_characteristicCount++] = characteristic;

  // Characteristic Declaration
  bt_gatt_chrc &chrc = _chrcs[_chrcCount++];
  chrc = {
      .uuid = (bt_uuid *)characteristic->uuid(),
      .properties = characteristic->properties(),
      .value_handle = 0,
  };

  _attrs[_attrCount++] = {
      .uuid = BT_UUID_GATT_CHRC,
      .perm = BT_GATT_PERM_READ,
      .read = bt_gatt_attr_read_chrc,
      .write = nullptr,
      .user_data = &characteristic,
  };

  // Characteristic Value
  _attrs[_attrCount++] = {
      .uuid = (bt_uuid *)characteristic->uuid(),
      .perm = characteristic->permissions(),
      .read = characteristic->readCallback(),
      .write = characteristic->writeCallback(),
      .user_data = characteristic->userData(),
  };

  // Optional CCC
  if (characteristic->properties() &
      (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE)) {
    _attrs[_attrCount++] = {
        .uuid = BT_UUID_GATT_CCC,
        .perm = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
        .read = bt_gatt_attr_read_ccc,
        .write = bt_gatt_attr_write_ccc,
        .user_data = &_cccs[_cccCount++],
    };
  }
}

int Service::registerService() {
  _gattService.attrs = _attrs;
  _gattService.attr_count = _attrCount;

  return bt_gatt_service_register(&_gattService);
}
