#include "Watchdog.h"

#include "Assert.h"
#include "Log.h"
#include "SimpleScheduler.h"
#include "WatchdogHal.h"

#define WATCHDOG_TASK_PERIOD_MS (WATCHDOG_HAL_TRIGGER_TIME_MS / 10)

static bool IsInitialized = false;

static void Watchdog_Refresh(void);

void Watchdog_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("Watchdog initialization");

    if (!WatchdogHal_IsInitialized())
    {
        WatchdogHal_Init();
    }

    SimpleScheduler_TaskAdd(WATCHDOG_TASK_PERIOD_MS, Watchdog_Refresh, SIMPLE_SCHEDULER_TASK_ID_WATCHDOG, true);

    IsInitialized = true;
}

bool Watchdog_IsInitialized(void)
{
    return IsInitialized;
}

static void Watchdog_Refresh(void)
{
    WatchdogHal_Refresh();
}
