#include "bluetooth/BtManager.hpp"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#ifdef CONFIG_ROLE_CENTRAL
#include "central/Central.hpp"
#endif
#ifdef CONFIG_ROLE_PERIPHERAL
#include "peripheral/Peripheral.hpp"
#endif

int main() { return 0; }
