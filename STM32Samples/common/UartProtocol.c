#include "UartProtocol.h"

#include <stddef.h>

#include "Assert.h"
#include "Log.h"
#include "Mesh.h"
#include "SimpleScheduler.h"
#include "Timestamp.h"
#include "Utils.h"

#define UART_PROTOCOL_FRAME_TIMEOUT_ELAPSED_MS 150

#define UART_PROTOCOL_TASK_PERIOD_MS 0

#define UART_PROTOCOL_MAX_NUMBER_OF_HANDLERS 16

#define UART_PROTOCOL_INVALID_MESH_OPCODE 0
#define UART_PROTOCOL_CMD_AND_LEN_SIZE 2

#define UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK 0xC0
#define UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK 0x80
#define UART_PROTOCOL_MESH_OPCODE_SIZE_RFU_MASK 0x7F
#define UART_PROTOCOL_MESH_OPCODE_SIZE_1_OCTET_MASK 0x00

static bool IsInitialized = false;

static struct UartProtocolHandlerConfig *HandlerConfig[UART_PROTOCOL_MAX_NUMBER_OF_HANDLERS];
static uint8_t                           HandlerConfigCnt = 0;

static void    UartProtocol_ProcessIncomingData(void);
static bool    UartProtocol_ParseMeshMessageRequest(struct UartFrameRxTxFrame *p_rx_frame, struct UartProtocolFrameMeshMessageFrame *p_mesh_message_frame);
static uint8_t UartProtocol_CheckIfInstanceIndexExist(struct UartFrameRxTxFrame *p_rx_frame);
static void    UartProtocol_CallAllUartCommandHandlers(struct UartProtocolHandlerConfig *p_handler_config_row, struct UartFrameRxTxFrame *p_rx_frame);
static void    UartProtocol_CallAllMeshHandlers(struct UartProtocolHandlerConfig         *p_handler_config_row,
                                                struct UartProtocolFrameMeshMessageFrame *p_mesh_message_frame);

void UartProtocol_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("UartProtocol initialization");

    Timestamp_DelayMs(UART_PROTOCOL_FRAME_TIMEOUT_ELAPSED_MS);

    // This delay is added just to keep consistent behavior of older and current MCU FW versions.
    // Older MCU FW versions had delay at startup equal to 1000 ms for uart protocol timeout.
    // Please remove this delay after task SP-10932 is closed.
    Timestamp_DelayMs(1000 - UART_PROTOCOL_FRAME_TIMEOUT_ELAPSED_MS);

    if (!UartFrame_IsInitialized())
    {
        UartFrame_Init();
    }

    SimpleScheduler_TaskAdd(UART_PROTOCOL_TASK_PERIOD_MS, UartProtocol_ProcessIncomingData, SIMPLE_SCHEDULER_TASK_ID_UART_PROTOCOL, true);

    IsInitialized = true;
}

bool UartProtocol_IsInitialized(void)
{
    return IsInitialized;
}

void UartProtocol_RegisterMessageHandler(struct UartProtocolHandlerConfig *p_config)
{
    ASSERT((p_config != NULL) && (HandlerConfigCnt < UART_PROTOCOL_MAX_NUMBER_OF_HANDLERS));

    HandlerConfig[HandlerConfigCnt] = p_config;
    HandlerConfigCnt++;
}

void UartProtocol_Send(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len)
{
    UartFrame_Send(cmd, p_payload, len);
}

void UartProtocol_SendFrame(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    UartFrame_Send(p_frame->cmd, p_frame->p_payload, p_frame->len);
}

void UartProtocol_Flush(void)
{
    UartFrame_Flush();
}

