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
#define MAX_CRITERIA_PER_GROUP 4
#define MAX_FILTER_GROUPS 4

// Logical operators for combining filter conditions
enum class FilterOperator { AND, OR };

// Field types that can be filtered
enum class FilterCriterionType {
  LOCAL_NAME,         // Match against device local name
  MANUFACTURER_DATA,  // Match against manufacturer data
  SERVICE_UUID,       // Match against service UUID in advertisement
  CHARACTERISTIC_UUID // Match against characteristic UUID in advertisement
};

// Individual filter criterion (field check)
struct FilterCriterion {
  FilterCriterionType type;
  char pattern[MAX_PATTERN_LENGTH];
  bool enabled;

  FilterCriterion()
      : type(FilterCriterionType::LOCAL_NAME), enabled(false) {
    pattern[0] = '\0';
  }
};

// A filter group containing multiple criteria
struct FilterGroup {
  FilterCriterion criteria[MAX_CRITERIA_PER_GROUP];
  uint8_t criteria_count;
  FilterOperator operator_between; // How to combine criteria in this group
  bool enabled;

  FilterGroup() : criteria_count(0), operator_between(FilterOperator::AND),
                  enabled(true) {}
};

class Filter {
public:
  Filter();
  Filter &operator=(const Filter &other);
  ~Filter() = default;

  // Main API: Add criteria to current group and create new groups
  void addGroup();
  void addCriterion(FilterCriterionType type, const char *pattern);
  void setGroupOperator(FilterOperator op); // Set operator for current group
 

  bool matchesDevice(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
                     struct net_buf_simple *buf) const;

  bool validatePattern(const char *pattern) const;
  bool isValid() const;

private:
  FilterGroup _groups[MAX_FILTER_GROUPS];
  uint8_t _group_count;
  uint8_t _current_group_index;

  // Internal matching functions
  bool matchesCriterion(const FilterCriterion &criterion,
                        struct net_buf_simple *buf) const;
  bool evaluateGroup(const FilterGroup &group,
                     struct net_buf_simple *buf) const;
  bool matchesPattern(const char *data, size_t data_len,
                      const char *pattern) const;

  // Advertisement data parsing
  bool parseAdvertisementData(struct net_buf_simple *buf, char *local_name,
                              size_t name_size, uint8_t *manufacturer_data,
                              size_t mfg_data_size, uint8_t *service_uuid,
                              size_t service_uuid_size,
                              uint8_t *characteristic_uuid,
                              size_t characteristic_uuid_size) const;

  // Helper functions to extract specific fields
  bool getLocalName(struct net_buf_simple *buf, char *local_name,
                    size_t name_size) const;
  bool getManufacturerData(struct net_buf_simple *buf, uint8_t *data,
                           size_t data_size) const;
  bool getServiceUUID(struct net_buf_simple *buf, uint8_t *uuid,
                      size_t uuid_size) const;
  bool getCharacteristicUUID(struct net_buf_simple *buf, uint8_t *uuid,
                              size_t uuid_size) const;
};
