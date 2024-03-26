#ifndef UART_FRAME_H
#define UART_FRAME_H

#include <stdbool.h>
#include <stdint.h>

#include "Utils.h"

#define UART_FRAME_LOGGER_ENABLE 0

#define UART_FRAME_MAX_PAYLOAD_LEN 127

enum UartFrameCmd
{
    // First range of UART frames command
    UART_FRAME_CMD_RANGE1_START                    = 0x00,
    UART_FRAME_CMD_PROHIBITED                      = 0x00,
    UART_FRAME_CMD_PING_REQUEST                    = 0x01,
    UART_FRAME_CMD_PONG_RESPONSE                   = 0x02,
    UART_FRAME_CMD_INIT_DEVICE_EVENT               = 0x03,
    UART_FRAME_CMD_CREATE_INSTANCES_REQUEST        = 0x04,
    UART_FRAME_CMD_CREATE_INSTANCES_RESPONSE       = 0x05,
    UART_FRAME_CMD_INIT_NODE_EVENT                 = 0x06,
    UART_FRAME_CMD_MESH_MESSAGE_REQUEST            = 0x07,
    UART_FRAME_CMD_NOT_USED_1                      = 0x08,
    UART_FRAME_CMD_START_NODE_REQUEST              = 0x09,
    UART_FRAME_CMD_NOT_USED_2                      = 0x0A,
    UART_FRAME_CMD_START_NODE_RESPONSE             = 0x0B,
    UART_FRAME_CMD_FACTORY_RESET_REQUEST           = 0x0C,
    UART_FRAME_CMD_FACTORY_RESET_RESPONSE          = 0x0D,
    UART_FRAME_CMD_FACTORY_RESET_EVENT             = 0x0E,
    UART_FRAME_CMD_MESH_MESSAGE_RESPONSE           = 0x0F,
    UART_FRAME_CMD_CURRENT_STATE_REQUEST           = 0x10,
    UART_FRAME_CMD_CURRENT_STATE_RESPONSE          = 0x11,
    UART_FRAME_CMD_ERROR                           = 0x12,
    UART_FRAME_CMD_MODEM_FIRMWARE_VERSION_REQUEST  = 0x13,
    UART_FRAME_CMD_MODEM_FIRMWARE_VERSION_RESPONSE = 0x14,
    UART_FRAME_CMD_SENSOR_UPDATE_REQUEST           = 0x15,
    UART_FRAME_CMD_ATTENTION_EVENT                 = 0x16,
    UART_FRAME_CMD_SOFTWARE_RESET_REQUEST          = 0x17,
    UART_FRAME_CMD_SOFTWARE_RESET_RESPONSE         = 0x18,
    UART_FRAME_CMD_SENSOR_UPDATE_RESPONSE          = 0x19,
    UART_FRAME_CMD_DEVICE_UUID_REQUEST             = 0x1A,
    UART_FRAME_CMD_DEVICE_UUID_RESPONSE            = 0x1B,
    UART_FRAME_CMD_SET_FAULT_REQUEST               = 0x1C,
    UART_FRAME_CMD_SET_FAULT_RESPONSE              = 0x1D,
    UART_FRAME_CMD_CLEAR_FAULT_REQUEST             = 0x1E,
    UART_FRAME_CMD_CLEAR_FAULT_RESPONSE            = 0x1F,
    UART_FRAME_CMD_START_TEST_REQ                  = 0x20,
    UART_FRAME_CMD_START_TEST_RESP                 = 0x21,
    UART_FRAME_CMD_TEST_FINISHED_REQ               = 0x22,
    UART_FRAME_CMD_TEST_FINISHED_RESP              = 0x23,
    UART_FRAME_CMD_FIRMWARE_VERSION_SET_REQ        = 0x24,
    UART_FRAME_CMD_FIRMWARE_VERSION_SET_RESP       = 0x25,
    UART_FRAME_CMD_BATTERY_STATUS_SET_REQ          = 0x26,
    UART_FRAME_CMD_BATTERY_STATUS_SET_RESP         = 0x27,
    UART_FRAME_CMD_MESH_MESSAGE_REQUEST1           = 0x28,
    UART_FRAME_CMD_TIME_SOURCE_SET_REQ             = 0x29,
    UART_FRAME_CMD_TIME_SOURCE_SET_RESP            = 0x2A,
    UART_FRAME_CMD_TIME_SOURCE_GET_REQ             = 0x2B,
    UART_FRAME_CMD_TIME_SOURCE_GET_RESP            = 0x2C,
    UART_FRAME_CMD_TIME_GET_REQ                    = 0x2D,
    UART_FRAME_CMD_TIME_GET_RESP                   = 0x2E,
    UART_FRAME_CMD_RANGE1_END                      = 0x2E,

    // Second, DFU range of UART frame commands
    UART_FRAME_CMD_RANGE2_START         = 0x80,
    UART_FRAME_CMD_DFU_INIT_REQ         = 0x80,
    UART_FRAME_CMD_DFU_INIT_RESP        = 0x81,
    UART_FRAME_CMD_DFU_STATUS_REQ       = 0x82,
    UART_FRAME_CMD_DFU_STATUS_RESP      = 0x83,
    UART_FRAME_CMD_DFU_PAGE_CREATE_REQ  = 0x84,
    UART_FRAME_CMD_DFU_PAGE_CREATE_RESP = 0x85,
    UART_FRAME_CMD_DFU_WRITE_DATA_EVENT = 0x86,
    UART_FRAME_CMD_DFU_PAGE_STORE_REQ   = 0x87,
    UART_FRAME_CMD_DFU_PAGE_STORE_RESP  = 0x88,
    UART_FRAME_CMD_DFU_STATE_CHECK_REQ  = 0x89,
    UART_FRAME_CMD_DFU_STATE_CHECK_RESP = 0x8A,
    UART_FRAME_CMD_DFU_CANCEL_REQ       = 0x8B,
    UART_FRAME_CMD_DFU_CANCEL_RESP      = 0x8C,
    UART_FRAME_CMD_RANGE2_END           = 0x8C
};

enum UartFrameStatus
{
    UART_FRAME_STATUS_FRAME_READY,
    UART_FRAME_STATUS_PROCESSING_NO_ERROR,
    UART_FRAME_STATUS_PREAMBLE_ERROR,
    UART_FRAME_STATUS_LENGTH_ERROR,
    UART_FRAME_STATUS_COMMAND_ERROR,
    UART_FRAME_STATUS_CRC_ERROR,
    UART_FRAME_STATUS_ERROR_UNKNOWN
};

struct PACKED UartFrameRxTxFrame
{
    uint8_t           len;
    enum UartFrameCmd cmd;
    uint8_t           p_payload[UART_FRAME_MAX_PAYLOAD_LEN];
};

void UartFrame_Init(void);

bool UartFrame_IsInitialized(void);

bool UartFrame_ProcessIncomingData(struct UartFrameRxTxFrame *p_rx_frame);

void UartFrame_Send(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len);

void UartFrame_Flush(void);

STATIC_ASSERT(sizeof(struct UartFrameRxTxFrame) == 129, Wrong_size_of_the_struct_UartFrameRxFrame);

#endif