static void UartProtocol_ProcessIncomingData(void)
{
    // This structure must be aligned to avoid pointer misalignment after casting
    static struct UartFrameRxTxFrame rx_frame ALIGN(4);

    if (!UartFrame_ProcessIncomingData(&rx_frame))
    {
        return;
    }

    uint8_t instance_index = UartProtocol_CheckIfInstanceIndexExist(&rx_frame);

    struct UartProtocolFrameMeshMessageFrame mesh_message_frame = {0};

    bool is_mesh_message_frame_valid = UartProtocol_ParseMeshMessageRequest(&rx_frame, &mesh_message_frame);

    size_t i;
    for (i = 0; i < HandlerConfigCnt; i++)
    {
        // Filter instance index only if configured instance index is known
        if ((HandlerConfig[i]->instance_index != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN) && (instance_index != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN) &&
            (HandlerConfig[i]->instance_index != instance_index))
        {
            continue;
        }

        if (is_mesh_message_frame_valid)
        {
            UartProtocol_CallAllMeshHandlers(HandlerConfig[i], &mesh_message_frame);
            continue;
        }

        UartProtocol_CallAllUartCommandHandlers(HandlerConfig[i], &rx_frame);
    }
}

static bool UartProtocol_ParseMeshMessageRequest(struct UartFrameRxTxFrame *p_rx_frame, struct UartProtocolFrameMeshMessageFrame *p_mesh_message_frame)
{
    if (p_rx_frame->cmd == UART_FRAME_CMD_MESH_MESSAGE_REQUEST)
    {
        struct UartProtocolFrameMeshMessageRequest *p_rx_mmr_frame = (struct UartProtocolFrameMeshMessageRequest *)p_rx_frame;

        p_mesh_message_frame->instance_index     = p_rx_mmr_frame->instance_index;
        p_mesh_message_frame->sub_index          = p_rx_mmr_frame->sub_index;
        p_mesh_message_frame->mesh_opcode        = p_rx_mmr_frame->mesh_opcode;
        p_mesh_message_frame->mesh_msg_len       = p_rx_frame->len - sizeof(struct UartProtocolFrameMeshMessageRequest) + UART_PROTOCOL_CMD_AND_LEN_SIZE;
        p_mesh_message_frame->p_mesh_msg_payload = p_rx_mmr_frame->p_data;

        return true;
    }

    if (p_rx_frame->cmd == UART_FRAME_CMD_MESH_MESSAGE_REQUEST1)
    {
        struct UartProtocolFrameMeshMessageRequest1 *p_rx_mmr_frame = (struct UartProtocolFrameMeshMessageRequest1 *)p_rx_frame;

        p_mesh_message_frame->instance_index = p_rx_mmr_frame->instance_index;
        p_mesh_message_frame->sub_index      = p_rx_mmr_frame->sub_index;

        if (p_rx_mmr_frame->p_data[0] == UART_PROTOCOL_MESH_OPCODE_SIZE_RFU_MASK)
        {
            return false;
        }

        if ((p_rx_mmr_frame->p_data[0] & UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK) == UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK)
        {
            struct UartProtocolFrameMeshMessageRequest1Opcode3B *p_rx_mmr_frame1 = (struct UartProtocolFrameMeshMessageRequest1Opcode3B *)p_rx_frame;

            // Big to little endian conversion
            p_mesh_message_frame->mesh_opcode  = SWAP_3_BYTES(p_rx_mmr_frame1->mesh_opcode_be);
            p_mesh_message_frame->mesh_msg_len = p_rx_frame->len - sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode3B) + UART_PROTOCOL_CMD_AND_LEN_SIZE;
            p_mesh_message_frame->p_mesh_msg_payload = p_rx_mmr_frame1->p_data;

            return true;
        }

        if ((p_rx_mmr_frame->p_data[0] & UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK) == UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK)
        {
            struct UartProtocolFrameMeshMessageRequest1Opcode2B *p_rx_mmr_frame1 = (struct UartProtocolFrameMeshMessageRequest1Opcode2B *)p_rx_frame;

            // Big to little endian conversion
            p_mesh_message_frame->mesh_opcode  = SWAP_2_BYTES(p_rx_mmr_frame1->mesh_opcode_be);
            p_mesh_message_frame->mesh_msg_len = p_rx_frame->len - sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode2B) + UART_PROTOCOL_CMD_AND_LEN_SIZE;
            p_mesh_message_frame->p_mesh_msg_payload = p_rx_mmr_frame1->p_data;

            return true;
        }

        struct UartProtocolFrameMeshMessageRequest1Opcode1B *p_rx_mmr_frame1 = (struct UartProtocolFrameMeshMessageRequest1Opcode1B *)p_rx_frame;

        // Big endian - 1 byte don't require conversion
        p_mesh_message_frame->mesh_opcode  = p_rx_mmr_frame1->mesh_opcode;
        p_mesh_message_frame->mesh_msg_len = p_rx_frame->len - sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode1B) + UART_PROTOCOL_CMD_AND_LEN_SIZE;
        p_mesh_message_frame->p_mesh_msg_payload = p_rx_mmr_frame1->p_data;

        return true;
    }

    return false;
}

