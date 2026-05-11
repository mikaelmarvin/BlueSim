BOARD ?= nrf52840dk_nrf52840
APP_DIR := $(CURDIR)
BUILD_DIR ?= $(APP_DIR)/build
# By default, west uses flash-runner from build/zephyr/runners.yaml (board + CMake).
# Override only if needed: make flash FLASH_RUNNER=nrfutil

.PHONY: all build flash clean pristine

all: build

build:
	west build -b $(BOARD) $(APP_DIR) -d $(BUILD_DIR)

# Use after moving NCS (e.g. ~/ncs vs /opt/ncs) — stale CMakeCache keeps old absolute paths.
pristine:
	west build -b $(BOARD) $(APP_DIR) -d $(BUILD_DIR) --pristine

flash:
	west flash -d $(BUILD_DIR) $(if $(strip $(FLASH_RUNNER)),--runner $(FLASH_RUNNER),)

clean:
	rm -rf $(BUILD_DIR)
