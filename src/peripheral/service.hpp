#pragma once

#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#define MAX_CHARACTERISTICS_PER_SERVICE 3
#define MAX_ATTRIBUTES_PER_SERVICE 3 * MAX_CHARACTERISTICS_PER_SERVICE + 1

class Characteristic;
class Peripheral;

class Service {
public:
  Service();
  ~Service() = default;
  void init(const bt_uuid *uuid, const char *name = "");

  int buildService();
  int addCharacteristic(Characteristic *characteristic);

  Peripheral *_peripheral = nullptr;
  char _name[32];
  struct bt_gatt_service _gattService;

private:
  struct bt_gatt_attr _attrs[MAX_ATTRIBUTES_PER_SERVICE];
  struct bt_gatt_chrc _chrcs[MAX_CHARACTERISTICS_PER_SERVICE];
  struct bt_gatt_ccc_managed_user_data _cccs[MAX_CHARACTERISTICS_PER_SERVICE];
  Characteristic *_characteristics[MAX_CHARACTERISTICS_PER_SERVICE];
  const struct bt_uuid *_uuid = nullptr;
  uint8_t _attrCount = 0;
  uint8_t _chrcCount = 0;
  uint8_t _cccCount = 0;
};