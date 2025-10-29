#include "filter.hpp"
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(FILTER, LOG_LEVEL_DBG);

Filter::Filter() : _group_count(0), _current_group_index(0) {}

Filter &Filter::operator=(const Filter &other) {
  if (this != &other) {
    _group_count = other._group_count;
    _current_group_index = other._current_group_index;

    // Copy all groups
    for (uint8_t i = 0; i < MAX_FILTER_GROUPS; i++) {
      _groups[i] = other._groups[i];
    }
  }
  return *this;
}

void Filter::addGroup() {
  if (_group_count >= MAX_FILTER_GROUPS) {
    LOG_WRN("Maximum number of filter groups reached");
    return;
  }

  _current_group_index = _group_count;
  _group_count++;
  LOG_INF("Added filter group %d", _current_group_index);
}

void Filter::addCriterion(FilterCriterionType type, const char *pattern) {
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

  if (_current_group_index >= _group_count) {
    LOG_WRN("No active group. Call addGroup() first");
    return;
  }

  FilterGroup &current_group = _groups[_current_group_index];
  if (current_group.criteria_count >= MAX_CRITERIA_PER_GROUP) {
    LOG_WRN("Maximum criteria per group reached");
    return;
  }

  uint8_t index = current_group.criteria_count;
  current_group.criteria[index].type = type;
  current_group.criteria[index].enabled = true;
  strncpy(current_group.criteria[index].pattern, pattern,
          MAX_PATTERN_LENGTH - 1);
  current_group.criteria[index].pattern[MAX_PATTERN_LENGTH - 1] = '\0';
  current_group.criteria_count++;

  LOG_INF("Added criterion type %d with pattern '%s' to group %d", (int)type,
          pattern, _current_group_index);
}

void Filter::setGroupOperator(FilterOperator op) {
  if (_current_group_index >= _group_count) {
    LOG_WRN("No active group");
    return;
  }

  _groups[_current_group_index].operator_between = op;
  LOG_INF("Set operator for group %d to %d", _current_group_index, (int)op);
}

bool Filter::matchesDevice(const bt_addr_le_t *addr, int8_t rssi,
                           uint8_t adv_type, struct net_buf_simple *buf) const {
  // If no groups are configured, match everything
  if (_group_count == 0) {
    return true;
  }

  // Evaluate each group
  bool group_results[MAX_FILTER_GROUPS];
  for (uint8_t i = 0; i < _group_count; i++) {
    group_results[i] = evaluateGroup(_groups[i], buf);
  }

  // At least one group must match
  for (uint8_t i = 0; i < _group_count; i++) {
    if (group_results[i]) {
      return true;
    }
  }
  return false;
}

bool Filter::evaluateGroup(const FilterGroup &group,
                           struct net_buf_simple *buf) const {
  if (!group.enabled || group.criteria_count == 0) {
    return false;
  }

  // Evaluate each criterion in the group
  bool criterion_results[MAX_CRITERIA_PER_GROUP];
  for (uint8_t i = 0; i < group.criteria_count; i++) {
    criterion_results[i] = matchesCriterion(group.criteria[i], buf);
  }

  // Combine criterion results with the group's operator
  if (group.operator_between == FilterOperator::OR) {
    // At least one criterion must match
    for (uint8_t i = 0; i < group.criteria_count; i++) {
      if (criterion_results[i]) {
        return true;
      }
    }
    return false;
  } else {
    // All criteria must match (AND)
    for (uint8_t i = 0; i < group.criteria_count; i++) {
      if (!criterion_results[i]) {
        return false;
      }
    }
    return true;
  }
}

