#include "TimeSource.h"

#include "Assert.h"
#include "Log.h"
#include "ModelManager.h"
#include "ModelParameters.h"
#include "RTC.h"
#include "TimeReceiver.h"
#include "UartFrame.h"
#include "UartProtocol.h"

#define FEBRUARY_MONTH 2

static void ProcessTimeSourceGetRequest(uint8_t *p_payload, uint8_t len);
static void ProcessTimeSourceSetRequest(uint8_t *p_payload, uint8_t len);
static void TimeSourceGetProcessed(struct Pcf8523Drv_TimeDate *p_time);
static void TimeSourceSetProcessed(void);

static bool IsLeapYear(uint16_t year);
static bool ValidateMonthDay(struct Pcf8523Drv_TimeDate *time_date);
static bool ValidateTimeValuesRange(struct Pcf8523Drv_TimeDate *time_date);
static bool IsTimeValuesZero(struct Pcf8523Drv_TimeDate *time_date);


static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame);

static const enum UartFrameCmd UartFrameCommandList[] = {
    UART_FRAME_CMD_TIME_SOURCE_SET_REQ,
    UART_FRAME_CMD_TIME_SOURCE_GET_REQ,
};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = UartMessageHandler,
    .p_mesh_message_handler       = NULL,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = NULL,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = 0,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

static const struct ModelParametersTimeServer time_with_battery_registration_parameters = {
    .flags        = RTC_WITH_BATTERY_ATTACHED,
    .rtc_accuracy = RTC_TIME_ACCURACY_PPB,
};

static const struct ModelParametersTimeServer time_without_battery_registration_parameters = {
    .flags        = RTC_WITHOUT_BATTERY_ATTACHED,
    .rtc_accuracy = RTC_TIME_ACCURACY_PPB,
};

static const struct ModelParametersTimeServer time_without_rtc_registration_parameters = {
    .flags        = RTC_NOT_ATTACHED,
    .rtc_accuracy = 0,
};

struct ModelManagerRegistrationRow ModelConfigTimeServer = {
    .model_id            = MODEL_MANAGER_ID_TIME_SERVER,
    .p_model_parameter   = (uint8_t *)&time_without_rtc_registration_parameters,
    .model_parameter_len = sizeof(time_without_rtc_registration_parameters),
    .p_instance_index    = &MessageHandlerConfig.instance_index,
};

static const uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void TimeSource_Init(void)
{
    RTC_Init(TimeSourceGetProcessed, TimeSourceSetProcessed, &MessageHandlerConfig.instance_index);

    TimeReceiver_Init(&MessageHandlerConfig.instance_index);

    //Update Time server, according to RTC status
    if (RTC_IsInitialized() && RTC_IsBatteryDetected())
    {
        ModelConfigTimeServer.p_model_parameter   = (uint8_t *)&time_with_battery_registration_parameters;
        ModelConfigTimeServer.model_parameter_len = sizeof(time_with_battery_registration_parameters);
    }
    else if (RTC_IsInitialized())
    {
        ModelConfigTimeServer.p_model_parameter   = (uint8_t *)&time_without_battery_registration_parameters;
        ModelConfigTimeServer.model_parameter_len = sizeof(time_without_battery_registration_parameters);
    }

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);
    ModelManager_RegisterModel(&ModelConfigTimeServer);
}

static void ProcessTimeSourceGetRequest(uint8_t *p_payload, uint8_t len)
{
    if ((p_payload == NULL) || (len != sizeof(struct TimeSourceGetReq)))
    {
        return;
    }

    RTC_TriggerTimeGet();
}

static void ProcessTimeSourceSetRequest(uint8_t *p_payload, uint8_t len)
{
    if ((p_payload == NULL) || (len != sizeof(struct TimeSourceSetReq)))
    {
        LOG_D("Time source set request - invalid payload");
        return;
    }

    struct TimeSourceSetReq *msg = (struct TimeSourceSetReq *)p_payload;

    if (IsTimeValuesZero(&msg->date))
    {
        LOG_D("Time source set request - stop counting");
        RTC_StopCounting();
        return;
    }

    if (!ValidateTimeValuesRange(&msg->date) || !ValidateMonthDay(&msg->date))
    {
        LOG_D("Time source set request: invalid time values");
        return;
    }
    LOG_D("Time source set request - trigger time set");
    RTC_TriggerTimeSet(&msg->date);
}

static void TimeSourceGetProcessed(struct Pcf8523Drv_TimeDate *p_time)
{
    struct TimeSourceGetResp msg = {0};

    msg.instance_index = MessageHandlerConfig.instance_index;
    msg.date           = *p_time;

    UartProtocol_Send(UART_FRAME_CMD_TIME_SOURCE_GET_RESP, (uint8_t *)&msg, sizeof(msg));
}

static void TimeSourceSetProcessed(void)
{
    struct TimeSourceSetResp msg = {0};

    msg.instance_index = MessageHandlerConfig.instance_index;

    LOG_D("Time Source Set Processed - send response");
    UartProtocol_Send(UART_FRAME_CMD_TIME_SOURCE_SET_RESP, (uint8_t *)&msg, sizeof(msg));
}

static bool IsLeapYear(uint16_t year)
{
    if (year % 4 != 0)
    {
        return false;
    }
    if (year % 100 != 0)
    {
        return true;
    }
    if (year % 400 != 0)
    {
        return false;
    }
    return true;
}

static bool ValidateMonthDay(struct Pcf8523Drv_TimeDate *time_date)
{
    if (time_date->month != FEBRUARY_MONTH)
    {
        return time_date->day <= days_in_month[time_date->month];
    }

    uint8_t num_days_feb = days_in_month[FEBRUARY_MONTH];

    if (IsLeapYear(time_date->year))
    {
        num_days_feb += 1;
    }

    return time_date->day <= num_days_feb;
}

static bool ValidateTimeValuesRange(struct Pcf8523Drv_TimeDate *time_date)
{
    return ((time_date->year <= 36841) && (time_date->year > 0) && (time_date->month <= 12) && (time_date->month > 0) && (time_date->day <= 31) &&
            (time_date->day > 0) && (time_date->hour <= 23) && (time_date->minute <= 59) && (time_date->seconds <= 59) && (time_date->milliseconds <= 999));
}

static bool IsTimeValuesZero(struct Pcf8523Drv_TimeDate *time_date)
{
    return ((time_date->year == 0) && (time_date->month == 0) && (time_date->day == 0) && (time_date->hour == 0) && (time_date->minute == 0) &&
            (time_date->seconds == 0) && (time_date->milliseconds == 0));
}

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_TIME_SOURCE_SET_REQ:
            ProcessTimeSourceSetRequest(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_TIME_SOURCE_GET_REQ:
            ProcessTimeSourceGetRequest(p_frame->p_payload, p_frame->len);
            break;

        default:
            break;
    }
}
