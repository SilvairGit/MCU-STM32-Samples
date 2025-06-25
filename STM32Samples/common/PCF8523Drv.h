#ifndef PCF8523_DRV_H
#define PCF8523_DRV_H

#include <stdbool.h>
#include <stdint.h>

#include "Utils.h"

struct PACKED Pcf8523Drv_TimeDate
{
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  seconds;
    uint16_t milliseconds;
};

bool Pcf8523Drv_Init(void);

bool Pcf8523Drv_IsAvailable(void);

void Pcf8523Drv_SetTime(struct Pcf8523Drv_TimeDate *p_time);

bool Pcf8523Drv_GetTime(struct Pcf8523Drv_TimeDate *p_time);

bool Pcf8523Drv_IsResetState(void);

#endif