bool Filter::matchesCriterion(const FilterCriterion &criterion,
                              struct net_buf_simple *buf) const {
  if (!criterion.enabled) {
    return false;
  }

  switch (criterion.type) {
  case FilterCriterionType::LOCAL_NAME: {
    char local_name[MAX_LOCAL_NAME_LENGTH];
    if (getLocalName(buf, local_name, sizeof(local_name))) {
      return matchesPattern(local_name, strlen(local_name), criterion.pattern);
    }
    return false;
  }

  case FilterCriterionType::MANUFACTURER_DATA: {
    uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];
    if (getManufacturerData(buf, manufacturer_data,
                            sizeof(manufacturer_data))) {
      // Convert to hex string for pattern matching
      char hex_string[MAX_MANUFACTURER_DATA_LENGTH * 2 + 1];
      for (size_t i = 0; i < sizeof(manufacturer_data); i++) {
        snprintf(&hex_string[i * 2], 3, "%02X", manufacturer_data[i]);
      }
      return matchesPattern(hex_string, strlen(hex_string), criterion.pattern);
    }
    return false;
  }

  case FilterCriterionType::SERVICE_UUID: {
    uint8_t service_uuid[16];
    if (getServiceUUID(buf, service_uuid, sizeof(service_uuid))) {
      char hex_string[33]; // 16 bytes * 2
      for (size_t i = 0; i < sizeof(service_uuid); i++) {
        snprintf(&hex_string[i * 2], 3, "%02X", service_uuid[i]);
      }
      return matchesPattern(hex_string, strlen(hex_string), criterion.pattern);
    }
    return false;
  }

  case FilterCriterionType::CHARACTERISTIC_UUID: {
    uint8_t char_uuid[16];
    if (getCharacteristicUUID(buf, char_uuid, sizeof(char_uuid))) {
      char hex_string[33];
      for (size_t i = 0; i < sizeof(char_uuid); i++) {
        snprintf(&hex_string[i * 2], 3, "%02X", char_uuid[i]);
      }
      return matchesPattern(hex_string, strlen(hex_string), criterion.pattern);
    }
    return false;
  }

  default:
    return false;
  }
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
                                    size_t mfg_data_size, uint8_t *service_uuid,
                                    size_t service_uuid_size,
                                    uint8_t *characteristic_uuid,
                                    size_t characteristic_uuid_size) const {
  if (!buf) {
    return false;
  }

  // Initialize output buffers
  if (local_name)
    memset(local_name, 0, name_size);
  if (manufacturer_data)
    memset(manufacturer_data, 0, mfg_data_size);
  if (service_uuid)
    memset(service_uuid, 0, service_uuid_size);
  if (characteristic_uuid)
    memset(characteristic_uuid, 0, characteristic_uuid_size);

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
      if (local_name && field_data_len < name_size) {
        memcpy(local_name, field_data, field_data_len);
        local_name[field_data_len] = '\0';
      }
      break;

    case BT_DATA_MANUFACTURER_DATA:
      if (manufacturer_data && field_data_len <= mfg_data_size) {
        memcpy(manufacturer_data, field_data, field_data_len);
      }
      break;

    case BT_DATA_UUID16_SOME:
    case BT_DATA_UUID16_ALL:
    case BT_DATA_UUID32_SOME:
    case BT_DATA_UUID32_ALL:
    case BT_DATA_UUID128_SOME:
    case BT_DATA_UUID128_ALL:
      // For service UUID, we extract the first UUID found
      if (service_uuid && field_data_len <= service_uuid_size) {
        memcpy(service_uuid, field_data, field_data_len);
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

bool Filter::getLocalName(struct net_buf_simple *buf, char *local_name,
                          size_t name_size) const {
  uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];
  uint8_t service_uuid[16];
  uint8_t characteristic_uuid[16];

  return parseAdvertisementData(buf, local_name, name_size, manufacturer_data,
                                sizeof(manufacturer_data), service_uuid,
                                sizeof(service_uuid), characteristic_uuid,
                                sizeof(characteristic_uuid));
}

bool Filter::getManufacturerData(struct net_buf_simple *buf, uint8_t *data,
                                 size_t data_size) const {
  char local_name[MAX_LOCAL_NAME_LENGTH];
  uint8_t service_uuid[16];
  uint8_t characteristic_uuid[16];

  return parseAdvertisementData(
      buf, local_name, sizeof(local_name), data, data_size, service_uuid,
      sizeof(service_uuid), characteristic_uuid, sizeof(characteristic_uuid));
}

bool Filter::getServiceUUID(struct net_buf_simple *buf, uint8_t *uuid,
                            size_t uuid_size) const {
  char local_name[MAX_LOCAL_NAME_LENGTH];
  uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];
  uint8_t characteristic_uuid[16];

  return parseAdvertisementData(buf, local_name, sizeof(local_name),
                                manufacturer_data, sizeof(manufacturer_data),
                                uuid, uuid_size, characteristic_uuid,
                                sizeof(characteristic_uuid));
}

bool Filter::getCharacteristicUUID(struct net_buf_simple *buf, uint8_t *uuid,
                                   size_t uuid_size) const {
  char local_name[MAX_LOCAL_NAME_LENGTH];
  uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];
  uint8_t service_uuid[16];

  // For now, characteristic UUID is same as service UUID in advertisement
  // This is a limitation - characteristics are typically only available
  // after connection and service discovery
  return parseAdvertisementData(buf, local_name, sizeof(local_name),
                                manufacturer_data, sizeof(manufacturer_data),
                                service_uuid, sizeof(service_uuid), uuid,
                                uuid_size);
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

  // Check for valid characters (alphanumeric, wildcards, spaces, colons for
  // UUIDs)
  for (size_t i = 0; i < len; i++) {
    char c = pattern[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '*' || c == '?' || c == ' ' ||
          c == '-' || c == ':')) {
      return false;
    }
  }

  return true;
}

bool Filter::isValid() const {
  // Check if any groups are enabled and have criteria
  for (uint8_t i = 0; i < _group_count; i++) {
    if (_groups[i].enabled && _groups[i].criteria_count > 0) {
      return true;
    }
  }
  return false;
}
