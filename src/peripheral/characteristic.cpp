#include "characteristic.hpp"

void Characteristic::init(const bt_uuid *uuid, CharProperty properties,
                          const char *name = "") {
  _uuid = uuid;
  _properties = properties;
  memcpy(_name, name, sizeof(_name));
  _name[sizeof(_name) - 1] = '\0';
}