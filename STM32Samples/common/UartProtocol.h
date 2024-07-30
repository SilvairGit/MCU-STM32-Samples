#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "UartFrame.h"
#include "UartProtocolTypes.h"

#define UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN UINT8_MAX

typedef void (*UartProtocolUartMessageHandler_T)(struct UartFrameRxTxFrame *p_frame);
typedef void (*UartProtocolMeshMessageHandler_T)(struct UartProtocolFrameMeshMessageFrame *p_frame);

struct UartProtocolHandlerConfig
{
    UartProtocolUartMessageHandler_T p_uart_message_handler;
    UartProtocolMeshMessageHandler_T p_mesh_message_handler;

    const enum UartFrameCmd *p_uart_frame_command_list;
    const uint32_t          *p_mesh_message_opcode_list;

    uint8_t uart_frame_command_list_len;
    uint8_t mesh_message_opcode_list_len;

    uint8_t instance_index;
};

void UartProtocol_Init(void);

bool UartProtocol_IsInitialized(void);

void UartProtocol_RegisterMessageHandler(struct UartProtocolHandlerConfig *p_config);

void UartProtocol_Send(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len);

void UartProtocol_SendFrame(struct UartFrameRxTxFrame *p_frame);

void UartProtocol_Flush(void);

#endif
