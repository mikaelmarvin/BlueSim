
#pragma once

#include "filter.hpp"
#include <zephyr/logging/log.h>

extern "C" {
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
}

// Forward declaration to avoid circular include
class Central;

// Same as MAX_CENTRALS, but trying to avoid circular includes
#define MAX_SCANNERS                                                           \
  (CONFIG_BT_MAX_CONN - CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT) / 2

struct connection_info {
  struct k_work_delayable work;
  bt_addr_le_t target_addr;
  bool connection_attempted;
};

class Scanner {
public:
  Scanner(Central *owner);
  ~Scanner();

  int startScanning();
  int stopScanning();
  void addFilter(const Filter &filter);

  static void scanCallback(const bt_addr_le_t *addr, int8_t rssi,
                           uint8_t adv_type, struct net_buf_simple *buf);
  static struct bt_le_scan_param scanParameters;
  static bool isStackScanning;
  static int startStackScanning();
  static int stopStackScanning();

  uint8_t _index;
  bool _isScanning;
  Filter _filter;
  Central *_owner;
  struct connection_info _connection;
  static Scanner *registry[MAX_SCANNERS];
};