#include <string.h>

#include "MockAssert.h"
#include "MockAtomicHal.h"
#include "MockTickHal.h"
#include "SimpleScheduler.c"
#include "unity.h"

static uint32_t Task1ExeCnt = 0;
static uint32_t Task2ExeCnt = 0;
static uint32_t Task3ExeCnt = 0;

void task1(void)
{
    Task1ExeCnt++;
}

void task2(void)
{
    Task2ExeCnt++;
}

void task3(void)
{
    Task3ExeCnt++;
}

void setUp(void)
{
    NumberOfTaskCnt = 0;

    memset(TaskList, 0, sizeof(TaskList));

    Task1ExeCnt = 0;
    Task2ExeCnt = 0;
    Task3ExeCnt = 0;
}

void test_TaskAdd(void)
{
    SimpleScheduler_TaskAdd(0, task1, 0, true);
    SimpleScheduler_TaskAdd(1, task2, 1, true);
    SimpleScheduler_TaskAdd(100, task3, 0, false);

    TEST_ASSERT_EQUAL(TaskList[0].period_ms, 0);
    TEST_ASSERT_EQUAL(TaskList[0].p_cb, task1);
    TEST_ASSERT_EQUAL(TaskList[0].is_enable, true);

    TEST_ASSERT_EQUAL(TaskList[1].period_ms, 1);
    TEST_ASSERT_EQUAL(TaskList[1].p_cb, task2);
    TEST_ASSERT_EQUAL(TaskList[1].is_enable, true);

    TEST_ASSERT_EQUAL(TaskList[2].period_ms, 100);
    TEST_ASSERT_EQUAL(TaskList[2].p_cb, task3);
    TEST_ASSERT_EQUAL(TaskList[2].is_enable, false);

    TEST_ASSERT_EQUAL(NumberOfTaskCnt, 3);
}

void test_TaskAddNullCb(void)
{
    Assert_Callback_ExpectAnyArgs();
    SimpleScheduler_TaskAdd(0, NULL, 0, true);
}

void test_TaskAddTaskIdOutOfRange(void)
{
    Assert_Callback_ExpectAnyArgs();
    SimpleScheduler_TaskAdd(0, NULL, SIMPLE_SCHEDULER_TASK_ID_LENGTH_MARKER, true);
}

void test_TaskEnable(void)
{
    SimpleScheduler_TaskAdd(100, task3, 2, false);
    SimpleScheduler_TaskAdd(10, task2, 1, false);
    SimpleScheduler_TaskAdd(0, task1, 0, false);

    AtomicHal_IrqDisable_Expect();
    TickHal_GetTimestampMs_ExpectAnyArgsAndReturn(10);

    AtomicHal_IrqEnable_Expect();
    SimpleScheduler_TaskStateChange(1, false);
    TEST_ASSERT_EQUAL(TaskList[0].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[2].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].last_timestamp_cb_called, 10);

    AtomicHal_IrqDisable_Expect();
    TickHal_GetTimestampMs_ExpectAnyArgsAndReturn(25);
    AtomicHal_IrqEnable_Expect();
    SimpleScheduler_TaskStateChange(1, true);
    TEST_ASSERT_EQUAL(TaskList[0].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].is_enable, true);
    TEST_ASSERT_EQUAL(TaskList[2].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].last_timestamp_cb_called, 25);

    AtomicHal_IrqDisable_Expect();
    TickHal_GetTimestampMs_ExpectAnyArgsAndReturn(100);
    AtomicHal_IrqEnable_Expect();
    SimpleScheduler_TaskStateChange(1, false);
    TEST_ASSERT_EQUAL(TaskList[0].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[2].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].last_timestamp_cb_called, 100);

    AtomicHal_IrqDisable_Expect();
    TickHal_GetTimestampMs_ExpectAnyArgsAndReturn(123);
    AtomicHal_IrqEnable_Expect();
    SimpleScheduler_TaskStateChange(0, true);
    TEST_ASSERT_EQUAL(TaskList[0].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[1].is_enable, false);
    TEST_ASSERT_EQUAL(TaskList[2].is_enable, true);
    TEST_ASSERT_EQUAL(TaskList[2].last_timestamp_cb_called, 123);
}

void test_RunTaskDisable(void)
{
    SimpleScheduler_TaskAdd(0, task1, 0, false);

    size_t i;
    for (i = 0; i < 1000; i++)
    {
        SimpleScheduler_TaskExecutor();
    }

    TEST_ASSERT_EQUAL(Task1ExeCnt, 0);
}

void test_RunWithZeroPeriod(void)
{
    SimpleScheduler_TaskAdd(0, task1, 0, true);

    size_t i;
    for (i = 0; i < 100; i++)
    {
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
    }

    TEST_ASSERT_EQUAL(Task1ExeCnt, 200);
}

void test_RunWithOnePeriod(void)
{
    SimpleScheduler_TaskAdd(1, task1, 0, true);

    size_t i;
    for (i = 0; i < 500 + 1; i++)
    {
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
    }

    TEST_ASSERT_EQUAL(Task1ExeCnt, 500);
}

void test_RunWithPeriod(void)
{
    SimpleScheduler_TaskAdd(10, task1, 0, true);

    size_t i;
    for (i = 0; i < 500 + 1; i++)
    {
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
    }

    TEST_ASSERT_EQUAL(Task1ExeCnt, 50);
}

void test_RunMutipleTask(void)
{
    SimpleScheduler_TaskAdd(5, task3, 2, true);
    SimpleScheduler_TaskAdd(10, task2, 1, true);
    SimpleScheduler_TaskAdd(20, task1, 0, true);

    size_t i;
    for (i = 0; i < 100 + 1; i++)
    {
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
        TickHal_GetTimestampMs_ExpectAndReturn(i);
        SimpleScheduler_TaskExecutor();
    }

    TEST_ASSERT_EQUAL(Task3ExeCnt, 20);
    TEST_ASSERT_EQUAL(Task2ExeCnt, 10);
    TEST_ASSERT_EQUAL(Task1ExeCnt, 5);
}
