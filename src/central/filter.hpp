#pragma once

extern "C" {
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
}

#include <stdbool.h>
#include <stdint.h>

// Maximum lengths for filter patterns and data
#define MAX_PATTERN_LENGTH 64
#define MAX_MANUFACTURER_DATA_LENGTH 32
#define MAX_LOCAL_NAME_LENGTH 32

// Logical operators for combining filter conditions
enum class FilterOperator { AND, OR, NONE };

// Individual filter conditions
struct FilterCondition {
  bool enabled;
  char pattern[MAX_PATTERN_LENGTH];

  FilterCondition() : enabled(false) { pattern[0] = '\0'; }
};

class Filter {
public:
  Filter();
  Filter &operator=(const Filter &other);
  ~Filter() = default;

  // Enable/disable the entire filter
  void setActive(bool active) { _isActive = active; }
  bool isActive() const { return _isActive; }

  // Local name filtering
  void setLocalNamePattern(const char *pattern);
  void enableLocalNameFilter(bool enable) { _localNameFilter.enabled = enable; }
  bool isLocalNameFilterEnabled() const { return _localNameFilter.enabled; }

  // Manufacturer data filtering
  void setManufacturerDataPattern(const char *pattern);
  void enableManufacturerDataFilter(bool enable) {
    _manufacturerDataFilter.enabled = enable;
  }
  bool isManufacturerDataFilterEnabled() const {
    return _manufacturerDataFilter.enabled;
  }

  // Logical operator for combining conditions
  void setOperator(FilterOperator op) { _operator = op; }
  FilterOperator getOperator() const { return _operator; }

  // Main matching function
  bool matchesDevice(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
                     struct net_buf_simple *buf) const;

  // Validation methods
  bool validatePattern(const char *pattern) const;
  bool isValid() const;

private:
  bool _isActive;
  FilterOperator _operator;

  // Individual filter conditions
  FilterCondition _localNameFilter;
  FilterCondition _manufacturerDataFilter;

  // Internal matching functions
  bool matchesLocalName(struct net_buf_simple *buf) const;
  bool matchesManufacturerData(struct net_buf_simple *buf) const;
  bool matchesPattern(const char *data, size_t data_len,
                      const char *pattern) const;

  // Advertisement data parsing
  bool parseAdvertisementData(struct net_buf_simple *buf, char *local_name,
                              size_t name_size, uint8_t *manufacturer_data,
                              size_t mfg_data_size) const;
};
