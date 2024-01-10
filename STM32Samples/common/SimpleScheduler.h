#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum SimpleSchedulerTaskId
{
    // 0 - reserved for uninitialized task
    SIMPLE_SCHEDULER_TASK_ID_MESH = 1,
    SIMPLE_SCHEDULER_TASK_ID_LCD,
    SIMPLE_SCHEDULER_TASK_ID_ATTENTION,
    SIMPLE_SCHEDULER_TASK_ID_UART_PROTOCOL,
    SIMPLE_SCHEDULER_TASK_ID_HEALTH,
    SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS,
    SIMPLE_SCHEDULER_TASK_ID_EMG_L_TEST,
    SIMPLE_SCHEDULER_TASK_ID_RTC,
    SIMPLE_SCHEDULER_TASK_ID_SWITCH,
    SIMPLE_SCHEDULER_TASK_ID_TIME_SYNC,
    SIMPLE_SCHEDULER_TASK_ID_SENSOR_INPUT,
    SIMPLE_SCHEDULER_TASK_ID_WATCHDOG,
    SIMPLE_SCHEDULER_TASK_ID_ENERGY_SENSOR_SIMULATOR,
    SIMPLE_SCHEDULER_TASK_ID_EMERGENCY_DRIVER_SIMULATOR,
    SIMPLE_SCHEDULER_TASK_ID_LENGTH_MARKER,
};

struct SimpleSchedulerTask
{
    uint32_t period_ms;
    uint32_t last_timestamp_cb_called;
    void (*p_cb)(void);
    bool is_enable;
};

void SimpleScheduler_TaskAdd(uint32_t period_ms, void (*const p_cb)(void), enum SimpleSchedulerTaskId task_id, bool is_enable);

void SimpleScheduler_TaskStateChange(enum SimpleSchedulerTaskId task_id, bool is_enable);

void SimpleScheduler_Run(void);

#endif
