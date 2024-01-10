#include "Provisioning.h"

#include <stddef.h>
#include <string.h>

#include "Assert.h"
#include "Attention.h"
#include "EmgLTest.h"
#include "LCD.h"
#include "Log.h"
#include "Luminaire.h"
#include "MCU_Definitions.h"
#include "MCU_Health.h"
#include "MeshSensorGenerator.h"
#include "ModelManager.h"
#include "RTC.h"
#include "SimpleScheduler.h"
#include "Switch.h"
#include "UartFrame.h"
#include "UartProtocol.h"

static void EnableTasks(bool enable);
static void ProcessEnterInitDevice(struct UartFrameRxTxFrame *p_frame);
static void ProcessEnterDevice(uint8_t *p_payload, uint8_t len);
static void ProcessEnterInitNode(uint8_t *p_payload, uint8_t len);
static void ProcessEnterNode(uint8_t *p_payload, uint8_t len);
static void ProcessError(uint8_t *p_payload, uint8_t len);
static void ProcessModemFirmwareVersion(uint8_t *p_payload, uint8_t len);
static void SendFirmwareVersionSetRequest(void);
static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame);

static const enum UartFrameCmd UartFrameCommandList[] = {
    UART_FRAME_CMD_INIT_DEVICE_EVENT,
    UART_FRAME_CMD_CREATE_INSTANCES_RESPONSE,
    UART_FRAME_CMD_INIT_NODE_EVENT,
    UART_FRAME_CMD_START_NODE_RESPONSE,
    UART_FRAME_CMD_MESH_MESSAGE_REQUEST,
    UART_FRAME_CMD_ERROR,
    UART_FRAME_CMD_MODEM_FIRMWARE_VERSION_RESPONSE,
    UART_FRAME_CMD_FIRMWARE_VERSION_SET_RESP,
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

static enum ModemState ModemState    = MODEM_STATE_UNKNOWN;
static bool            IsInitialized = false;

void Provisioning_Init(void)
{
    ASSERT(!IsInitialized);

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    IsInitialized = true;
}

bool Provisioning_IsInitialized(void)
{
    return IsInitialized;
}

static void EnableTasks(bool enable)
{
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_RTC, enable);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_SWITCH, enable);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_TIME_SYNC, enable);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_SENSOR_INPUT, enable);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_ENERGY_SENSOR_SIMULATOR, enable);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_EMG_L_TEST, enable);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_EMERGENCY_DRIVER_SIMULATOR, enable);
}

static void ProcessEnterInitDevice(struct UartFrameRxTxFrame *p_frame)
{
    LOG_D("Init Device State");
    ModemState = MODEM_STATE_INIT_DEVICE;
    LCD_UpdateModemState(ModemState);
    Attention_StateChange(false);

    size_t payload_len = ModelManager_GetCreateInstanceIndexPayloadLen();

    struct UartProtocolFrameInitDeviceEvent *p_init_device_event_frame = (struct UartProtocolFrameInitDeviceEvent *)p_frame;

    // Avoid misaligned pointer
    uint16_t model_id[p_init_device_event_frame->len / sizeof(uint16_t)];
    memcpy(model_id, p_init_device_event_frame->p_model_id, p_init_device_event_frame->len);

    uint8_t model_ids[payload_len];
    size_t  index = ModelManager_CreateInstanceIndexPayload(model_ids, model_id, p_init_device_event_frame->len / sizeof(uint16_t));

    if (index == 0)
    {
        return;
    }

    SendFirmwareVersionSetRequest();
    UartProtocol_Send(UART_FRAME_CMD_MODEM_FIRMWARE_VERSION_REQUEST, NULL, 0);
    UartProtocol_Send(UART_FRAME_CMD_CREATE_INSTANCES_REQUEST, model_ids, index);
}

static void ProcessEnterDevice(uint8_t *p_payload, uint8_t len)
{
    LOG_D("Device State");

    Luminaire_StartStartupBehavior();
    ModemState = MODEM_STATE_DEVICE;
    LCD_UpdateModemState(ModemState);

    EnableTasks(false);
    UNUSED(p_payload);
    UNUSED(len);
}

static void ProcessEnterInitNode(uint8_t *p_payload, uint8_t len)
{
    LOG_D("Init Node State");
    ModemState = MODEM_STATE_INIT_NODE;
    LCD_UpdateModemState(ModemState);
    Attention_StateChange(false);
    Luminaire_StopStartupBehavior();

    ModelManager_ResetAllInstanceIndexes();

    if (len == 0)
    {
        return;
    }

    for (size_t index = 0; index < len;)
    {
        uint16_t model_id = ((uint16_t)p_payload[index++]);
        model_id |= ((uint16_t)p_payload[index++] << 8);
        uint16_t current_model_id_instance_index = index / 2;

        ModelManager_SetInstanceIndex(model_id, current_model_id_instance_index);
    }

    ModelManager_IsAllModelsRegistered();

    SendFirmwareVersionSetRequest();
    UartProtocol_Send(UART_FRAME_CMD_START_NODE_REQUEST, NULL, 0);
    UartProtocol_Send(UART_FRAME_CMD_MODEM_FIRMWARE_VERSION_REQUEST, NULL, 0);
}

static void ProcessEnterNode(uint8_t *p_payload, uint8_t len)
{
    LOG_D("Node State");
    ModemState = MODEM_STATE_NODE;
    LCD_UpdateModemState(ModemState);

    EnableTasks(true);

    UNUSED(p_payload);
    UNUSED(len);
}

static void ProcessError(uint8_t *p_payload, uint8_t len)
{
    LOG_W("Error %d", p_payload[0]);

    UNUSED(len);
}

static void ProcessModemFirmwareVersion(uint8_t *p_payload, uint8_t len)
{
    LOG_D("Process Modem Firmware Version");
    LCD_UpdateModemFwVersion((char *)p_payload, len);
}

static void SendFirmwareVersionSetRequest(void)
{
    const char *p_firmware_version = BUILD_NUMBER;

    UartProtocol_Send(UART_FRAME_CMD_FIRMWARE_VERSION_SET_REQ, (uint8_t *)p_firmware_version, strlen(p_firmware_version));
}

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        // 1 init
        case UART_FRAME_CMD_INIT_DEVICE_EVENT:
        {
            ProcessEnterInitDevice(p_frame);
            break;
        }
        case UART_FRAME_CMD_CREATE_INSTANCES_RESPONSE:
        {
            ProcessEnterDevice(p_frame->p_payload, p_frame->len);
            break;
        }
        case UART_FRAME_CMD_INIT_NODE_EVENT:
        {
            ProcessEnterInitNode(p_frame->p_payload, p_frame->len);
            break;
        }
        case UART_FRAME_CMD_START_NODE_RESPONSE:
        {
            ProcessEnterNode(p_frame->p_payload, p_frame->len);
            break;
        }
        case UART_FRAME_CMD_ERROR:
        {
            ProcessError(p_frame->p_payload, p_frame->len);
            break;
        }
        case UART_FRAME_CMD_MODEM_FIRMWARE_VERSION_RESPONSE:
        {
            ProcessModemFirmwareVersion(p_frame->p_payload, p_frame->len);
            break;
        }

        case UART_FRAME_CMD_FIRMWARE_VERSION_SET_RESP:
        {
            // Do nothing
            break;
        }

        default:
            break;
    }
}
