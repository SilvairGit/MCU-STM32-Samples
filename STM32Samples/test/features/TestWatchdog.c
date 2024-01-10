#include <string.h>

#include "MockAssert.h"
#include "MockSimpleScheduler.h"
#include "MockWatchdogHal.h"
#include "Watchdog.c"
#include "unity.h"

void setUp(void)
{
    IsInitialized = false;
}

void test_Init(void)
{
    TEST_ASSERT_EQUAL(false, Watchdog_IsInitialized());

    WatchdogHal_IsInitialized_ExpectAndReturn(true);
    SimpleScheduler_TaskAdd_Expect(WATCHDOG_TASK_PERIOD_MS, Watchdog_Refresh, SIMPLE_SCHEDULER_TASK_ID_WATCHDOG, true);

    Watchdog_Init();

    TEST_ASSERT_EQUAL(true, Watchdog_IsInitialized());
}

void test_IsInitialized(void)
{
    TEST_ASSERT_EQUAL(false, Watchdog_IsInitialized());

    WatchdogHal_IsInitialized_ExpectAndReturn(false);
    WatchdogHal_Init_Expect();
    SimpleScheduler_TaskAdd_Expect(WATCHDOG_TASK_PERIOD_MS, Watchdog_Refresh, SIMPLE_SCHEDULER_TASK_ID_WATCHDOG, true);

    Watchdog_Init();

    TEST_ASSERT_EQUAL(true, Watchdog_IsInitialized());
}

void test_Refresh(void)
{
    WatchdogHal_Refresh_Expect();
    Watchdog_Refresh();
}
