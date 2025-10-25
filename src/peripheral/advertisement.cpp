#include "advertisement.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ADVERTISEMENT, LOG_LEVEL_INF);

Advertisement *Advertisement::registry[MAX_ADVERTISEMENTS] = {nullptr};
struct bt_le_ext_adv_cb Advertisement::_extendedAdvCb = {};

// Advertisement startup delay
constexpr int kAdvStartDelayMs = 50;

Advertisement::Advertisement() : _index(0), _id(0), _isAdvertising(false) {
  memset(_localName, 0, sizeof(_localName));
  memset(&_advert, 0, sizeof(_advert));

  for (uint8_t i = 0; i < MAX_ADVERTISEMENTS; ++i) {
    if (!Advertisement::registry[i]) {
      _index = i;
      Advertisement::registry[i] = this;
      return;
    }
  }

  LOG_ERR("Advertisement registry is full! Maximum %d advertisements allowed.",
          MAX_ADVERTISEMENTS);
  __ASSERT(false, "Failed to register Advertisement - registry full");
}

Advertisement::~Advertisement() {
  if (_isAdvertising) {
    stopAdvertising();
  }
}

int Advertisement::init(const char *advertiser_name) {
  // Generate a simple ID based on memory address for uniqueness
  _id = bt_id_create(NULL, NULL);

  if (advertiser_name && strlen(advertiser_name) > 0) {
    strncpy(_localName, advertiser_name, sizeof(_localName) - 1);
    _localName[sizeof(_localName) - 1] = '\0';
  } else {
    strncpy(_localName, "BlueSim Device", sizeof(_localName) - 1);
    _localName[sizeof(_localName) - 1] = '\0';
  }

  // Initialize the work item
  k_work_init_delayable(&_advert.work, workAction);

  // Create advertisement parameters
  _advParam = BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN,
                                   BT_GAP_ADV_FAST_INT_MIN_2,
                                   BT_GAP_ADV_FAST_INT_MAX_2, NULL);
  _advParam.id = _id;

  Advertisement::_extendedAdvCb = {
      .connected = extAdvConnectedCb,
  };

  // Create the extended advertisement
  int err = bt_le_ext_adv_create(&_advParam, &Advertisement::_extendedAdvCb,
                                 &_advert.adv);
  if (err < 0) {
    LOG_ERR("Failed to create adv for id %d (err %d)", _index, err);
    return err;
  }
  LOG_INF("Advertisement %d created adv %p", _index, _advert.adv);

  // Set advertisement data
  _advert.adv_data[0] =
      BT_DATA(BT_DATA_NAME_COMPLETE, _localName, sizeof(_localName));

  err = bt_le_ext_adv_set_data(_advert.adv, _advert.adv_data, 1, nullptr, 0);
  if (err < 0) {
    LOG_ERR("Advertisement %d failed to set adv data (err %d)", _index, err);
    return err;
  }

  LOG_INF("Advertisement %d initialized successfully with name: %s", _index,
          _localName);
  return 0;
}

int Advertisement::startAdvertising() {
  // Schedule the work item to handle advertising after delay
  int err = k_work_reschedule(&_advert.work, K_MSEC(kAdvStartDelayMs));
  if (err < 0) {
    LOG_ERR("Advertisement %d failed to schedule work item (err %d)", _index,
            err);
    return err;
  }

  LOG_INF("Advertiser %d work item scheduled for %dms", _index,
          kAdvStartDelayMs);
  return 0;
}

int Advertisement::stopAdvertising() {
  if (!_isAdvertising) {
    LOG_INF("Advertisement %d is not advertising", _index);
    return 0;
  }

  int err = bt_le_ext_adv_stop(_advert.adv);
  if (err < 0) {
    LOG_ERR("Advertisement %d failed to stop advertising (err %d)", _index,
            err);
    return err;
  }
  _isAdvertising = false;
  LOG_INF("Advertisement %d stopped advertising", _index);

  return 0;
}

bool Advertisement::isAdvertising() const { return _isAdvertising; }

void Advertisement::workAction(struct k_work *work) {
  // Recover the Advertisement instance from the k_work pointer
  Advertisement *self = CONTAINER_OF(work, Advertisement, _advert.work);

  if (self->_isAdvertising) {
    LOG_INF("Advertisement %d is already advertising", self->_index);
    return;
  }

  // Do the actual advertising start
  int err = bt_le_ext_adv_start(self->_advert.adv, BT_LE_EXT_ADV_START_DEFAULT);

  if (err < 0) {
    LOG_ERR("Advertisement %d failed to start advertising (err %d)",
            self->_index, err);
  } else {
    self->_isAdvertising = true;
    LOG_INF("Advertisement %d started advertising successfully", self->_index);
  }
}

void Advertisement::extAdvConnectedCb(
    struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info) {
  Advertisement *advertisement = Advertisement::fromAdv(adv);
  if (!advertisement) {
    return;
  }

  // Update the isAdvertising flag
  advertisement->_isAdvertising = false;
}

Advertisement *Advertisement::fromAdv(bt_le_ext_adv *adv) {
  for (Advertisement *advertisement : Advertisement::registry) {
    if (!advertisement) {
      continue;
    }

    if (advertisement->_advert.adv == adv) {
      return advertisement;
    }
  }

  LOG_WRN("Failed to find Advertisement for bt_le_ext_adv %p", adv);
  return nullptr;
}
