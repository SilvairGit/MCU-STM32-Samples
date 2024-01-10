#include <string.h>

#include "MockAssert.h"
#include "MockUartHal.h"
#include "UartFrame.c"
#include "unity.h"

static uint8_t *                 StubUartFrame    = NULL;
static size_t                    StubUartFrameCnt = 0;
static struct UartFrameRxTxFrame RxFrame          = {0};

static uint8_t *ExpectedFrame    = NULL;
static size_t   ExpectedFrameLen = 0;

void setUp(void)
{
    uint8_t uart_frame[6] = {0};

    size_t i;
    for (i = 0; i < sizeof(uart_frame); i++)
    {
        UartFrame_Decode(uart_frame[i], &RxFrame);
    }
}

void CheckValidFrame(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x01, 0x12, 0x34, 0x8B, 0xC4};

    enum UartFrameStatus status;

    size_t i;
    for (i = 0; i < sizeof(uart_frame); i++)
    {
        status = UartFrame_Decode(uart_frame[i], &RxFrame);

        if (i < (sizeof(uart_frame) - 1))
        {
            TEST_ASSERT_EQUAL(status, UART_FRAME_STATUS_PROCESSING_NO_ERROR);
        }
    }

    TEST_ASSERT_EQUAL(status, UART_FRAME_STATUS_FRAME_READY);
}

void CheckFrameDecodingStatus(uint8_t *p_frame, size_t frame_len, enum UartFrameStatus expected_status)
{
    enum UartFrameStatus status;

    size_t i;
    for (i = 0; i < frame_len; i++)
    {
        status = UartFrame_Decode(p_frame[i], &RxFrame);

        if (i < (frame_len - 1))
        {
            TEST_ASSERT_EQUAL(status, UART_FRAME_STATUS_PROCESSING_NO_ERROR);
        }
    }
    TEST_ASSERT_EQUAL(status, expected_status);
}

static bool StubUartHal_ReadByte(uint8_t *p_byte, int cmock_num_calls)
{
    *p_byte = StubUartFrame[StubUartFrameCnt];
    StubUartFrameCnt++;

    UNUSED(cmock_num_calls);

    return true;
}

void CheckFrameProcessingData(uint8_t *p_frame, size_t frame_len, bool expected_status)
{
    StubUartFrame    = p_frame;
    StubUartFrameCnt = 0;
    bool status;

    size_t i;
    for (i = 0; i < frame_len; i++)
    {
        UartHal_ReadByte_StubWithCallback(StubUartHal_ReadByte);

        status = UartFrame_ProcessIncomingData(&RxFrame);
        if (i < (frame_len - 1))
        {
            TEST_ASSERT_EQUAL(status, false);
        }
    }

    TEST_ASSERT_EQUAL(status, expected_status);
}

void test_ProcessIncomingDataNullPtr(void)
{
    Assert_Callback_ExpectAnyArgs();
    UartHal_ReadByte_ExpectAnyArgsAndReturn(false);

    UartFrame_ProcessIncomingData(NULL);
}

void test_DecodePreambleByte1NoError(void)
{
    uint8_t uart_frame[] = {0x12, 0x34, 0x56, 0x78};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_PROCESSING_NO_ERROR);
    CheckValidFrame();
}

void test_DecodePreambleByte2Error(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, 0x56};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_PREAMBLE_ERROR);
    CheckValidFrame();
}

void test_DecodeLenError(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 128};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_LENGTH_ERROR);
    CheckValidFrame();
}

void test_DecodeCommandOutOfRange1Error1(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, 0x00};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_COMMAND_ERROR);
    CheckValidFrame();
}

void test_DecodeCommandOutOfRange1Error2(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, UART_FRAME_CMD_RANGE1_END + 1};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_COMMAND_ERROR);
    CheckValidFrame();
}

void test_DecodeCommandOutOfRange2Error(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, UART_FRAME_CMD_RANGE2_END + 1};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_COMMAND_ERROR);
    CheckValidFrame();
}

void test_DecodePayloadTooShort(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x01, 0x34, 0x8B, 0xC4};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_PROCESSING_NO_ERROR);
}

void test_DecodePayloadTooLong(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x01, 0x12, 0x34, 0x56, 0x8B, 0xC4};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame) - 1, UART_FRAME_STATUS_CRC_ERROR);
    CheckValidFrame();
}

