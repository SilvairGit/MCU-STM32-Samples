#include "RTC.h"

#include "AdcHal.h"
#include "Assert.h"
#include "Config.h"
#include "GpioHal.h"
#include "Log.h"
#include "MCU_Health.h"
#include "MeshGenericBattery.h"
#include "ModelManager.h"
#include "SimpleScheduler.h"
#include "TAILocalTimeConverter.h"
#include "TimeReceiver.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Utils.h"

#define RTC_TASK_PERIOD_MS 10

#define RTC_CONNECTED_CHECK_PERIOD_MS 10000

#define BATTERY_MEASUREMENT_PERIOD_MS 60000
#define BATTERY_CURVE_STEP_PERCENT 10
#define VOLTAGE_DIVIDER_COEFFICIENT 2

#define PCF8523_CURRENT_CONSUMPTION_NA 1200
#define CR1220_BATTER_CAPACITANCE_MAH 37
#define BATTERY_DISCHARGE_TIME_PER_PERCENT_IN_MINUTES ((((CR1220_BATTER_CAPACITANCE_MAH * 1000000) / PCF8523_CURRENT_CONSUMPTION_NA) * 60) / 100)
#define BATTERY_LEVEL_LOW_PERCENT 30
#define BATTERY_LEVEL_CRITICAL_LOW_PERCENT 10
#define BATTERY_NOT_DETECTED_THRESHOLD_PERCENT 0

#define HEALTH_FAULT_ID_BATTERY_LOW_WARNING 0x01
#define HEALTH_FAULT_ID_BATTERY_LOW_ERROR 0x02
#define HEALTH_FAULT_ID_RTC_ERROR 0xA1

static const uint16_t cr1220_battery_curve_mv[] = {
    0,    /* 0 % of battery capacity */
    2600, /* 10 % of battery capacity */
    2750, /* 20 % of battery capacity */
    2810, /* 30 % of battery capacity */
    2860, /* 40 % of battery capacity */
    2900, /* 50 % of battery capacity */
    2900, /* 60 % of battery capacity */
    2900, /* 70 % of battery capacity */
    2900, /* 80 % of battery capacity */
    2900, /* 90 % of battery capacity */
    2900, /* 100 % of battery capacity */
};

static struct
{
    uint32_t                   end_time;
    struct Pcf8523Drv_TimeDate set_time;
    bool                       set_time_pending;
} TimeSetParams;

static void LoopRTC(void);
static void OnSecondElapsed(void);
static void MeasureBatteryLevel(void);
static void UpdateBatteryStatus(void);
static void UpdateHealthFaultStatus(void);

static bool                          IsInitialized              = false;
static bool                          IsAvailable                = false;
static volatile bool                 ReceivedTimeGet            = false;
static uint8_t                       LastBatteryLevelPercent    = 0;
static bool                          IsBatteryDetected          = false;
static bool                          IsBatteryLevelEverMeasured = false;
static bool                          IsTimeValid                = false;
static bool                          IsCountingStopped          = false;
static uint8_t *                     pInstanceIndex             = NULL;
static unsigned long                 LastRtcConnectedTimestamp  = 0;
static RTCTimeGetProcessedCallback_T TimeGetProcessedCallback   = NULL;
static RTCTimeSetProcessedCallback_T TimeSetProcessedCallback   = NULL;


void RTC_Init(RTCTimeGetProcessedCallback_T time_get_processed_cbk, RTCTimeSetProcessedCallback_T time_set_processed_cbk, uint8_t *p_instance_idx)
{
    ASSERT((!IsInitialized) && (time_get_processed_cbk != NULL) && (time_set_processed_cbk != NULL));

    TimeGetProcessedCallback = time_get_processed_cbk;
    TimeSetProcessedCallback = time_set_processed_cbk;
    pInstanceIndex           = p_instance_idx;

    if (!AdcHal_IsInitialized())
    {
        AdcHal_Init();
    }

    if (!Pcf8523Drv_Init())
    {
        LOG_E("PCF8523 initialization failure");
        IsInitialized = false;
        return;
    }

    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    GpioHal_PinMode(GPIO_HAL_PIN_RTC_INT1, GPIO_HAL_MODE_INPUT_PULLUP);
    GpioHal_SetPinIrq(GPIO_HAL_PIN_RTC_INT1, GPIO_HAL_IRQ_EDGE_FALLING, OnSecondElapsed);

    SimpleScheduler_TaskAdd(RTC_TASK_PERIOD_MS, LoopRTC, SIMPLE_SCHEDULER_TASK_ID_RTC, false);

    IsTimeValid   = !Pcf8523Drv_IsResetState();
    IsInitialized = true;
    IsAvailable   = true;
}

