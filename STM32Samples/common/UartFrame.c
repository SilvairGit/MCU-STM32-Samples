#include "UartFrame.h"

#include <stddef.h>
#include <string.h>

#include "Assert.h"
#include "Checksum.h"
#include "Log.h"
#include "UartHal.h"


#define UART_FRAME_PREAMBLE_BYTE_1_OFFSET 0
#define UART_FRAME_PREAMBLE_BYTE_2_OFFSET 1
#define UART_FRAME_LEN_OFFSET 2
#define UART_FRAME_CMD_OFFSET 3
#define UART_FRAME_PAYLOAD_OFFSET 4
#define UART_FRAME_CRC_BYTE_1_OFFSET(len) (UART_FRAME_PAYLOAD_OFFSET + len)
#define UART_FRAME_CRC_BYTE_2_OFFSET(len) (UART_FRAME_PAYLOAD_OFFSET + len + 1)

#define UART_FRAME_HEADER_LEN 4
#define UART_FRAME_CRC_LEN 2
#define UART_FRAME_FRAME_LEN(len) (UART_FRAME_HEADER_LEN + len + UART_FRAME_CRC_LEN)

#define UART_FRAME_PREAMBLE_BYTE_1 0xAA
#define UART_FRAME_PREAMBLE_BYTE_2 0x55

#define UART_FRAME_CRC16_INIT_VAL 0xFFFFu


static bool IsInitialized = false;

static enum UartFrameStatus UartFrame_Decode(uint8_t received_byte, struct UartFrameRxTxFrame *p_rx_frame);
static uint16_t             UartFrame_CalculateCrc16(uint8_t len, uint8_t cmd, uint8_t *p_data);

void UartFrame_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("UartFrame initialization");

    if (!UartHal_IsInitialized())
    {
        UartHal_Init();
    }

    IsInitialized = true;
}

bool UartFrame_IsInitialized(void)
{
    return IsInitialized;
}

bool UartFrame_ProcessIncomingData(struct UartFrameRxTxFrame *p_rx_frame)
{
    ASSERT(p_rx_frame != NULL);

    uint8_t received_byte;
    if (!UartHal_ReadByte(&received_byte))
    {
        return false;
    }

    enum UartFrameStatus status = UartFrame_Decode(received_byte, p_rx_frame);

    switch (status)
    {
        case UART_FRAME_STATUS_FRAME_READY:
#if UART_FRAME_LOGGER_ENABLE
            LOG_D("Frame received: len: %u, cmd, 0x%02X", p_rx_frame->len, p_rx_frame->cmd);
            LOG_HEX_D("Payload:", p_rx_frame->p_payload, p_rx_frame->len);
#endif
            return true;

        case UART_FRAME_STATUS_PROCESSING_NO_ERROR:
            return false;

        case UART_FRAME_STATUS_PREAMBLE_ERROR:
        case UART_FRAME_STATUS_LENGTH_ERROR:
        case UART_FRAME_STATUS_COMMAND_ERROR:
        case UART_FRAME_STATUS_CRC_ERROR:
        case UART_FRAME_STATUS_ERROR_UNKNOWN:
            LOG_W("Frame received error: 0x%02X, len: %u, cmd: 0x%02X", status, p_rx_frame->len, p_rx_frame->cmd);
            LOG_HEX_W("Payload:", p_rx_frame->p_payload, p_rx_frame->len);
            return false;

        default:
            ASSERT(false);
            break;
    }

    return false;
}

void UartFrame_Send(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len)
{
    ASSERT((len <= UART_FRAME_MAX_PAYLOAD_LEN) && (((cmd >= UART_FRAME_CMD_RANGE1_START) && (cmd <= UART_FRAME_CMD_RANGE1_END)) ||
                                                   ((cmd >= UART_FRAME_CMD_RANGE2_START) && (cmd <= UART_FRAME_CMD_RANGE2_END))));

    uint16_t crc;
    uint8_t  frame[UART_FRAME_FRAME_LEN(len)];

    frame[UART_FRAME_PREAMBLE_BYTE_1_OFFSET] = UART_FRAME_PREAMBLE_BYTE_1;
    frame[UART_FRAME_PREAMBLE_BYTE_2_OFFSET] = UART_FRAME_PREAMBLE_BYTE_2;
    frame[UART_FRAME_LEN_OFFSET]             = len;
    frame[UART_FRAME_CMD_OFFSET]             = cmd;

    if (p_payload != NULL)
    {
        memcpy(frame + UART_FRAME_PAYLOAD_OFFSET, p_payload, len);
    }

    crc = UartFrame_CalculateCrc16(len, cmd, p_payload);

    frame[UART_FRAME_CRC_BYTE_1_OFFSET(len)] = LOW_BYTE(crc);
    frame[UART_FRAME_CRC_BYTE_2_OFFSET(len)] = HIGH_BYTE(crc);

    UartHal_SendBuffer(frame, UART_FRAME_FRAME_LEN(len));

#if UART_FRAME_LOGGER_ENABLE
    LOG_D("Frame sent: len: %u, cmd: 0x%02X", len, cmd);
    LOG_HEX_D("Payload:", p_payload, len);
#endif
}