void test_DecodeProperFrameWithZeroLen(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x00, 0x17, 0x7F, 0x80};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_DecodeProperFrameWithMaxLen(void)
{
    uint8_t uart_frame[UART_FRAME_FRAME_LEN(127)] = {0};
    uart_frame[0]                                 = UART_FRAME_PREAMBLE_BYTE_1;
    uart_frame[1]                                 = UART_FRAME_PREAMBLE_BYTE_2;
    uart_frame[2]                                 = 127;
    uart_frame[3]                                 = 0x02;
    uart_frame[sizeof(uart_frame) - 2]            = 0x2E;
    uart_frame[sizeof(uart_frame) - 1]            = 0xE4;

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_DecodeProperFrameWithPayload(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x02, 0x12, 0x34, 0xB7, 0xC4};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_DecodeProperFrameWithCommandRange1Minimum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_PING_REQUEST, 0x12, 0x32, 0x9F, 0xC4};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_DecodeProperFrameWithCommandRange1Maximum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_TIME_GET_RESP, 0x12, 0x32, 0xD0, 0x46};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_DecodeProperFrameWithCommandRange2Minimum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_DFU_INIT_REQ, 0x12, 0x32, 0x8B, 0xCE};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_DecodeProperFrameWithCommandRange2Maximum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_DFU_CANCEL_RESP, 0x12, 0x32, 0x7B, 0xCE};

    CheckFrameDecodingStatus(uart_frame, sizeof(uart_frame), UART_FRAME_STATUS_FRAME_READY);
    CheckValidFrame();
}

void test_CrcForZeros(void)
{
    uint8_t uart_frame[128] = {0};

    uint16_t crc = UartFrame_CalculateCrc16(sizeof(uart_frame), 0, uart_frame);

    TEST_ASSERT_EQUAL_HEX16(crc, 0x0058);
}

void test_CrcForFFFF(void)
{
    uint8_t uart_frame[128] = {0};

    memset(uart_frame, 0xFF, sizeof(uart_frame));

    uint16_t crc = UartFrame_CalculateCrc16(sizeof(uart_frame), 0, uart_frame);

    TEST_ASSERT_EQUAL_HEX16(crc, 0x0076);
}

void test_CRCForRandomData(void)
{
    uint8_t uart_frame[] = {0x12, 0x34, 0x56, 0x78};

    uint16_t crc = UartFrame_CalculateCrc16(sizeof(uart_frame), 0, uart_frame);

    TEST_ASSERT_EQUAL_HEX16(crc, 0xFE5D);
}

void test_ProcessIncommingDataPreambleByte1NoError(void)
{
    uint8_t uart_frame[] = {0x12, 0x34, 0x56, 0x78};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
    CheckValidFrame();
}

void test_ProcessIncommingDataPreambleByte2Error(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, 0x56};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
    CheckValidFrame();
}

void test_ProcessIncommingDataLenError(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 128};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
    CheckValidFrame();
}

void test_ProcessIncommingDataCommandOutOfRange1Error1(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, 0x00};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
    CheckValidFrame();
}

void test_ProcessIncommingDataCommandOutOfRange1Error2(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, UART_FRAME_CMD_RANGE1_END + 1};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
    CheckValidFrame();
}

void test_ProcessIncommingDataCommandOutOfRange2Error(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, UART_FRAME_CMD_RANGE2_END + 1};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
    CheckValidFrame();
}

void test_ProcessIncommingDataPayloadTooShort(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x01, 0x34, 0x8B, 0xC4};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), false);
}

void test_ProcessIncommingDataPayloadTooLong(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x01, 0x12, 0x34, 0x56, 0x8B, 0xC4};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame) - 1, false);
    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithZeroLen(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x00, 0x17, 0x7F, 0x80};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithMaxLen(void)
{
    uint8_t uart_frame[UART_FRAME_FRAME_LEN(127)] = {0};
    uart_frame[0]                                 = UART_FRAME_PREAMBLE_BYTE_1;
    uart_frame[1]                                 = UART_FRAME_PREAMBLE_BYTE_2;
    uart_frame[2]                                 = 127;
    uart_frame[3]                                 = 0x02;
    uart_frame[sizeof(uart_frame) - 2]            = 0x2E;
    uart_frame[sizeof(uart_frame) - 1]            = 0xE4;

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithPayload(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x02, 0x12, 0x34, 0xB7, 0xC4};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithCommandRange1Minimum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_PING_REQUEST, 0x12, 0x32, 0x9F, 0xC4};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithCommandRange1Maximum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_TIME_GET_RESP, 0x12, 0x32, 0xD0, 0x46};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithCommandRange2Minimum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_DFU_INIT_REQ, 0x12, 0x32, 0x8B, 0xCE};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void test_ProcessIncommingDataProperFrameWithCommandRange2Maximum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_DFU_CANCEL_RESP, 0x12, 0x32, 0x7B, 0xCE};

    CheckFrameProcessingData(uart_frame, sizeof(uart_frame), true);

    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_LEN_OFFSET], RxFrame.len);
    TEST_ASSERT_EQUAL(uart_frame[UART_FRAME_CMD_OFFSET], RxFrame.cmd);
    TEST_ASSERT_TRUE(memcmp(uart_frame + UART_FRAME_HEADER_LEN, RxFrame.p_payload, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN) == 0);

    CheckValidFrame();
}

