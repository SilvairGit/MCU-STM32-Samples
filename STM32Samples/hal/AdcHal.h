#ifndef ADC_HAL_H
#define ADC_HAL_H

#include <stdbool.h>
#include <stdint.h>

#define ADC_HAL_REF_V_MV 3300
#define ADC_HAL_ADC_MIN 0
#define ADC_HAL_ADC_MAX 4095

enum AdcHalChannel
{
    ADC_HAL_CHANNEL_ALS,
    ADC_HAL_CHANNEL_POTENTIOMETER,
    ADC_HAL_CHANNEL_RTC_BATTERY,
    ADC_HAL_CHANNEL_TEMPERATURE_SENSOR,
    ADC_HAL_CHANNEL_UP_VCC,
    ADC_HAL_CHANNEL_LENGTH_MARKER
};

void AdcHal_Init(void);

bool AdcHal_IsInitialized(void);

uint16_t AdcHal_ReadChannel(enum AdcHalChannel adc_channel);

uint16_t AdcHal_ReadChannelMv(enum AdcHalChannel adc_channel, uint16_t gain_coefficient);

int32_t AdcHal_ReadTemperature100mc(void);

#endif
