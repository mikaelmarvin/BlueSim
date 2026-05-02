BOARD ?= nrf52840dk_nrf52840
APP_DIR := $(CURDIR)
BUILD_DIR ?= $(APP_DIR)/build

.PHONY: all build flash clean

all: build

build:
	west build -b $(BOARD) $(APP_DIR) -d $(BUILD_DIR)

flash:
	west flash -d $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
