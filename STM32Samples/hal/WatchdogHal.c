#include "WatchdogHal.h"

#include "Assert.h"
#include "Log.h"
#include "Platform.h"
#include "Utils.h"

#define WATCHDOG_HAL_PRESCALER_VALUE 64
#define WATCHDOG_HAL_RELOAD_COUNTER ((WATCHDOG_HAL_TRIGGER_TIME_MS * LSI_VALUE) / (WATCHDOG_HAL_PRESCALER_VALUE * 1000))

STATIC_ASSERT(WATCHDOG_HAL_RELOAD_COUNTER <= 0x0FFF, Maximum_value_of_IWDG_Reload_Counter_exceeded);

static bool IsInitialized = false;

static void WatchdogHal_WdtInit(void);

void WatchdogHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("WatchdogHal initialization");

    WatchdogHal_WdtInit();
    WatchdogHal_Refresh();

    IsInitialized = true;
}

bool WatchdogHal_IsInitialized(void)
{
    return IsInitialized;
}

void WatchdogHal_Refresh(void)
{
    LL_IWDG_ReloadCounter(IWDG);
}

static void WatchdogHal_WdtInit(void)
{
    LL_IWDG_Enable(IWDG);
    LL_IWDG_EnableWriteAccess(IWDG);

    // This prescaler must be aligned with WATCHDOG_HAL_PRESCALER_VALUE value
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_64);

    LL_IWDG_SetReloadCounter(IWDG, WATCHDOG_HAL_RELOAD_COUNTER);

    while (LL_IWDG_IsReady(IWDG) != 1)
    {
    }
}
