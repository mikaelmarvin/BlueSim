cmake_minimum_required(VERSION 3.20.0)

# Pull in Zephyr build system
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(BlueSim CXX)

# Add your sources
target_sources(app PRIVATE
    src/main.cpp
    src/central/central.cpp
    src/central/scanner.cpp
    src/central/filter.cpp
    src/peripheral/advertisement.cpp
    src/peripheral/peripheral.cpp
    src/peripheral/service.cpp
    src/peripheral/characteristic.cpp
)

set_target_properties(app PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_compile_options(app PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-sized-deallocation>)
