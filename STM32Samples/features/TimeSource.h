#ifndef TIME_SOURCE_H
#define TIME_SOURCE_H

#include "PCF8523Drv.h"

struct PACKED TimeSourceSetReq
{
    uint8_t                    instance_index;
    struct Pcf8523Drv_TimeDate date;
};

struct PACKED TimeSourceGetReq
{
    uint8_t instance_index;
};

struct PACKED TimeSourceGetResp
{
    uint8_t                    instance_index;
    struct Pcf8523Drv_TimeDate date;
};

struct PACKED TimeSourceSetResp
{
    uint8_t instance_index;
};

STATIC_ASSERT(sizeof(struct TimeSourceSetReq) == 10, Wrong_size_of_the_struct_TimeSourceSetReq);
STATIC_ASSERT(sizeof(struct TimeSourceGetReq) == 1, Wrong_size_of_the_struct_TimeSourceGetReq);
STATIC_ASSERT(sizeof(struct TimeSourceGetResp) == 10, Wrong_size_of_the_struct_TimeSourceGetResp);
STATIC_ASSERT(sizeof(struct TimeSourceSetResp) == 1, Wrong_size_of_the_struct_TimeSourceSetResp);

// Initialize Time Source module
void TimeSource_Init(void);

#endif    // TIME_SOURCE_H
