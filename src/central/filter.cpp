#include "filter.hpp"
#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(FILTER, LOG_LEVEL_DBG);

Filter::Filter() : _isActive(false), _operator(FilterOperator::NONE) {}

Filter &Filter::operator=(const Filter &other) {
  if (this != &other) {
    _isActive = other._isActive;
    _operator = other._operator;

    _localNameFilter.enabled = other._localNameFilter.enabled;
    memcpy(_localNameFilter.pattern, other._localNameFilter.pattern,
           MAX_PATTERN_LENGTH);

    _manufacturerDataFilter.enabled = other._manufacturerDataFilter.enabled;
    memcpy(_manufacturerDataFilter.pattern,
           other._manufacturerDataFilter.pattern, MAX_PATTERN_LENGTH);
  }
  return *this;
}

void Filter::setLocalNamePattern(const char *pattern) {
  if (!pattern) {
    LOG_WRN("Pattern cannot be null");
    return;
  }

  if (strlen(pattern) >= MAX_PATTERN_LENGTH) {
    LOG_WRN("Pattern too long");
    return;
  }

  if (!validatePattern(pattern)) {
    LOG_WRN("Invalid pattern syntax");
    return;
  }

  strncpy(_localNameFilter.pattern, pattern, MAX_PATTERN_LENGTH - 1);
  _localNameFilter.pattern[MAX_PATTERN_LENGTH - 1] = '\0';
}

void Filter::setManufacturerDataPattern(const char *pattern) {
  if (!pattern) {
    LOG_WRN("Pattern cannot be null");
    return;
  }

  if (strlen(pattern) >= MAX_PATTERN_LENGTH) {
    LOG_WRN("Pattern too long");
    return;
  }

  if (!validatePattern(pattern)) {
    LOG_WRN("Invalid pattern syntax");
    return;
  }

  strncpy(_manufacturerDataFilter.pattern, pattern, MAX_PATTERN_LENGTH - 1);
  _manufacturerDataFilter.pattern[MAX_PATTERN_LENGTH - 1] = '\0';
}

bool Filter::matchesDevice(const bt_addr_le_t *addr, int8_t rssi,
                           uint8_t adv_type, struct net_buf_simple *buf) const {
  if (!_isActive) {
    return false;
  }

  // Check if any filters are enabled
  bool hasEnabledFilters =
      _localNameFilter.enabled || _manufacturerDataFilter.enabled;
  if (!hasEnabledFilters) {
    // No filters enabled = match everything
    return true;
  }

  // Evaluate individual filter conditions
  bool localNameMatch = false;
  bool manufacturerMatch = false;

  if (_localNameFilter.enabled) {
    localNameMatch = matchesLocalName(buf);
  }

  if (_manufacturerDataFilter.enabled) {
    manufacturerMatch = matchesManufacturerData(buf);
  }

  // Apply logical operator
  switch (_operator) {
  case FilterOperator::AND:
    return localNameMatch && manufacturerMatch;

  case FilterOperator::OR:
    return localNameMatch || manufacturerMatch;

  case FilterOperator::NONE:
    // If only one filter is enabled, return its result
    if (_localNameFilter.enabled && !_manufacturerDataFilter.enabled) {
      return localNameMatch;
    }
    if (_manufacturerDataFilter.enabled && !_localNameFilter.enabled) {
      return manufacturerMatch;
    }
    // If both are enabled but operator is NONE, default to OR
    return localNameMatch || manufacturerMatch;

  default:
    return false;
  }
}

bool Filter::matchesLocalName(struct net_buf_simple *buf) const {
  if (!_localNameFilter.enabled || strlen(_localNameFilter.pattern) == 0) {
    return false;
  }

  char local_name[MAX_LOCAL_NAME_LENGTH];
  uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];

  if (!parseAdvertisementData(buf, local_name, sizeof(local_name),
                              manufacturer_data, sizeof(manufacturer_data))) {
    return false;
  }

  return matchesPattern(local_name, strlen(local_name),
                        _localNameFilter.pattern);
}

