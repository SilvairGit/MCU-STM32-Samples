#include "Attention.c"
#include "MockAssert.h"
#include "MockGpioHal.h"
#include "MockLuminaire.h"
#include "MockSimpleScheduler.h"
#include "MockUartProtocol.h"
#include "unity.h"

void test_Init(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, Attention_IsInitialized());

    GpioHal_IsInitialized_ExpectAndReturn(false);
    GpioHal_Init_Expect();

    UartProtocol_IsInitialized_ExpectAndReturn(false);
    UartProtocol_Init_Expect();

    GpioHal_PinMode_Expect(GPIO_HAL_PIN_LED_STATUS, GPIO_HAL_MODE_OUTPUT);

    UartProtocol_RegisterMessageHandler_Expect(&MessageHandlerConfig);

    SimpleScheduler_TaskAdd_Expect(ATTENTION_TASK_PERIOD_MS, Attention_Loop, SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);

    Attention_Init();

    TEST_ASSERT_EQUAL(true, Attention_IsInitialized());
}

void test_InitAssert(void)
{
    IsInitialized = true;

    Assert_Callback_ExpectAnyArgs();

    GpioHal_IsInitialized_ExpectAndReturn(true);
    UartProtocol_IsInitialized_ExpectAndReturn(true);
    GpioHal_PinMode_Expect(GPIO_HAL_PIN_LED_STATUS, GPIO_HAL_MODE_OUTPUT);

    UartProtocol_RegisterMessageHandler_ExpectAnyArgs();
    SimpleScheduler_TaskAdd_Expect(ATTENTION_TASK_PERIOD_MS, Attention_Loop, SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);

    Attention_Init();
}

void test_IsInitialized(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, Attention_IsInitialized());

    IsInitialized = true;

    TEST_ASSERT_EQUAL(true, Attention_IsInitialized());
}

void test_StateChangeEnableLuminairePresent(void)
{
    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, true);
    Luminaire_IsInitialized_ExpectAndReturn(true);
    Luminaire_IndicateAttention_Expect(true, false);

    Attention_StateChange(true);

    TEST_ASSERT_EQUAL(false, AttentionLedValue);
}

void test_StateChangeDisableLuminairePresent(void)
{
    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);
    Luminaire_IsInitialized_ExpectAndReturn(true);
    Luminaire_IndicateAttention_Expect(false, true);

    Attention_StateChange(false);

    TEST_ASSERT_EQUAL(true, AttentionLedValue);
}

void test_StateChangeEnableLuminaireNotPresent(void)
{
    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, true);
    Luminaire_IsInitialized_ExpectAndReturn(false);

    Attention_StateChange(true);

    TEST_ASSERT_EQUAL(false, AttentionLedValue);
}

void test_StateChangeDisableLuminaireNotPresent(void)
{
    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);
    Luminaire_IsInitialized_ExpectAndReturn(false);

    Attention_StateChange(false);

    TEST_ASSERT_EQUAL(true, AttentionLedValue);
}

void test_UartMessageHandlerAttentionEventOffLuminairePresent(void)
{
    struct UartFrameRxTxFrame frame = {
        .len       = 1,
        .cmd       = UART_FRAME_CMD_ATTENTION_EVENT,
        .p_payload = {0x00},
    };

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);
    Luminaire_IsInitialized_ExpectAndReturn(true);
    Luminaire_IndicateAttention_Expect(false, true);

    Attention_UartMessageHandler(&frame);

    TEST_ASSERT_EQUAL(true, AttentionLedValue);
}

void test_UartMessageHandlerAttentionEventOnLuminairePresent(void)
{
    struct UartFrameRxTxFrame frame = {
        .len       = 1,
        .cmd       = UART_FRAME_CMD_ATTENTION_EVENT,
        .p_payload = {0x01},
    };

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, true);
    Luminaire_IsInitialized_ExpectAndReturn(true);
    Luminaire_IndicateAttention_Expect(true, false);

    Attention_UartMessageHandler(&frame);

    TEST_ASSERT_EQUAL(false, AttentionLedValue);
}

void test_UartMessageHandlerAttentionEventOffLuminaireNotPresent(void)
{
    struct UartFrameRxTxFrame frame = {
        .len       = 1,
        .cmd       = UART_FRAME_CMD_ATTENTION_EVENT,
        .p_payload = {0x00},
    };

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, false);
    Luminaire_IsInitialized_ExpectAndReturn(false);

    Attention_UartMessageHandler(&frame);

    TEST_ASSERT_EQUAL(true, AttentionLedValue);
}

void test_UartMessageHandlerAttentionEventOnLuminaireNotPresent(void)
{
    struct UartFrameRxTxFrame frame = {
        .len       = 1,
        .cmd       = UART_FRAME_CMD_ATTENTION_EVENT,
        .p_payload = {0x01},
    };

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_ATTENTION, true);
    Luminaire_IsInitialized_ExpectAndReturn(false);

    Attention_UartMessageHandler(&frame);

    TEST_ASSERT_EQUAL(false, AttentionLedValue);
}

void test_UartMessageHandlerAttentionEventBadLength(void)
{
    struct UartFrameRxTxFrame frame1 = {
        .len       = 0,
        .cmd       = UART_FRAME_CMD_ATTENTION_EVENT,
        .p_payload = {0x01},
    };
    Attention_UartMessageHandler(&frame1);

    struct UartFrameRxTxFrame frame2 = {
        .len       = 2,
        .cmd       = UART_FRAME_CMD_ATTENTION_EVENT,
        .p_payload = {0x01, 0x00},
    };
    Attention_UartMessageHandler(&frame2);
}

void test_AttentionLoopLuminairePresent(void)
{
    AttentionLedValue = false;

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, true);
    Luminaire_IsInitialized_ExpectAndReturn(true);
    Luminaire_IndicateAttention_Expect(true, true);

    Attention_Loop();
    TEST_ASSERT_EQUAL(true, true);

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    Luminaire_IsInitialized_ExpectAndReturn(true);
    Luminaire_IndicateAttention_Expect(true, false);

    Attention_Loop();
    TEST_ASSERT_EQUAL(false, false);
}

void test_AttentionLoopLuminaireNotPresent(void)
{
    AttentionLedValue = false;

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, true);
    Luminaire_IsInitialized_ExpectAndReturn(false);

    Attention_Loop();
    TEST_ASSERT_EQUAL(true, true);

    GpioHal_PinSet_Expect(GPIO_HAL_PIN_LED_STATUS, false);
    Luminaire_IsInitialized_ExpectAndReturn(false);

    Attention_Loop();
    TEST_ASSERT_EQUAL(false, false);
}