bool RTC_IsInitialized(void)
{
    return IsInitialized;
}

void RTC_TriggerTimeSet(struct Pcf8523Drv_TimeDate *p_time)
{
    if (!IsInitialized)
    {
        return;
    }

    if (p_time->milliseconds == 0)
    {
        IsCountingStopped = false;
        IsTimeValid       = true;
        Pcf8523Drv_SetTime(p_time);
        TimeSetProcessedCallback();
        return;
    }

    TimeSetParams.end_time = Timestamp_GetCurrent() + (1000 - p_time->milliseconds);

    struct LocalTime local_time;
    local_time.year    = p_time->year;
    local_time.month   = (enum Month)(p_time->month + 1);
    local_time.day     = p_time->day;
    local_time.hour    = p_time->hour;
    local_time.minutes = p_time->minute;
    local_time.seconds = p_time->seconds;

    uint64_t time = TAILocalTimeConverter_LocalTimeToTAI(&local_time, 0, 0);
    time++;
    local_time = TAILocalTimeConverter_TAIToLocalTime(time, 0, 0);

    TimeSetParams.set_time.year         = local_time.year;
    TimeSetParams.set_time.month        = local_time.month - 1;
    TimeSetParams.set_time.day          = local_time.day;
    TimeSetParams.set_time.hour         = local_time.hour;
    TimeSetParams.set_time.minute       = local_time.minutes;
    TimeSetParams.set_time.seconds      = local_time.seconds;
    TimeSetParams.set_time.milliseconds = 0;

    TimeSetParams.set_time_pending = true;
}

void RTC_TriggerTimeGet(void)
{
    if (!IsInitialized)
    {
        return;
    }

    if (!IsTimeValid || IsCountingStopped)
    {
        struct Pcf8523Drv_TimeDate zero_date_time = {0};
        TimeGetProcessedCallback(&zero_date_time);
        return;
    }

    ReceivedTimeGet = true;
}

bool RTC_IsBatteryDetected(void)
{
    MeasureBatteryLevel();

    if (IsBatteryDetected)
    {
        LOG_D("Battery detected");
    }
    else
    {
        LOG_D("Battery not detected");
    }

    return IsBatteryDetected;
}

void RTC_StopCounting(void)
{
    IsCountingStopped = true;
}

static void LoopRTC(void)
{
    if (!IsInitialized || (*pInstanceIndex == UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN))
    {
        return;
    }

    if ((Timestamp_GetTimeElapsed(LastRtcConnectedTimestamp, Timestamp_GetCurrent()) > RTC_CONNECTED_CHECK_PERIOD_MS) || (LastRtcConnectedTimestamp == 0))
    {
        LastRtcConnectedTimestamp = Timestamp_GetCurrent();
        if (!Pcf8523Drv_IsAvailable() || Pcf8523Drv_IsResetState())
        {
            MCU_Health_SendSetFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_RTC_ERROR, *pInstanceIndex);
            return;
        }
        else
        {
            MCU_Health_SendClearFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_RTC_ERROR, *pInstanceIndex);
        }
    }

    MeasureBatteryLevel();

    if (!TimeSetParams.set_time_pending)
    {
        return;
    }

    if (Timestamp_Compare(TimeSetParams.end_time, Timestamp_GetCurrent()))
    {
        IsCountingStopped = false;
        IsTimeValid       = true;
        Pcf8523Drv_SetTime(&TimeSetParams.set_time);
        TimeSetProcessedCallback();

        TimeSetParams.set_time_pending = false;
    }
}

static void OnSecondElapsed(void)
{
    if (*pInstanceIndex == UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN)
    {
        return;
    }

    if (ReceivedTimeGet)
    {
        struct Pcf8523Drv_TimeDate date;
        Pcf8523Drv_GetTime(&date);

        if (date.month > 12)
        {
            // In case of connection error with RTC the library returns the month equal to 165.
            // All the other data is also invalid
            LOG_W("RTC connection error");
            MCU_Health_SendSetFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_RTC_ERROR, *pInstanceIndex);
            return;
        }

        MCU_Health_SendClearFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_RTC_ERROR, *pInstanceIndex);
        TimeGetProcessedCallback(&date);
        ReceivedTimeGet = false;
    }
}