bool Filter::matchesManufacturerData(struct net_buf_simple *buf) const {
  if (!_manufacturerDataFilter.enabled ||
      strlen(_manufacturerDataFilter.pattern) == 0) {
    return false;
  }

  char local_name[MAX_LOCAL_NAME_LENGTH];
  uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];

  if (!parseAdvertisementData(buf, local_name, sizeof(local_name),
                              manufacturer_data, sizeof(manufacturer_data))) {
    return false;
  }

  // Convert manufacturer data to hex string for pattern matching
  char hex_string[MAX_MANUFACTURER_DATA_LENGTH * 2 + 1];
  hex_string[0] = '\0';

  for (size_t i = 0; i < sizeof(manufacturer_data); i++) {
    if (manufacturer_data[i] != 0) {
      snprintf(hex_string, sizeof(hex_string), "%02X", manufacturer_data[i]);
      break;
    }
  }

  return matchesPattern(hex_string, strlen(hex_string),
                        _manufacturerDataFilter.pattern);
}

bool Filter::matchesPattern(const char *data, size_t data_len,
                            const char *pattern) const {
  if (!data || !pattern) {
    return false;
  }

  // Simple pattern matching implementation
  // Supports basic wildcards: * (any chars) and ? (single char)
  const char *p = pattern;
  const char *d = data;
  size_t remaining_len = data_len;

  while (*p && remaining_len > 0) {
    if (*p == '*') {
      // Skip to end of string or next pattern character
      p++;
      if (*p == '\0') {
        return true; // * at end matches everything
      }
      // Find next occurrence of *p in data
      while (remaining_len > 0 && *d != *p) {
        d++;
        remaining_len--;
      }
      if (remaining_len == 0) {
        return false;
      }
    } else if (*p == '?') {
      // Match any single character
      p++;
      d++;
      remaining_len--;
    } else if (*p == *d) {
      // Exact match
      p++;
      d++;
      remaining_len--;
    } else {
      return false;
    }
  }

  // Pattern must be exhausted and data must be fully consumed
  return (*p == '\0' && remaining_len == 0);
}

bool Filter::parseAdvertisementData(struct net_buf_simple *buf,
                                    char *local_name, size_t name_size,
                                    uint8_t *manufacturer_data,
                                    size_t mfg_data_size) const {
  if (!buf || !local_name || !manufacturer_data) {
    return false;
  }

  // Initialize output buffers
  memset(local_name, 0, name_size);
  memset(manufacturer_data, 0, mfg_data_size);

  // Parse advertisement data
  uint8_t *data = buf->data;
  size_t len = buf->len;

  while (len > 0) {
    if (len < 2) {
      break;
    }

    uint8_t field_len = data[0];
    if (field_len == 0 || field_len > len - 1) {
      break;
    }

    uint8_t field_type = data[1];
    uint8_t *field_data = &data[2];
    size_t field_data_len = field_len - 1;

    switch (field_type) {
    case BT_DATA_NAME_SHORTENED:
    case BT_DATA_NAME_COMPLETE:
      if (field_data_len < name_size) {
        memcpy(local_name, field_data, field_data_len);
        local_name[field_data_len] = '\0';
      }
      break;

    case BT_DATA_MANUFACTURER_DATA:
      if (field_data_len <= mfg_data_size) {
        memcpy(manufacturer_data, field_data, field_data_len);
      }
      break;

    default:
      break;
    }

    // Move to next field
    data += field_len + 1;
    len -= field_len + 1;
  }

  return true;
}

bool Filter::validatePattern(const char *pattern) const {
  if (!pattern) {
    return false;
  }

  // Basic validation - check for reasonable pattern length
  size_t len = strlen(pattern);
  if (len == 0 || len >= MAX_PATTERN_LENGTH) {
    return false;
  }

  // Check for valid characters (alphanumeric, wildcards, spaces)
  for (size_t i = 0; i < len; i++) {
    char c = pattern[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '*' || c == '?' || c == ' ' ||
          c == '-')) {
      return false;
    }
  }

  return true;
}

bool Filter::isValid() const {
  // Check if any enabled filters have valid patterns
  if (_localNameFilter.enabled && strlen(_localNameFilter.pattern) == 0) {
    return false;
  }

  if (_manufacturerDataFilter.enabled &&
      strlen(_manufacturerDataFilter.pattern) == 0) {
    return false;
  }

  return true;
}
