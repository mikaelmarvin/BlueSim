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

  uint8_t _id;
  struct advertiser_info _advert;
  char _local_name[32];
  struct bt_le_adv_param _adv_param;
  bool _is_advertising = false;
};