#include "Attention.h"

#include <stddef.h>
#include <stdint.h>

#include "Assert.h"
#include "GpioHal.h"
#include "Log.h"
#include "Luminaire.h"
#include "SimpleScheduler.h"
#include "UartProtocol.h"

#define ATTENTION_ON_OFF_PERIOD_MS 500
#define ATTENTION_TASK_PERIOD_MS ATTENTION_ON_OFF_PERIOD_MS

static void Attention_UartMessageHandler(struct UartFrameRxTxFrame *p_frame);
static void Attention_AttentionEvent(struct UartProtocolFrameAttentionEvent *p_rx_frame);
static void Attention_Loop(void);

static bool IsInitialized     = false;
static bool AttentionLedValue = false;

static const enum UartFrameCmd UartFrameCommandList[] = {UART_FRAME_CMD_ATTENTION_EVENT};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = Attention_UartMessageHandler,
    .p_mesh_message_handler       = NULL,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = NULL,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = 0,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

void Attention_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("Attention initialization");

    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    if (!UartProtocol_IsInitialized())
    {
        UartProtocol_Init();
    }

    GpioHal_PinMode(GPIO_HAL_PIN_LED_STATUS, GPIO_HAL_MODE_OUTPUT);

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    SimpleScheduler_TaskAdd(ATTENTION_TASK_PERIOD_MS, Attention_Loop, SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);

    IsInitialized = true;
}

bool Attention_IsInitialized(void)
{
    return IsInitialized;
}

void Attention_StateChange(bool is_enable)
{
    AttentionLedValue = !is_enable;

    // Status LED start and stop attention always in off state
    GpioHal_PinSet(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, is_enable);

    if (Luminaire_IsInitialized())
    {
        // Call this function at every state change
        Luminaire_IndicateAttention(is_enable, AttentionLedValue);
    }
}

static void Attention_UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_ATTENTION_EVENT:
            Attention_AttentionEvent((struct UartProtocolFrameAttentionEvent *)p_frame);
            break;

        default:
            break;
    }
}

static void Attention_AttentionEvent(struct UartProtocolFrameAttentionEvent *p_rx_frame)
{
    if (p_rx_frame->len != (sizeof(struct UartProtocolFrameAttentionEvent) - UART_PROTOCOL_FRAME_HEADER_LEN))
    {
        return;
    }

    LOG_D("Attention event: %u", p_rx_frame->attention);

    if (p_rx_frame->attention == UART_PROTOCOL_ATTENTION_EVENT_ON)
    {
        Attention_StateChange(true);
        return;
    }

    Attention_StateChange(false);
}

static void Attention_Loop(void)
{
    AttentionLedValue = !AttentionLedValue;
    GpioHal_PinSet(GPIO_HAL_PIN_LED_STATUS, AttentionLedValue);

    if (Luminaire_IsInitialized())
    {
        // Call this function at every state change
        Luminaire_IndicateAttention(true, AttentionLedValue);
    }
}