static void MeasureBatteryLevel(void)
{
    static unsigned long last_measurement_timestamp = 0;

    if (IsBatteryLevelEverMeasured && !IsBatteryDetected)
    {
        return;
    }

    if ((Timestamp_GetTimeElapsed(last_measurement_timestamp, Timestamp_GetCurrent()) > BATTERY_MEASUREMENT_PERIOD_MS) || (last_measurement_timestamp == 0))
    {
        uint16_t battery_voltage_mv = AdcHal_ReadChannelMv(ADC_HAL_CHANNEL_RTC_BATTERY, VOLTAGE_DIVIDER_COEFFICIENT);

        LastBatteryLevelPercent = 100;
        size_t i;
        for (i = 0; i < ARRAY_SIZE(cr1220_battery_curve_mv) - 1; i++)
        {
            if (battery_voltage_mv < cr1220_battery_curve_mv[i])
            {
                LastBatteryLevelPercent = (i - 1) * BATTERY_CURVE_STEP_PERCENT;
                break;
            }
        }

        if (IsBatteryDetected)
        {
            UpdateBatteryStatus();
            UpdateHealthFaultStatus();
        }
        LOG_D("RTC battery voltage: %d mV (%d%%)", battery_voltage_mv, LastBatteryLevelPercent);

        last_measurement_timestamp = Timestamp_GetCurrent();

        if (!IsBatteryLevelEverMeasured && (LastBatteryLevelPercent > BATTERY_NOT_DETECTED_THRESHOLD_PERCENT))
        {
            IsBatteryDetected = true;
        }
        IsBatteryLevelEverMeasured = true;
    }
}

static void UpdateBatteryStatus(void)
{
    uint32_t time_to_discharge_minutes = LastBatteryLevelPercent * BATTERY_DISCHARGE_TIME_PER_PERCENT_IN_MINUTES;

    uint8_t battery_flags = BATTERY_FLAGS_PRESENCE_PRESENT_AND_REMOVABLE | BATTERY_FLAGS_CHARGING_IS_NOT_CHARGEABLE;
    if (LastBatteryLevelPercent <= BATTERY_LEVEL_CRITICAL_LOW_PERCENT)
    {
        battery_flags |= BATTERY_FLAGS_INDICATOR_CRITICALLY_LOW_LEVEL;
        battery_flags |= BATTERY_FLAGS_SERVICEABILITY_BATTERY_REQUIRES_SERVICE;
    }
    else if (LastBatteryLevelPercent <= BATTERY_LEVEL_LOW_PERCENT)
    {
        battery_flags |= BATTERY_FLAGS_INDICATOR_LOW_LEVEL;
        battery_flags |= BATTERY_FLAGS_SERVICEABILITY_BATTERY_REQUIRES_SERVICE;
    }
    else
    {
        battery_flags |= BATTERY_FLAGS_INDICATOR_GOOD_LEVEL;
        battery_flags |= BATTERY_FLAGS_SERVICEABILITY_BATTERY_DOES_NOT_REQUIRE_SERVICE;
    }

    uint8_t payload[] = {
        *pInstanceIndex,
        LastBatteryLevelPercent,
        ((uint8_t)(time_to_discharge_minutes & 0xFF)),
        ((uint8_t)((time_to_discharge_minutes >> 8) & 0xFF)),
        ((uint8_t)((time_to_discharge_minutes >> 16) & 0xFF)),
        (BATTERY_TIME_TO_CHARGE_UNKNOWN & 0xFF),
        ((BATTERY_TIME_TO_CHARGE_UNKNOWN >> 8) & 0xFF),
        ((BATTERY_TIME_TO_CHARGE_UNKNOWN >> 16) & 0xFF),
        battery_flags,
    };

    UartProtocol_Send(UART_FRAME_CMD_BATTERY_STATUS_SET_REQ, payload, sizeof(payload));
}

static void UpdateHealthFaultStatus(void)
{
    if (LastBatteryLevelPercent <= BATTERY_LEVEL_CRITICAL_LOW_PERCENT)
    {
        MCU_Health_SendSetFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_BATTERY_LOW_WARNING, *pInstanceIndex);
        MCU_Health_SendSetFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_BATTERY_LOW_ERROR, *pInstanceIndex);
    }
    else if (LastBatteryLevelPercent <= BATTERY_LEVEL_LOW_PERCENT)
    {
        MCU_Health_SendSetFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_BATTERY_LOW_WARNING, *pInstanceIndex);
        MCU_Health_SendClearFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_BATTERY_LOW_ERROR, *pInstanceIndex);
    }
    else
    {
        MCU_Health_SendClearFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_BATTERY_LOW_WARNING, *pInstanceIndex);
        MCU_Health_SendClearFaultRequest(SILVAIR_ID, HEALTH_FAULT_ID_BATTERY_LOW_ERROR, *pInstanceIndex);
    }
}