void UartFrame_Flush(void)
{
    UartHal_Flush();
}

static enum UartFrameStatus UartFrame_Decode(uint8_t received_byte, struct UartFrameRxTxFrame *p_rx_frame)
{
    ASSERT(p_rx_frame != NULL);

    static uint16_t crc            = 0;
    static uint8_t  frame_byte_cnt = 0;

    if (frame_byte_cnt == UART_FRAME_PREAMBLE_BYTE_1_OFFSET)
    {
        if (received_byte == UART_FRAME_PREAMBLE_BYTE_1)
        {
            frame_byte_cnt++;
            p_rx_frame->len = 0;
            p_rx_frame->cmd = (enum UartFrameCmd)0;
            return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
        }

        frame_byte_cnt = 0;
        // This is first byte of the frame. If we start receiving frame from the middle byte we can enter in this case.
        return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
    }

    if (frame_byte_cnt == UART_FRAME_PREAMBLE_BYTE_2_OFFSET)
    {
        if (received_byte == UART_FRAME_PREAMBLE_BYTE_2)
        {
            frame_byte_cnt++;
            return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
        }

        frame_byte_cnt = 0;
        return UART_FRAME_STATUS_PREAMBLE_ERROR;
    }

    if (frame_byte_cnt == UART_FRAME_LEN_OFFSET)
    {
        if (received_byte <= UART_FRAME_MAX_PAYLOAD_LEN)
        {
            p_rx_frame->len = received_byte;
            frame_byte_cnt++;
            return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
        }

        frame_byte_cnt = 0;
        return UART_FRAME_STATUS_LENGTH_ERROR;
    }

    if (frame_byte_cnt == UART_FRAME_CMD_OFFSET)
    {
        if (((received_byte >= UART_FRAME_CMD_RANGE1_START) && (received_byte <= UART_FRAME_CMD_RANGE1_END)) ||
            ((received_byte >= UART_FRAME_CMD_RANGE2_START) && (received_byte <= UART_FRAME_CMD_RANGE2_END)))
        {
            p_rx_frame->cmd = (enum UartFrameCmd)received_byte;
            frame_byte_cnt++;
            return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
        }

        frame_byte_cnt = 0;
        return UART_FRAME_STATUS_COMMAND_ERROR;
    }

    if (frame_byte_cnt < (p_rx_frame->len + UART_FRAME_PAYLOAD_OFFSET))
    {
        p_rx_frame->p_payload[frame_byte_cnt - UART_FRAME_PAYLOAD_OFFSET] = received_byte;
        frame_byte_cnt++;
        return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
    }

    if (frame_byte_cnt == UART_FRAME_CRC_BYTE_1_OFFSET(p_rx_frame->len))
    {
        crc = received_byte;
        frame_byte_cnt++;
        return UART_FRAME_STATUS_PROCESSING_NO_ERROR;
    }

    if (frame_byte_cnt == UART_FRAME_CRC_BYTE_2_OFFSET(p_rx_frame->len))
    {
        crc += ((uint16_t)received_byte) << 8;
        if (crc == UartFrame_CalculateCrc16(p_rx_frame->len, p_rx_frame->cmd, p_rx_frame->p_payload))
        {
            frame_byte_cnt = 0;
            return UART_FRAME_STATUS_FRAME_READY;
        }

        frame_byte_cnt = 0;
        return UART_FRAME_STATUS_CRC_ERROR;
    }

    // We should never reach this point
    ASSERT(false);

    frame_byte_cnt = 0;
    return UART_FRAME_STATUS_ERROR_UNKNOWN;
}

static uint16_t UartFrame_CalculateCrc16(uint8_t len, uint8_t cmd, uint8_t *p_data)
{
    uint16_t crc = UART_FRAME_CRC16_INIT_VAL;
    crc          = Checksum_CalcCRC16(&len, sizeof(len), crc);
    crc          = Checksum_CalcCRC16(&cmd, sizeof(cmd), crc);
    crc          = Checksum_CalcCRC16(p_data, len, crc);
    return crc;
}