static uint8_t UartProtocol_CheckIfInstanceIndexExist(struct UartFrameRxTxFrame *p_rx_frame)
{
    switch (p_rx_frame->cmd)
    {
        case UART_FRAME_CMD_MESH_MESSAGE_REQUEST:
            return ((struct UartProtocolFrameMeshMessageRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_MESH_MESSAGE_RESPONSE:
            return ((struct UartProtocolFrameMeshMessageResponse *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_SENSOR_UPDATE_REQUEST:
            return ((struct UartProtocolFrameSensorUpdateRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_SET_FAULT_REQUEST:
            return ((struct UartProtocolFrameSetFaultRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_CLEAR_FAULT_REQUEST:
            return ((struct UartProtocolFrameClearFaultRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_START_TEST_REQ:
            return ((struct UartProtocolFrameStartTestRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TEST_FINISHED_REQ:
            return ((struct UartProtocolFrameTestFinishedRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_BATTERY_STATUS_SET_REQ:
            return ((struct UartProtocolFrameBatteryStatusSetRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_MESH_MESSAGE_REQUEST1:
            return ((struct UartProtocolFrameMeshMessageRequest1 *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TIME_SOURCE_SET_REQ:
            return ((struct UartProtocolFrameTimeSourceSetRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TIME_SOURCE_SET_RESP:
            return ((struct UartProtocolFrameTimeSourceSetResponse *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TIME_SOURCE_GET_REQ:
            return ((struct UartProtocolFrameTimeSourceGetRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TIME_SOURCE_GET_RESP:
            return ((struct UartProtocolFrameTimeGetRequest *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TIME_GET_REQ:
            return ((struct UartProtocolFrameTimeSourceGetResponse *)p_rx_frame)->instance_index;

        case UART_FRAME_CMD_TIME_GET_RESP:
            return ((struct UartProtocolFrameTimeGetResponse *)p_rx_frame)->instance_index;

        default:
            return UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
    }
}

static void UartProtocol_CallAllUartCommandHandlers(struct UartProtocolHandlerConfig *p_handler_config_row, struct UartFrameRxTxFrame *p_rx_frame)
{
    if ((p_handler_config_row->p_uart_frame_command_list == NULL) || (p_handler_config_row->p_uart_message_handler == NULL))
    {
        return;
    }

    size_t k;
    for (k = 0; k < p_handler_config_row->uart_frame_command_list_len; k++)
    {
        if (p_rx_frame->cmd == p_handler_config_row->p_uart_frame_command_list[k])
        {
            p_handler_config_row->p_uart_message_handler(p_rx_frame);
        }
    }
}

static void UartProtocol_CallAllMeshHandlers(struct UartProtocolHandlerConfig         *p_handler_config_row,
                                             struct UartProtocolFrameMeshMessageFrame *p_mesh_message_frame)
{
    if ((p_handler_config_row->p_mesh_message_opcode_list == NULL) || (p_handler_config_row->p_mesh_message_handler == NULL))
    {
        return;
    }

    size_t k;
    for (k = 0; k < p_handler_config_row->mesh_message_opcode_list_len; k++)
    {
        if (p_mesh_message_frame->mesh_opcode == p_handler_config_row->p_mesh_message_opcode_list[k])
        {
            p_handler_config_row->p_mesh_message_handler(p_mesh_message_frame);
        }
    }
}
