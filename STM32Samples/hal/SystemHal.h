#ifndef SYSTEM_HAL_H
#define SYSTEM_HAL_H

#include <stdbool.h>
#include <stdint.h>

#define SYSTEM_HAL_CLOCK_HZ 72000000

void SystemHal_Init(void);

bool SystemHal_IsInitialized(void);

uint32_t SystemHal_GetCoreClock(void);

void SystemHal_PrintResetCause(void);

void SystemHal_PrintBuildInfo(void);

#endif
