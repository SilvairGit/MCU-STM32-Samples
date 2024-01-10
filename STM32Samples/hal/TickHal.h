#ifndef TICK_HAL_H
#define TICK_HAL_H

#include <stdbool.h>
#include <stdint.h>

void TickHal_Init(void);

bool TickHal_IsInitialized(void);

uint32_t TickHal_GetTimestampMs(void);

uint32_t TickHal_GetClockTick(void);

#endif
