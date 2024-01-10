#include "SimpleScheduler.h"

#include "Assert.h"
#include "Atomic.h"
#include "Log.h"
#include "Timestamp.h"

#define SIMPLE_SCHEDULER_TASK_LUT_OFFSET 1
#define SIMPLE_SCHEDULER_TASK_LUT_EMPTY_VALUE 0

static void SimpleScheduler_TaskExecutor(void);

static struct SimpleSchedulerTask TaskList[SIMPLE_SCHEDULER_TASK_ID_LENGTH_MARKER];
static uint32_t                   TaskLut[SIMPLE_SCHEDULER_TASK_ID_LENGTH_MARKER];

static uint32_t NumberOfTaskCnt = 0;

void SimpleScheduler_TaskAdd(uint32_t period_ms, void (*const p_cb)(void), enum SimpleSchedulerTaskId task_id, bool is_enable)
{
    ASSERT((p_cb != NULL) && (task_id < SIMPLE_SCHEDULER_TASK_ID_LENGTH_MARKER) && (TaskList[NumberOfTaskCnt].p_cb == NULL));

    LOG_D("New task added, id: %u, ptr: 0x%08X", task_id, (uintptr_t)p_cb);

    TaskList[NumberOfTaskCnt].period_ms                = period_ms;
    TaskList[NumberOfTaskCnt].p_cb                     = p_cb;
    TaskList[NumberOfTaskCnt].is_enable                = is_enable;
    TaskList[NumberOfTaskCnt].last_timestamp_cb_called = 0;

    TaskLut[task_id] = NumberOfTaskCnt + SIMPLE_SCHEDULER_TASK_LUT_OFFSET;
    NumberOfTaskCnt++;
}

void SimpleScheduler_TaskStateChange(enum SimpleSchedulerTaskId task_id, bool is_enable)
{
    ASSERT(task_id < SIMPLE_SCHEDULER_TASK_ID_LENGTH_MARKER);

    uint32_t task_index = TaskLut[task_id];

    if (task_index == SIMPLE_SCHEDULER_TASK_LUT_EMPTY_VALUE)
    {
        return;
    }

    task_index -= SIMPLE_SCHEDULER_TASK_LUT_OFFSET;

    Atomic_CriticalEnter();

    TaskList[task_index].is_enable                = is_enable;
    TaskList[task_index].last_timestamp_cb_called = Timestamp_GetCurrent();

    Atomic_CriticalExit();
}

void SimpleScheduler_Run(void)
{
    while (true)
    {
        // This function must be a function only for testing purpose
        SimpleScheduler_TaskExecutor();
    }
}

static void SimpleScheduler_TaskExecutor(void)
{
    static uint32_t i = 0;

    if (TaskList[i].is_enable && (Timestamp_GetTimeElapsed(TaskList[i].last_timestamp_cb_called, Timestamp_GetCurrent()) >= TaskList[i].period_ms))
    {
        TaskList[i].last_timestamp_cb_called += TaskList[i].period_ms;
        TaskList[i].p_cb();
    }

    i++;
    if (i == NumberOfTaskCnt)
    {
        i = 0;
    }
}
