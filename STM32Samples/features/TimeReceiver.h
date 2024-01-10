#ifndef TIME_RECEIVER_H
#define TIME_RECEIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Utils.h"

struct PACKED TimeGetReq
{
    uint8_t instance_index;
};

struct PACKED TimeGetResp
{
    uint8_t  instance_index;
    uint64_t tai_seconds : 40;
    uint64_t subsecond : 8;
    uint64_t tai_utc_delta : 16;
    uint8_t  time_zone_offset;
};

struct TimeReceiver_MeshTimeLastSync
{
    uint32_t local_sync_timestamp_ms;
    uint64_t tai_seconds : 40;
    uint8_t  subsecond;
    uint16_t tai_utc_delta;
    uint8_t  time_zone_offset;
};

STATIC_ASSERT(sizeof(struct TimeGetReq) == 1, Wrong_size_of_the_struct_TimeGetReq);
STATIC_ASSERT(sizeof(struct TimeGetResp) == 10, Wrong_size_of_the_struct_TimeGetResp);

// Initialize Time Receiver module
void TimeReceiver_Init(uint8_t *p_instance_index);

// Check if Time Receiver module is initialized
bool TimeReceiver_IsInitialized(void);

// Get last sync time
struct TimeReceiver_MeshTimeLastSync *TimeReceiver_GetLastSyncTime(void);

#endif    // TIME_RECEIVER_H
