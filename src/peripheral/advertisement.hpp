#pragma once

#include <zephyr/logging/log.h>

extern "C" {
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
}

#define MAX_ADVERTISEMENTS CONFIG_BT_EXT_ADV_MAX_ADV_SET

struct advertiser_info {
  struct k_work_delayable work;
  struct bt_le_ext_adv *adv;
  struct bt_data adv_data[1];
};

class Advertisement {
public:
  Advertisement();
  ~Advertisement();

  int init(const char *advertiser_name = nullptr);
  int startAdvertising();
  int stopAdvertising();
  bool isAdvertising() const;

  static void workAction(struct k_work *work);
  static void extAdvConnectedCb(struct bt_le_ext_adv *adv,
                                struct bt_le_ext_adv_connected_info *info);
  static Advertisement *fromAdv(struct bt_le_ext_adv *adv);

  uint8_t _index; // Order in the registry
  uint8_t _id;    // Unique ID for advertising set
  struct advertiser_info _advert;
  char _localName[32];
  struct bt_le_adv_param _advParam;
  bool _isAdvertising = false;

  // Static attributes
  static struct bt_le_ext_adv_cb _extendedAdvCb;
  static Advertisement *registry[MAX_ADVERTISEMENTS];
};