#include "scanner.hpp"
#include "central.hpp"

LOG_MODULE_REGISTER(SCANNER, LOG_LEVEL_DBG);

Scanner *Scanner::registry[MAX_SCANNERS] = {nullptr};
struct bt_le_scan_param Scanner::scanParameters = BT_LE_SCAN_PARAM_INIT(
    BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_FILTER_DUPLICATE,
    BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
bool Scanner::isStackScanning = false;

Scanner::Scanner(Central *owner)
    : _index(0), _isScanning(false), _owner(owner) {
  for (uint8_t i = 0; i < MAX_SCANNERS; ++i) {
    if (!Scanner::registry[i]) {
      Scanner::registry[i] = this;
      _index = i;
      return;
    }
  }

  LOG_ERR("Scanner registry full!");
  __ASSERT(false, "Failed to register Scanner");
}

Scanner::~Scanner() { Scanner::registry[_index] = nullptr; }

void Scanner::addFilter(const Filter &filter) {
  _filter = filter;
  _filter.setActive(true);
  LOG_INF("Filter added");
}

int Scanner::startScanning() {
  uint8_t numberOfActiveScanners = 0;
  for (uint8_t i = 0; i < MAX_SCANNERS; i++) {
    Scanner *scanner = Scanner::registry[i];
    if (!scanner) {
      continue;
    }

    if (scanner->_isScanning) {
      ++numberOfActiveScanners;
    }
  }

  // Start Zephyr scanning if this is the first active scanner
  if (numberOfActiveScanners == 0) {
    int err = Scanner::startStackScanning();
    if (err < 0) {
      LOG_ERR("Scanner %d: Failed to start stack scanning (err %d)", _index,
              err);
      return err;
    }
  }

  // Start scanning for the specific instance
  _isScanning = true;
  LOG_INF("Scanner %d: Scanning started", _index);
  return 0;
}

int Scanner::stopScanning() {
  uint8_t numberOfActiveScanners = 0;
  for (Scanner *scanner : Scanner::registry) {
    if (scanner && scanner->_isScanning) {
      ++numberOfActiveScanners;
    }
  }

  // Stop Zephyr scanning if there is only one active scanner left
  if (numberOfActiveScanners == 1) {
    int err = Scanner::stopStackScanning();
    if (err < 0) {
      LOG_ERR("Scanner %d: Failed to stop stack scanning (err %d)", _index,
              err);
      return err;
    }
  }

  // Stop scanning for the specific instance
  _isScanning = false;
  LOG_INF("Scanner %d: Scanning stopped", _index);
  return 0;
}

void Scanner::scanCallback(const bt_addr_le_t *addr, int8_t rssi,
                           uint8_t adv_type, struct net_buf_simple *buf) {
  // Check each active scanner for filter matches
  for (Scanner *scanner : Scanner::registry) {
    if (!scanner || !scanner->_isScanning) {
      continue;
    }

    // Check if filter matches
    bool filterMatched = false;
    filterMatched = scanner->_filter.matchesDevice(addr, rssi, adv_type, buf);

    if (filterMatched && scanner->_owner) {
      LOG_INF("Filter matched for Central %d, initiating connection",
              scanner->_owner->_index);

      // Stop scanning before connection attempt
      int stop_err = Scanner::stopStackScanning();
      if (stop_err < 0) {
        LOG_ERR("Failed to stop scanning for connection (err %d)", stop_err);
        return;
      }

      // Attempt connection
      int conn_err = scanner->_owner->connectToDevice(addr);
      if (conn_err < 0) {
        LOG_ERR("Connection attempt failed (err %d)", conn_err);
      }

      // Note: Scanning restart is handled by Central's deferred work items
      // The Central class will automatically restart scanning via onConnected()
      // callback if the connection fails, or stop scanning if it succeeds
    }
  }
}

int Scanner::startStackScanning() {
  if (Scanner::isStackScanning) {
    LOG_WRN("Stack scanning already active");
    return 0; // Already scanning, not an error
  }

  int err = bt_le_scan_start(&Scanner::scanParameters, &Scanner::scanCallback);
  if (err < 0) {
    LOG_ERR("Failed to start stack scanning (err %d)", err);
    return err;
  }

  Scanner::isStackScanning = true;
  LOG_INF("Stack scanning started successfully");
  return 0;
}

int Scanner::stopStackScanning() {
  if (!Scanner::isStackScanning) {
    LOG_WRN("Stack scanning not active");
    return 0;
  }

  int err = bt_le_scan_stop();
  if (err < 0) {
    LOG_ERR("Failed to stop stack scanning (err %d)", err);
    return err;
  }

  Scanner::isStackScanning = false;
  LOG_INF("Stack scanning stopped successfully");
  return 0;
}
