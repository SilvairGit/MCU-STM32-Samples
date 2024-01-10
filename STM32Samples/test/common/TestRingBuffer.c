#include <string.h>

#include "MockAssert.h"
#include "RingBuffer.c"
#include "unity.h"

#define BYTE_BUFFER_LEN 16
static uint8_t ByteBuffer[BYTE_BUFFER_LEN];

static struct RingBuffer RingBuffer;

void setUp(void)
{
    memset(&RingBuffer, 0, sizeof(struct RingBuffer));

    RingBuffer_Init(&RingBuffer, ByteBuffer, sizeof(ByteBuffer));
}

void test_Init(void)
{
    TEST_ASSERT_EQUAL(RingBuffer.p_buf, ByteBuffer);
    TEST_ASSERT_EQUAL(RingBuffer.buf_len, sizeof(ByteBuffer));
    TEST_ASSERT_EQUAL(RingBuffer.wr, 0);
    TEST_ASSERT_EQUAL(RingBuffer.rd, 0);
}

void test_InitNull(void)
{
    Assert_Callback_ExpectAnyArgs();
    RingBuffer_Init(&RingBuffer, NULL, sizeof(ByteBuffer));
}

void test_IsEmpty(void)
{
    TEST_ASSERT_EQUAL(RingBuffer_IsEmpty(&RingBuffer), true);

    RingBuffer.wr++;
    TEST_ASSERT_EQUAL(RingBuffer_IsEmpty(&RingBuffer), false);
}

void test_SetWrIndex(void)
{
    RingBuffer_SetWrIndex(&RingBuffer, 15);
    TEST_ASSERT_EQUAL(RingBuffer.wr, 15);

    RingBuffer_SetWrIndex(&RingBuffer, 16);
    TEST_ASSERT_EQUAL(RingBuffer.wr, 0);

    RingBuffer_SetWrIndex(&RingBuffer, 17);
    TEST_ASSERT_EQUAL(RingBuffer.wr, 1);
}

void test_IncrementRdIndex(void)
{
    RingBuffer_IncrementRdIndex(&RingBuffer, 15);
    TEST_ASSERT_EQUAL(RingBuffer.rd, 15);

    RingBuffer_IncrementRdIndex(&RingBuffer, 1);
    TEST_ASSERT_EQUAL(RingBuffer.rd, 0);

    RingBuffer_IncrementRdIndex(&RingBuffer, 1);
    TEST_ASSERT_EQUAL(RingBuffer.rd, 1);
}

void test_DequeueByteEmpty(void)
{
    uint8_t byte;
    bool    ret_val = RingBuffer_DequeueByte(&RingBuffer, &byte);
    TEST_ASSERT_EQUAL(ret_val, false);
}

void test_QueueByteOverflow(void)
{
    bool ret_val;

    uint8_t bytes1[BYTE_BUFFER_LEN];
    ret_val = RingBuffer_QueueBytes(&RingBuffer, bytes1, sizeof(bytes1));
    TEST_ASSERT_EQUAL(ret_val, true);

    uint8_t bytes2[BYTE_BUFFER_LEN + 1];
    ret_val = RingBuffer_QueueBytes(&RingBuffer, bytes2, sizeof(bytes2));

    TEST_ASSERT_EQUAL(ret_val, false);
}

void test_DequeuBytesNull(void)
{
    Assert_Callback_ExpectAnyArgs();

    RingBuffer_DequeueByte(&RingBuffer, NULL);
}

void test_QueueAndDequeueByte(void)
{
    size_t i;
    for (i = 0; i < BYTE_BUFFER_LEN - 1; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_QueueBytes(&RingBuffer, &byte, sizeof(byte));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i], i);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 1);
    }

    for (i = 0; i < BYTE_BUFFER_LEN - 1; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_DequeueByte(&RingBuffer, &byte);
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(byte, i);
    }

    for (i = 0; i < BYTE_BUFFER_LEN - 1; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_QueueBytes(&RingBuffer, &byte, sizeof(byte));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i], i);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 1);
    }

    for (i = 0; i < BYTE_BUFFER_LEN - 1; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_DequeueByte(&RingBuffer, &byte);
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(byte, i);
    }
}

