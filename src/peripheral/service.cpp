#include "service.hpp"
#include "characteristic.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(SERVICE, LOG_LEVEL_DBG);

Service::Service() {
  memset(_name, 0, sizeof(_name));
  memset(&_gattService, 0, sizeof(_gattService));
  memset(_attrs, 0, sizeof(_attrs));
  memset(_chrcs, 0, sizeof(_chrcs));
  memset(_cccs, 0, sizeof(_cccs));
  memset(_characteristics, 0, sizeof(_characteristics));
  memset(_cccs, 0, sizeof(_cccs));
}

int Service::init(const struct bt_uuid *uuid, const char *name) {
  _uuid = uuid;
  memcpy(_name, name, sizeof(_name));
  _name[sizeof(_name) - 1] = '\0';

  // Primary Service declaration
  _attrs[_attrCount].uuid = BT_UUID_GATT_PRIMARY;
  _attrs[_attrCount].read = bt_gatt_attr_read_service;
  _attrs[_attrCount].write = NULL;
  _attrs[_attrCount].user_data = &_uuid;
  _attrs[_attrCount].perm = BT_GATT_PERM_READ;
  _attrCount++;

  return 1;
}

int Service::addCharacteristic(Characteristic *characteristic) {
  _characteristics[_chrcCount] = characteristic;

  // Characteristic Declaration
  bt_gatt_chrc &chrc = _chrcs[_chrcCount];
  chrc.uuid = (bt_uuid *)characteristic->_uuid;
  chrc.properties = characteristic->_properties;
  chrc.value_handle = 0;
  _chrcCount++;

  _attrs[_attrCount].uuid = BT_UUID_GATT_CHRC;
  _attrs[_attrCount].perm = BT_GATT_PERM_READ;
  _attrs[_attrCount].read = bt_gatt_attr_read_chrc;
  _attrs[_attrCount].write = nullptr;
  _attrs[_attrCount].user_data = &chrc;
  _attrCount++;

  // Characteristic Value
  _attrs[_attrCount].uuid = (bt_uuid *)characteristic->_uuid;
  _attrs[_attrCount].perm = characteristic->_permissions;
  _attrs[_attrCount].read = Characteristic::_readDispatcher;
  _attrs[_attrCount].write = Characteristic::_writeDispatcher;
  _attrs[_attrCount].user_data = characteristic;
  _attrCount++;

  // Optional CCC
  if (characteristic->_properties &
      (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE)) {
    _cccs[_cccCount].ccc.cfg_changed = Characteristic::_cccDispatcher;
    _cccs[_cccCount].chr = characteristic;
    _attrs[_attrCount].uuid = BT_UUID_GATT_CCC;
    _attrs[_attrCount].perm = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE;
    _attrs[_attrCount].read = bt_gatt_attr_read_ccc;
    _attrs[_attrCount].write = bt_gatt_attr_write_ccc;
    _attrs[_attrCount].user_data = &_cccs[_cccCount].ccc;
    _attrCount++;
    _cccCount++;
  }
  return 1;
}

int Service::buildService() {
  _gattService.attrs = _attrs;
  _gattService.attr_count = _attrCount;
  return 0;
}
