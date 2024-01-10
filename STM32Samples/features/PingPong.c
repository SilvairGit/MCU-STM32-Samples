#include "PingPong.h"

#include <stddef.h>

#include "Assert.h"
#include "Log.h"
#include "UartProtocol.h"
#include "Utils.h"

static void PingPong_UartMessageHandler(struct UartFrameRxTxFrame *p_frame);
static void PingPong_SendPongResponse(struct UartProtocolFramePingRequest *p_rx_frame);

static bool IsInitialized = false;

static bool PingPongIsEnable = true;

static const enum UartFrameCmd UartFrameCommandList[] = {UART_FRAME_CMD_PING_REQUEST};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = PingPong_UartMessageHandler,
    .p_mesh_message_handler       = NULL,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = NULL,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = 0,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

void PingPong_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("PingPong initialization");

    if (!UartProtocol_IsInitialized())
    {
        UartProtocol_Init();
    }

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    IsInitialized = true;
}

bool PingPong_IsInitialized(void)
{
    return IsInitialized;
}

void PingPong_Enable(bool is_enable)
{
    LOG_D("PingPong enable: %d", is_enable);

    PingPongIsEnable = is_enable;
}

static void PingPong_UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_PING_REQUEST:
            PingPong_SendPongResponse((struct UartProtocolFramePingRequest *)p_frame);
            break;

        default:
            break;
    }
}

static void PingPong_SendPongResponse(struct UartProtocolFramePingRequest *p_rx_frame)
{
    if (PingPongIsEnable)
    {
        UartProtocol_Send(UART_FRAME_CMD_PONG_RESPONSE, p_rx_frame->p_data, p_rx_frame->len);
    }
}