void test_QueueAndDequeueMutipleBytes(void)
{
    size_t i;
    for (i = 0; i < 12; i += 4)
    {
        uint8_t bytes[4] = {i, i + 1, i + 2, i + 3};
        bool    ret_val  = RingBuffer_QueueBytes(&RingBuffer, bytes, sizeof(bytes));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 0], i + 0);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 1], i + 1);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 2], i + 2);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 3], i + 3);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 4);
    }

    for (i = 0; i < 12; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_DequeueByte(&RingBuffer, &byte);
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(byte, i);
    }

    for (i = 0; i < 12; i += 4)
    {
        uint8_t bytes[4] = {i, i + 1, i + 2, i + 3};
        bool    ret_val  = RingBuffer_QueueBytes(&RingBuffer, bytes, sizeof(bytes));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 0], i + 0);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 1], i + 1);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 2], i + 2);
        TEST_ASSERT_EQUAL(ByteBuffer[i + 3], i + 3);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 4);
    }

    for (i = 0; i < 12; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_DequeueByte(&RingBuffer, &byte);
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(byte, i);
    }
}

void test_GetMaxContinuousBuffer(void)
{
    bool     ret_val;
    uint8_t *p_buffer;
    uint8_t  bytes[12] = {0};
    uint16_t buff_len  = 0;

    ret_val = RingBuffer_QueueBytes(&RingBuffer, bytes, sizeof(bytes));
    TEST_ASSERT_EQUAL(ret_val, true);

    p_buffer = RingBuffer_GetMaxContinuousBuffer(&RingBuffer, &buff_len);

    TEST_ASSERT_EQUAL(p_buffer, RingBuffer.p_buf);
    TEST_ASSERT_EQUAL(buff_len, 12);

    size_t i;
    for (i = 0; i < 12; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_DequeueByte(&RingBuffer, &byte);
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(byte, 0);
    }

    ret_val = RingBuffer_QueueBytes(&RingBuffer, bytes, sizeof(bytes));
    TEST_ASSERT_EQUAL(ret_val, true);

    p_buffer = RingBuffer_GetMaxContinuousBuffer(&RingBuffer, &buff_len);

    TEST_ASSERT_EQUAL(p_buffer, RingBuffer.p_buf + 12);
    TEST_ASSERT_EQUAL(buff_len, 4);
}

void test_DataLen(void)
{
    size_t i;
    for (i = 0; i < BYTE_BUFFER_LEN - 1; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_QueueBytes(&RingBuffer, &byte, sizeof(byte));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i], i);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 1);
    }

    TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), 15);

    uint8_t byte    = i;
    bool    ret_val = RingBuffer_QueueBytes(&RingBuffer, &byte, sizeof(byte));
    TEST_ASSERT_EQUAL(ret_val, true);
    TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), 0);
}

void test_IsOverflow(void)
{
    size_t i;
    for (i = 0; i < 12; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_QueueBytes(&RingBuffer, &byte, sizeof(byte));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i], i);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 1);
    }

    TEST_ASSERT_EQUAL(IsOverflow(&RingBuffer, 4), false);
    TEST_ASSERT_EQUAL(IsOverflow(&RingBuffer, 5), true);
}

void test_MaxQueueBufferLen(void)
{
    TEST_ASSERT_EQUAL(MaxQueueBufferLen(&RingBuffer, 17), 0);

    size_t i;
    for (i = 0; i < 12; i++)
    {
        uint8_t byte    = i;
        bool    ret_val = RingBuffer_QueueBytes(&RingBuffer, &byte, sizeof(byte));
        TEST_ASSERT_EQUAL(ret_val, true);
        TEST_ASSERT_EQUAL(ByteBuffer[i], i);
        TEST_ASSERT_EQUAL(RingBuffer_DataLen(&RingBuffer), i + 1);
    }

    TEST_ASSERT_EQUAL(MaxQueueBufferLen(&RingBuffer, 4), 4);
}
