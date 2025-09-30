#include "advertisement.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ADVERTISEMENT, LOG_LEVEL_INF);

// Advertisement startup delay
constexpr int kAdvStartDelayMs = 50;

Advertisement::Advertisement() : _id(0), _is_advertising(false) {
  memset(_local_name, 0, sizeof(_local_name));
  memset(&_advert, 0, sizeof(_advert));
}

Advertisement::~Advertisement() {
  if (_is_advertising) {
    stopAdvertising();
  }
}

int Advertisement::init(const char *advertiser_name) {
  // Generate a simple ID based on memory address for uniqueness
  _id = bt_id_create(NULL, NULL);

  if (advertiser_name && strlen(advertiser_name) > 0) {
    strncpy(_local_name, advertiser_name, sizeof(_local_name) - 1);
    _local_name[sizeof(_local_name) - 1] = '\0';
  } else {
    strncpy(_local_name, "BlueSim Device", sizeof(_local_name) - 1);
    _local_name[sizeof(_local_name) - 1] = '\0';
  }

  // Initialize the work item
  k_work_init_delayable(&_advert.work, workAction);

  // Create advertisement parameters
  _adv_param = BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN,
                                    BT_GAP_ADV_FAST_INT_MIN_2,
                                    BT_GAP_ADV_FAST_INT_MAX_2, NULL);
  _adv_param.id = _id;

  // Create the extended advertisement
  int err = bt_le_ext_adv_create(&_adv_param, NULL, &_advert.adv);
  if (err) {
    LOG_ERR("Failed to create adv for id %d (err %d)", _id, err);
    return err;
  }
  LOG_INF("Advertisement %d created adv %p", _id, _advert.adv);

  // Set advertisement data
  _advert.adv_data[0] =
      BT_DATA(BT_DATA_NAME_COMPLETE, _local_name, sizeof(_local_name));

  err = bt_le_ext_adv_set_data(_advert.adv, _advert.adv_data, 1, nullptr, 0);
  if (err) {
    LOG_ERR("Advertisement %d failed to set adv data (err %d)", _id, err);
    return err;
  }

  LOG_INF("Advertisement %d initialized successfully with name: %s", _id,
          _local_name);
  return 0;
}

int Advertisement::startAdvertising() {
  if (_is_advertising) {
    LOG_WRN("Advertisement %d is already advertising", _id);
    return 0;
  }

  // Schedule the work item to handle advertising after delay
  int err = k_work_reschedule(&_advert.work, K_MSEC(kAdvStartDelayMs));
  if (err < 0) {
    LOG_ERR("Advertisement %d failed to schedule work item (err %d)", _id, err);
    return err;
  }

  LOG_INF("Advertiser %d work item scheduled for %dms", _id, kAdvStartDelayMs);
  return 0;
}

int Advertisement::stopAdvertising() {
  if (!_is_advertising) {
    LOG_INF("Advertisement %d is not advertising", _id);
    return 0;
  }

  int err = bt_le_ext_adv_stop(_advert.adv);
  if (err) {
    LOG_ERR("Advertisement %d failed to stop advertising (err %d)", _id, err);
    return err;
  }
  _is_advertising = false;
  LOG_INF("Advertisement %d stopped advertising", _id);

  return 0;
}

bool Advertisement::isAdvertising() const { return _is_advertising; }

void Advertisement::workAction(struct k_work *work) {
  // Recover the Advertisement instance from the k_work pointer
  Advertisement *self = CONTAINER_OF(work, Advertisement, _advert.work);

  // Do the actual advertising start
  int err = bt_le_ext_adv_start(self->_advert.adv, BT_LE_EXT_ADV_START_DEFAULT);

  if (err) {
    LOG_ERR("Advertisement %d failed to start advertising (err %d)", self->_id,
            err);
  } else {
    self->_is_advertising = true;
    LOG_INF("Advertisement %d started advertising successfully", self->_id);
  }
}