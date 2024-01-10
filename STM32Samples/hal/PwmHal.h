#ifndef PWM_HAL_H
#define PWM_HAL_H

#include <stdbool.h>
#include <stdint.h>

#include "SystemHal.h"

#define PWM_HAL_PWM_FREQUENCY_HZ 2000
#define PWM_HAL_TIMER_PRESCALER (SYSTEM_HAL_CLOCK_HZ / PWM_HAL_PWM_FREQUENCY_HZ / UINT16_MAX)
#define PWM_HAL_TIMER_PERIOD (SYSTEM_HAL_CLOCK_HZ / (PWM_HAL_TIMER_PRESCALER + 1) / PWM_HAL_PWM_FREQUENCY_HZ)
#define PWM_HAL_OUTPUT_DUTY_CYCLE_MAX PWM_HAL_TIMER_PERIOD

enum PwmHalChannel
{
    PWM_HAL_CHANNEL_WARM,
    PWM_HAL_CHANNEL_COLD,
    PWM_HAL_CHANNEL_WARM_1_10V,
    PWM_HAL_CHANNEL_COLD_1_10V,
    PWM_HAL_CHANNEL_LENGTH_MARKER
};

void PwmHal_Init(void);

bool PwmHal_IsInitialized(void);

void PwmHal_SetPwmDuty(enum PwmHalChannel channel, uint16_t duty_cycle);

#endif
