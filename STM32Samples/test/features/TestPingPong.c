#include <string.h>

#include "MockAssert.h"
#include "MockUartProtocol.h"
#include "PingPong.c"
#include "Utils.h"
#include "unity.h"

void test_Init(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, PingPong_IsInitialized());

    UartProtocol_IsInitialized_ExpectAndReturn(true);
    UartProtocol_RegisterMessageHandler_ExpectAnyArgs();

    PingPong_Init();

    TEST_ASSERT_EQUAL(true, PingPong_IsInitialized());
}

void test_InitAssert(void)
{
    IsInitialized = false;

    UartProtocol_IsInitialized_ExpectAndReturn(false);
    UartProtocol_Init_Expect();
    UartProtocol_RegisterMessageHandler_ExpectAnyArgs();

    PingPong_Init();
}

void test_IsInitialized(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, PingPong_IsInitialized());

    IsInitialized = true;

    TEST_ASSERT_EQUAL(true, PingPong_IsInitialized());
}

void test_Enable(void)
{
    PingPong_Enable(false);
    TEST_ASSERT_EQUAL(false, PingPongIsEnable);

    PingPong_Enable(true);
    TEST_ASSERT_EQUAL(true, PingPongIsEnable);
}

void UartProtocol_Send_Stub(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len, int cmock_num_calls)
{
    uint8_t frame_payload[] = "Test";

    TEST_ASSERT_EQUAL(cmd, UART_FRAME_CMD_PONG_RESPONSE);
    TEST_ASSERT_TRUE(memcmp(p_payload, frame_payload, len) == 0);
    TEST_ASSERT_EQUAL(len, 4);

    UNUSED(cmock_num_calls);
}

void test_UartMessageHandlerPingEnable(void)
{
    PingPong_Enable(true);

    uint8_t frame_buff[32];

    struct UartProtocolFramePingRequest *p_frame = (struct UartProtocolFramePingRequest *)frame_buff;

    size_t index             = 0;
    p_frame->cmd             = UART_FRAME_CMD_PING_REQUEST;
    p_frame->p_data[index++] = 'T';
    p_frame->p_data[index++] = 'e';
    p_frame->p_data[index++] = 's';
    p_frame->p_data[index++] = 't';
    p_frame->len             = index;

    UartProtocol_Send_StubWithCallback(UartProtocol_Send_Stub);
    PingPong_UartMessageHandler((struct UartFrameRxTxFrame *)p_frame);
}

void test_SendPongResponse(void)
{
    PingPong_Enable(false);

    uint8_t frame_buff[32];

    struct UartProtocolFramePingRequest *p_frame = (struct UartProtocolFramePingRequest *)frame_buff;

    size_t index             = 0;
    p_frame->cmd             = UART_FRAME_CMD_PING_REQUEST;
    p_frame->p_data[index++] = 'T';
    p_frame->p_data[index++] = 'e';
    p_frame->p_data[index++] = 's';
    p_frame->p_data[index++] = 't';
    p_frame->len             = index;

    PingPong_UartMessageHandler((struct UartFrameRxTxFrame *)p_frame);
}
