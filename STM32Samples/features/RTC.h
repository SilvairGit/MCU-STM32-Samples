#ifndef RTC_H
#define RTC_H

#include <stddef.h>
#include <stdint.h>

#include "PCF8523Drv.h"

#define RTC_TIME_ACCURACY_PPB 10000 /**<  Accuracy of PCF8253 RTC */
#define RTC_WITH_BATTERY_ATTACHED 0x03
#define RTC_WITHOUT_BATTERY_ATTACHED 0x01
#define RTC_NOT_ATTACHED 0x00

typedef void (*RTCTimeGetProcessedCallback_T)(struct Pcf8523Drv_TimeDate *p_time);
typedef void (*RTCTimeSetProcessedCallback_T)(void);

/* Function for initializing RTC module
 * @param[in] time_get_processed_cbk - function to be called when get time processed
 * @param[in] time_set_processed_cbk - function to be called when set time processed
 * @param[in] p_instance_idx - pointer to instance index
 */
void RTC_Init(RTCTimeGetProcessedCallback_T time_get_processed_cbk, RTCTimeSetProcessedCallback_T time_set_processed_cbk, uint8_t *p_instance_idx);

// Check if RTC module is initialized
bool RTC_IsInitialized(void);

// Trigger time set in RTC module
void RTC_TriggerTimeSet(struct Pcf8523Drv_TimeDate *p_time);

// Trigger time get from RTC module
void RTC_TriggerTimeGet(void);

// Check if battery is detected
bool RTC_IsBatteryDetected(void);

// Stop counting time by RTC module
void RTC_StopCounting(void);

#endif /* RTC_H */
