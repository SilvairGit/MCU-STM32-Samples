#include "MCU_Health.h"

#include <limits.h>
#include <string.h>

#include "Assert.h"
#include "Config.h"
#include "GpioHal.h"
#include "Log.h"
#include "Mesh.h"
#include "ModelManager.h"
#include "PingPong.h"
#include "SimpleScheduler.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Utils.h"

#define HEALTH_TASK_PERIOD_MS 10

#define PB_FAULT GPIO_HAL_PIN_SW1      /**< Defines Fault (used to Set and Clear faults) button location. */
#define PB_CONNECTION GPIO_HAL_PIN_SW2 /**< Defines Connection (used to disconnect and connect UART) button location. */

#define HEALTH_MESSAGE_SET_FAULT_LEN 4
#define HEALTH_MESSAGE_CLEAR_FAULT_LEN 4
#define TEST_MSG_LEN 4

#define EXAMPLE_FAULT_ID 0x01u

#define BUTTON_DEBOUNCE_TIME_MS 20 /**< Defines buttons debounce time in milliseconds. */
#define TEST_TIME_MS 1500          /**< Defines fake test duration in milliseconds. */

static void LoopHealth(void);
static void ProcessStartTest(uint8_t *p_payload, uint8_t len);
static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame);

static const enum UartFrameCmd UartFrameCommandList[] = {UART_FRAME_CMD_START_TEST_REQ};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = UartMessageHandler,
    .p_mesh_message_handler       = NULL,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = NULL,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = 0,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

static const struct ModelParametersHealthServer1Id health_registration_parameters = {
    .number_of_company_ids = 0x01,    //Number of company IDs
    .company_id_list       = {SILVAIR_ID},
};

struct ModelManagerRegistrationRow ModelConfigHealthServer = {
    .model_id            = MODEL_MANAGER_ID_HEALTH_SERVER,
    .p_model_parameter   = (uint8_t *)&health_registration_parameters,
    .model_parameter_len = sizeof(health_registration_parameters),
    .p_instance_index    = &MessageHandlerConfig.instance_index,
};

static bool     TestStarted        = false;     /**  True, if test is started. */
static uint32_t TestStartTimestamp = 0;         /**  Time left to finish test.*/
static uint8_t  TestStartPayload[TEST_MSG_LEN]; /**  Current test payload.*/

void MCU_Health_Setup(void)
{
    LOG_D("Health initialization");

    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    GpioHal_PinMode(GPIO_HAL_PIN_LED_STATUS, GPIO_HAL_MODE_OUTPUT);

#if MCU_SERVER
    ModelManager_RegisterModel(&ModelConfigHealthServer);
#endif

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    SimpleScheduler_TaskAdd(HEALTH_TASK_PERIOD_MS, LoopHealth, SIMPLE_SCHEDULER_TASK_ID_HEALTH, true);
}

void MCU_Health_SendSetFaultRequest(uint16_t company_id, uint8_t fault_id, uint8_t instance_idx)
{
    uint8_t buf[HEALTH_MESSAGE_SET_FAULT_LEN];
    size_t  index = 0;

    buf[index++] = LOW_BYTE(company_id);
    buf[index++] = HIGH_BYTE(company_id);
    buf[index++] = fault_id;
    buf[index++] = instance_idx;

    UartProtocol_Send(UART_FRAME_CMD_SET_FAULT_REQUEST, buf, sizeof(buf));
}

void MCU_Health_SendClearFaultRequest(uint16_t company_id, uint8_t fault_id, uint8_t instance_idx)
{
    uint8_t buf[HEALTH_MESSAGE_CLEAR_FAULT_LEN];
    size_t  index = 0;

    buf[index++] = LOW_BYTE(company_id);
    buf[index++] = HIGH_BYTE(company_id);
    buf[index++] = fault_id;
    buf[index++] = instance_idx;

    UartProtocol_Send(UART_FRAME_CMD_CLEAR_FAULT_REQUEST, buf, sizeof(buf));
}

bool MCU_Health_IsTestInProgress(void)
{
    return TestStarted;
}

static void LoopHealth(void)
{
    if (MCU_Health_IsTestInProgress())
    {
        if (Timestamp_GetTimeElapsed(TestStartTimestamp, Timestamp_GetCurrent()) >= TEST_TIME_MS)
        {
            TestStarted = false;
            GpioHal_PinSet(GPIO_HAL_PIN_LED_STATUS, false);
            UartProtocol_Send(UART_FRAME_CMD_TEST_FINISHED_REQ, TestStartPayload, TEST_MSG_LEN);
        }
    }
}

static void ProcessStartTest(uint8_t *p_payload, uint8_t len)
{
    UartProtocol_Send(UART_FRAME_CMD_START_TEST_RESP, NULL, 0);
    GpioHal_PinSet(GPIO_HAL_PIN_LED_STATUS, true);
    memcpy(TestStartPayload, p_payload, len);
    TestStarted        = true;
    TestStartTimestamp = Timestamp_GetCurrent();
}

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_START_TEST_REQ:
            ProcessStartTest(p_frame->p_payload, p_frame->len);
            break;

        default:
            break;
    }
}
