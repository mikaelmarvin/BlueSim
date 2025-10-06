#pragma once

#include "peripheral.hpp"

extern "C" {
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
}

// Minimum required attributes are Characteristic declaration, Characteristic
// value and CCC, the +1 is the one per service Service declaration
#define NUMBER_OF_STD_ATTR 3
#define MAX_CHARACTERISTICS_PER_SERVICE 3
#define MAX_ATTRIBUTES_PER_SERVICE                                             \
  NUMBER_OF_STD_ATTR *MAX_CHARACTERISTICS_PER_SERVICE + 1

class Characteristic;
class Peripheral;

// Wrapper for a CCC descriptor that stores both Zephyr's CCC state
// and a pointer to the owning Characteristic for use in the dispatcher.
struct CccWrapper {
  Characteristic *chr;
  bt_gatt_ccc_managed_user_data ccc;
};

class Service {
public:
  Service();
  ~Service() = default;
  int init(const bt_uuid *uuid, const char *name = "");

  int buildService();
  int addCharacteristic(Characteristic *characteristic);

  Peripheral *_peripheral = nullptr;
  char _name[32];
  struct bt_gatt_service _gattService;
  struct bt_gatt_attr _attrs[MAX_ATTRIBUTES_PER_SERVICE];
  struct bt_gatt_chrc _chrcs[MAX_CHARACTERISTICS_PER_SERVICE];
  struct CccWrapper _cccs[MAX_CHARACTERISTICS_PER_SERVICE];
  Characteristic *_characteristics[MAX_CHARACTERISTICS_PER_SERVICE];
  const struct bt_uuid *_uuid = nullptr;
  uint8_t _attrCount = 0;
  uint8_t _chrcCount = 0;
  uint8_t _cccCount = 0;
};