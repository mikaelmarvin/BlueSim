cmake_minimum_required(VERSION 3.20.0)

# Pull in Zephyr
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ble-simulator)

# Always compile main and common utilities
target_sources(app PRIVATE
    src/main.cpp
    src/bluetooth/BtManager.cpp
)

# Add role-specific sources depending on Kconfig options
if(CONFIG_ROLE_CENTRAL)
  target_sources(app PRIVATE
      src/central/Central.cpp
  )
endif()

if(CONFIG_ROLE_PERIPHERAL)
  target_sources(app PRIVATE
      src/peripheral/Peripheral.cpp
  )
endif()

target_include_directories(app PRIVATE src)
