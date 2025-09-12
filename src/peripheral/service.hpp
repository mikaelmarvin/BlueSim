#pragma once

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

// Forward declaration
class Characteristic;

// Base GATT Service class
class Service {
public:
  Service();
  void init(const bt_uuid *uuid, const char *name = "");
  virtual ~Service();

  // Service management
  int registerService();
  void unregisterService();

  // Characteristic management
  int addCharacteristic(Characteristic *characteristic);

protected:
  int buildServiceAttributes();

  const struct bt_uuid *_uuid;
  char _name[32];
  struct bt_gatt_service _gattService;
  struct bt_gatt_attr _attrs[MAX_ATTRIBUTES_PER_SERVICE];
  struct bt_gatt_chrc _chrcs[MAX_CHARACTERISTICS_PER_SERVICE];
  struct bt_gatt_ccc_cfg _cccs[MAX_CHARACTERISTICS_PER_SERVICE]
                              [BT_GATT_CCC_MAX];
  Characteristic *_characteristics[MAX_CHARACTERISTICS_PER_SERVICE];
  uint8_t _characteristicCount;
  uint8_t _attrCount;
  uint8_t _chrcCount;
  uint8_t _cccCount;
};