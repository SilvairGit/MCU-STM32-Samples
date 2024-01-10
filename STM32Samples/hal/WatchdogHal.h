#ifndef WATCHDOG_HAL_H
#define WATCHDOG_HAL_H

#include <stdbool.h>

#define WATCHDOG_HAL_TRIGGER_TIME_MS 5000

void WatchdogHal_Init(void);

bool WatchdogHal_IsInitialized(void);

void WatchdogHal_Refresh(void);

#endif