void StubUartHal_SendBuffer(uint8_t *p_buff, size_t buff_len, int cmock_num_calls)
{
    TEST_ASSERT_EQUAL(ExpectedFrameLen, buff_len);
    TEST_ASSERT_TRUE(memcmp(p_buff, ExpectedFrame, ExpectedFrameLen) == 0);

    UNUSED(cmock_num_calls);
}

void test_SendFrameWithZeroLen(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0, 0x01, 0x08, 0x00};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(0x01, NULL, 0);
}

void test_SendFrameWithMaxLen(void)
{
    uint8_t uart_frame[UART_FRAME_FRAME_LEN(127)] = {0};
    uart_frame[0]                                 = UART_FRAME_PREAMBLE_BYTE_1;
    uart_frame[1]                                 = UART_FRAME_PREAMBLE_BYTE_2;
    uart_frame[2]                                 = 127;
    uart_frame[3]                                 = 0x02;
    uart_frame[sizeof(uart_frame) - 2]            = 0x2E;
    uart_frame[sizeof(uart_frame) - 1]            = 0xE4;

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(0x02, uart_frame + UART_FRAME_PAYLOAD_OFFSET, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithPayload(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, 0x02, 0x12, 0x34, 0xB7, 0xC4};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(0x02, uart_frame + UART_FRAME_PAYLOAD_OFFSET, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandOutOfRange1Error1(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, 0x00};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    Assert_Callback_ExpectAnyArgs();
    UartHal_SendBuffer_ExpectAnyArgs();
    UartFrame_Send(0x02, uart_frame + UART_FRAME_PAYLOAD_OFFSET, (uint8_t)sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandOutOfRange1Error2(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, UART_FRAME_CMD_RANGE1_END + 1};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    Assert_Callback_ExpectAnyArgs();
    UartHal_SendBuffer_ExpectAnyArgs();
    UartFrame_Send(0x02, uart_frame + UART_FRAME_PAYLOAD_OFFSET, (uint8_t)sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandOutOfRange2Error(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x01, UART_FRAME_CMD_RANGE2_END + 1};
    ExpectedFrame        = uart_frame;
    ExpectedFrameLen     = sizeof(uart_frame);

    Assert_Callback_ExpectAnyArgs();
    UartHal_SendBuffer_ExpectAnyArgs();
    UartFrame_Send(0x02, uart_frame + UART_FRAME_PAYLOAD_OFFSET, (uint8_t)sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandRange1Minimum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_PING_REQUEST, 0x12, 0x32, 0x9F, 0xC4};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(UART_FRAME_CMD_PING_REQUEST, uart_frame + UART_FRAME_PAYLOAD_OFFSET, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandRange1Maximum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_TIME_GET_RESP, 0x12, 0x32, 0xD0, 0x46};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(UART_FRAME_CMD_TIME_GET_RESP, uart_frame + UART_FRAME_PAYLOAD_OFFSET, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandRange2Minimum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_DFU_INIT_REQ, 0x12, 0x32, 0x8B, 0xCE};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(UART_FRAME_CMD_DFU_INIT_REQ, uart_frame + UART_FRAME_PAYLOAD_OFFSET, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}

void test_SendFrameWithCommandRange2Maximum(void)
{
    uint8_t uart_frame[] = {UART_FRAME_PREAMBLE_BYTE_1, UART_FRAME_PREAMBLE_BYTE_2, 0x02, UART_FRAME_CMD_DFU_CANCEL_RESP, 0x12, 0x32, 0x7B, 0xCE};

    ExpectedFrame    = uart_frame;
    ExpectedFrameLen = sizeof(uart_frame);

    UartHal_SendBuffer_StubWithCallback(StubUartHal_SendBuffer);

    UartFrame_Send(UART_FRAME_CMD_DFU_CANCEL_RESP, uart_frame + UART_FRAME_PAYLOAD_OFFSET, sizeof(uart_frame) - UART_FRAME_HEADER_LEN - UART_FRAME_CRC_LEN);
}
