#include "Timestamp.h"

#include "Assert.h"
#include "TickHal.h"

const uint32_t TIMESTAMP_MAX                 = UINT32_MAX;
const uint32_t TIMESTAMP_MAX_COMPARABLE_DIFF = UINT32_MAX / 2;

static bool IsInitialized = false;

void Timestamp_Init(void)
{
    ASSERT(!IsInitialized);

    if (!TickHal_IsInitialized())
    {
        TickHal_Init();
    }
}

bool Timestamp_IsInitialized(void)
{
    return IsInitialized;
}

uint32_t Timestamp_GetCurrent(void)
{
    return TickHal_GetTimestampMs();
}

bool Timestamp_Compare(uint32_t timestamp_lhs, uint32_t timestamp_rhs)
{
    return (timestamp_rhs - timestamp_lhs) <= TIMESTAMP_MAX_COMPARABLE_DIFF;
}

uint32_t Timestamp_GetTimeElapsed(uint32_t timestamp_earlier, uint32_t timestamp_further)
{
    if (timestamp_further < timestamp_earlier)
    {
        return timestamp_further + (TIMESTAMP_MAX - timestamp_earlier) + 1;
    }

    return timestamp_further - timestamp_earlier;
}

void Timestamp_DelayMs(uint32_t delay_time)
{
    delay_time = Timestamp_GetCurrent() + delay_time;
    while (delay_time - Timestamp_GetCurrent() != 0)
    {
        // Do nothing
    }
}
