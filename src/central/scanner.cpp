#include "scanner.hpp"
#include "central.hpp"

LOG_MODULE_REGISTER(SCANNER, LOG_LEVEL_DBG);

Scanner *Scanner::registry[MAX_SCANNERS] = {nullptr};
struct bt_le_scan_param Scanner::scanParameters = BT_LE_SCAN_PARAM_INIT(
    BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_FILTER_DUPLICATE,
    BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);

Scanner::Scanner(Central *owner)
    : _index(0), _isScanning(false), _owner(owner) {
  // Initialize the work item
  k_work_init_delayable(&_connection.work, connectionWorkAction);
  _connection.connection_attempted = false;
  
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

void Scanner::initiateConnection(const bt_addr_le_t *addr) {
  // Store the target address
  memcpy(&_connection.target_addr, addr, sizeof(bt_addr_le_t));
  _connection.connection_attempted = false;
  
  // Schedule the connection work item with a small delay
  // The work item will handle stopping scanning and attempting connection
  int err = k_work_reschedule(&_connection.work, K_MSEC(100));
  if (err < 0) {
    LOG_ERR("Scanner %d: Failed to schedule connection work item (err %d)", _index, err);
  } else {
    LOG_INF("Scanner %d: Connection work item scheduled", _index);
  }
}

int Scanner::startScanning() {
  uint8_t numberOfActiveScanners = 0;
  for (uint8_t i = 0; i < MAX_SCANNERS; i++) {
    Scanner *scanner = Scanner::registry[i];
    if (!scanner) {
      continue;
    }

    LOG_INF("Registry[%d]: Scanner %d, _isScanning=%s", i, scanner->_index,
            scanner->_isScanning ? "true" : "false");
    if (scanner->_isScanning) {
      ++numberOfActiveScanners;
    }
  }

  // Start Zephyr scanning if this is the first active scanner
  if (numberOfActiveScanners == 0) {
    int err =
        bt_le_scan_start(&Scanner::scanParameters, &Scanner::scanCallback);
    if (err < 0) {
      LOG_ERR("Failed to start scanning (err %d)", err);
      return err;
    }
  }

  // Start scanning for the specific instance
  _isScanning = true;
  LOG_INF("Scanning started");
  return 0;
}

int Scanner::stopScanning() {
  uint8_t numberOfActiveScanners = 0;
  for (Scanner *scanner : Scanner::registry) {
    if (scanner->_isScanning) {
      ++numberOfActiveScanners;
    }
  }

  // Stop Zephyr scanning if there is only one active scanner left
  if (numberOfActiveScanners == 1) {
    int err = bt_le_scan_stop();
    if (err < 0) {
      LOG_ERR("Failed to stop scanning (err %d)", err);
      return err;
    }
  }

  // Stop scanning for the specific instance; _isScanning is a flag used in the
  // scanCallback
  _isScanning = false;
  LOG_INF("Scanning stopped");
  return 0;
}

void Scanner::connectionWorkAction(struct k_work *work) {
  // Recover the Scanner instance from the k_work pointer
  Scanner *self = CONTAINER_OF(work, Scanner, _connection.work);
  
  if (self->_connection.connection_attempted) {
    LOG_INF("Scanner %d: Connection already attempted", self->_index);
    return;
  }
  
  self->_connection.connection_attempted = true;
  
  // Stop scanning before attempting connection
  if (self->_isScanning) {
    int stop_err = bt_le_scan_stop();
    if (stop_err < 0) {
      LOG_ERR("Scanner %d: Failed to stop Zephyr scanning (err %d)", self->_index, stop_err);
    } else {
      LOG_INF("Scanner %d: Stopped Zephyr scanning for connection", self->_index);
    }
    self->_isScanning = false;
  }
  
  LOG_INF("Scanner %d: Attempting connection to device", self->_index);
  
  // Attempt the connection
  int err = self->_owner->connectToDevice(&self->_connection.target_addr);
  if (err < 0) {
    LOG_ERR("Scanner %d: Failed to connect to device (err %d)", self->_index, err);
    // Force restart scanning after failed connection attempt
    int scan_err = bt_le_scan_start(&Scanner::scanParameters, &Scanner::scanCallback);
    if (scan_err < 0) {
      LOG_ERR("Scanner %d: Failed to restart Zephyr scanning (err %d)", self->_index, scan_err);
    } else {
      LOG_INF("Scanner %d: Force restarted Zephyr scanning after failed connection", self->_index);
      self->_isScanning = true;
    }
  } else {
    LOG_INF("Scanner %d: Connection initiated successfully", self->_index);
    // Don't restart scanning - let the connection callback handle it
  }
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
    if (scanner->_filter.isActive()) {
      filterMatched = scanner->_filter.matchesDevice(addr, rssi, adv_type, buf);
    }

    // If no filters are active, consider it a match (scan all devices)
    if (!filterMatched && !scanner->hasActiveFilters()) {
      filterMatched = true;
    }

    if (filterMatched && scanner->_owner) {
      LOG_INF("Filter matched for Central %d, initiating connection",
              scanner->_owner->_index);
      scanner->initiateConnection(addr);
    }
  }
}
