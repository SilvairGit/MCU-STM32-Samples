#include "Luminaire.c"
#include "MockAssert.h"
#include "MockModelManager.h"
#include "MockPwmHal.h"
#include "MockSimpleScheduler.h"
#include "MockTimestamp.h"
#include "MockUartProtocol.h"
#include "math.h"
#include "unity.h"

static uint16_t LightnessToPwm(uint16_t lightness)
{
    uint32_t pwm_out;
    pwm_out = Luminaire_ConvertLightnessToLinear(lightness);
    pwm_out = pwm_out * PWM_HAL_OUTPUT_DUTY_CYCLE_MAX / UINT16_MAX;
    return pwm_out;
}

static uint16_t LightnessToPwmWarm(uint16_t lightness, uint16_t temperature)
{
    uint32_t warm;
    warm = (LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K - temperature) * LightnessToPwm(lightness);
    warm /= LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K - LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K;

    return warm;
}

static uint16_t LightnessToPwmCold(uint16_t lightness, uint16_t temperature)
{
    uint32_t cold;
    cold = (temperature - LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K) * LightnessToPwm(lightness);
    cold /= LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K - LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K;

    return cold;
}

void setUp(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    IsInitialized                 = false;
    IsCtlInitialized              = false;
    IsAttentionSequenceInProgress = false;

    Lightness   = 0;
    Temperature = 0;
}

void test_InitLc(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, Luminaire_IsInitialized());

    PwmHal_IsInitialized_ExpectAndReturn(false);
    PwmHal_Init_ExpectAnyArgs();

    UartProtocol_IsInitialized_ExpectAndReturn(false);
    UartProtocol_Init_ExpectAnyArgs();

    ModelManager_RegisterModel_Expect(&ModelConfigLightLcServer);

    UartProtocol_RegisterMessageHandler_Expect(&MessageHandlerConfig);
    SimpleScheduler_TaskAdd_Expect(LUMINAIRE_TASK_PERIOD_MS, Luminaire_Loop, SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);

    Luminaire_Init(LUMINAIRE_INIT_MODE_LIGHT_LC);

    TEST_ASSERT_EQUAL(true, Luminaire_IsInitialized());
}

void test_InitCtl(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, Luminaire_IsInitialized());

    PwmHal_IsInitialized_ExpectAndReturn(false);
    PwmHal_Init_ExpectAnyArgs();

    UartProtocol_IsInitialized_ExpectAndReturn(false);
    UartProtocol_Init_ExpectAnyArgs();

    ModelManager_RegisterModel_Expect(&ModelConfigLightCtlServer);

    UartProtocol_RegisterMessageHandler_Expect(&MessageHandlerConfig);
    SimpleScheduler_TaskAdd_Expect(LUMINAIRE_TASK_PERIOD_MS, Luminaire_Loop, SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);

    Luminaire_Init(LUMINAIRE_INIT_MODE_LIGHT_CTL);

    TEST_ASSERT_EQUAL(true, Luminaire_IsInitialized());
}

void test_InitAssert(void)
{
    IsInitialized = true;

    Assert_Callback_ExpectAnyArgs();

    PwmHal_IsInitialized_ExpectAndReturn(true);
    UartProtocol_IsInitialized_ExpectAndReturn(true);

    ModelManager_RegisterModel_ExpectAnyArgs();

    UartProtocol_RegisterMessageHandler_ExpectAnyArgs();
    SimpleScheduler_TaskAdd_Expect(LUMINAIRE_TASK_PERIOD_MS, Luminaire_Loop, SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);

    Luminaire_Init(LUMINAIRE_INIT_MODE_LIGHT_LC);
}

void test_InitAssertModeNotSupported(void)
{
    Assert_Callback_ExpectAnyArgs();

    PwmHal_IsInitialized_ExpectAndReturn(true);
    UartProtocol_IsInitialized_ExpectAndReturn(true);

    UartProtocol_RegisterMessageHandler_ExpectAnyArgs();
    SimpleScheduler_TaskAdd_Expect(LUMINAIRE_TASK_PERIOD_MS, Luminaire_Loop, SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);

    Luminaire_Init(LUMINAIRE_INIT_MODE_LIGHT_LC + 2);
}

void test_IsInitialized(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, Luminaire_IsInitialized());

    IsInitialized = true;

    TEST_ASSERT_EQUAL(true, Luminaire_IsInitialized());
}

void test_Luminaire_IndicateAttentionAttentionDisableLedDisable(void)
{
    // Set lightness before attention
    Lightness                     = 2345;
    IsAttentionSequenceInProgress = false;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(true, false);

    Lightness = 2345;

    IsAttentionSequenceInProgress = true;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(false, false);

    IsAttentionSequenceInProgress = false;

    Luminaire_IndicateAttention(false, false);
}

void test_Luminaire_IndicateAttentionAttentionDisableLedEnable(void)
{
    // Set lightness before attention
    Lightness                     = 1234;
    IsAttentionSequenceInProgress = false;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(true, false);

    Lightness                     = 1234;
    IsAttentionSequenceInProgress = true;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(false, true);

    IsAttentionSequenceInProgress = false;

    Luminaire_IndicateAttention(false, true);
}

void test_Luminaire_IndicateAttentionAttentionEnableLedDisable(void)
{
    IsAttentionSequenceInProgress = true;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(true, false);

    IsAttentionSequenceInProgress = false;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_LOW));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(true, false);
}

void test_Luminaire_IndicateAttentionAttentionEnableLedEnable(void)
{
    IsAttentionSequenceInProgress = true;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_HIGH));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_HIGH));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(true, true);

    IsAttentionSequenceInProgress = false;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_HIGH));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_ATTENTION_LIGHTNESS_HIGH));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_IndicateAttention(true, true);
}

void test_StartStartupSequence(void)
{
    Timestamp_GetCurrent_ExpectAndReturn(1234);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, true);

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(0);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_StartStartupBehavior();

    TEST_ASSERT_EQUAL(1234, StartupSequenceStartTimestamp);
}

void test_StopStartupSequence(void)
{
    Lightness = 2345;

    IsAttentionSequenceInProgress = true;

    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);
    Luminaire_StopStartupSequence();

    IsAttentionSequenceInProgress = false;

    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_StopStartupSequence();
}

void test_MeshMessageHandlerLightLStatus(void)
{
    MessageHandlerConfig.instance_index = 1;

    struct UartProtocolFrameMeshMessageLightLStatusV1 payload_v1 = {
        .present_lightness = 1234,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v1 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v1,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightLStatusV1),
    };

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(payload_v1.present_lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(payload_v1.present_lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_MeshMessageHandler(&frame_v1);

    struct UartProtocolFrameMeshMessageLightLStatusV2 payload_v2 = {
        .present_lightness = 2345,
        .target_lightness  = 1,
        .remaining_time    = 2,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v2 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v2,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightLStatusV2),
    };

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(payload_v2.present_lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(payload_v2.present_lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_MeshMessageHandler(&frame_v2);
}

void test_MeshMessageHandlerLightLStatusBadLenght(void)
{
    MessageHandlerConfig.instance_index = 1;

    struct UartProtocolFrameMeshMessageLightLStatusV1 payload_v1 = {
        .present_lightness = 1234,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v1 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v1,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightLStatusV1) - 1,
    };

    Luminaire_MeshMessageHandler(&frame_v1);
}

void test_MeshMessageHandlerLightLStatusUnknownInstanceIndex(void)
{
    struct UartProtocolFrameMeshMessageLightLStatusV1 payload_v1 = {
        .present_lightness = 1234,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v1 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v1,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightLStatusV1),
    };

    Luminaire_MeshMessageHandler(&frame_v1);
}

void test_MeshMessageHandlerLightCtlTemperatureStatus(void)
{
    MessageHandlerConfig.instance_index = 1;

    IsCtlInitialized = true;
    Lightness        = 1234;

    struct UartProtocolFrameMeshMessageLightCtlTempStatusV1 payload_v1 = {
        .present_ctl_temperature = 5678,
        .present_ctl_delta_uv    = 0,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v1 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v1,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV1),
    };

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwmCold(Lightness, payload_v1.present_ctl_temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwmWarm(Lightness, payload_v1.present_ctl_temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwmCold(Lightness, payload_v1.present_ctl_temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwmWarm(Lightness, payload_v1.present_ctl_temperature));

    Luminaire_MeshMessageHandler(&frame_v1);

    struct UartProtocolFrameMeshMessageLightCtlTempStatusV2 payload_v2 = {
        .present_ctl_temperature = 5678,
        .present_ctl_delta_uv    = 0,
        .target_ctl_temperature  = 1,
        .target_ctl_delat_uv     = 2,
        .remaining_time          = 3,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v2 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v2,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV2),
    };

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwmCold(Lightness, payload_v2.present_ctl_temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwmWarm(Lightness, payload_v2.present_ctl_temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwmCold(Lightness, payload_v2.present_ctl_temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwmWarm(Lightness, payload_v2.present_ctl_temperature));

    Luminaire_MeshMessageHandler(&frame_v2);
}

void test_MeshMessageHandlerLightCtlTemperatureStatusBadLength(void)
{
    MessageHandlerConfig.instance_index = 1;

    struct UartProtocolFrameMeshMessageLightCtlTempStatusV1 payload_v1 = {
        .present_ctl_temperature = 5678,
        .present_ctl_delta_uv    = 0,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v1 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v1,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV1) - 1,
    };

    Luminaire_MeshMessageHandler(&frame_v1);
}

void test_MeshMessageHandlerLightCtlTemperatureStatusUnknownInstanceIndex(void)
{
    struct UartProtocolFrameMeshMessageLightCtlTempStatusV1 payload_v1 = {
        .present_ctl_temperature = 5678,
        .present_ctl_delta_uv    = 0,
    };

    struct UartProtocolFrameMeshMessageFrame frame_v1 = {
        .instance_index     = 2,
        .sub_index          = 3,
        .mesh_opcode        = UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS,
        .p_mesh_msg_payload = (uint8_t *)&payload_v1,
        .mesh_msg_len       = sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV1),
    };

    Luminaire_MeshMessageHandler(&frame_v1);
}

void test_ProcessLightnessAttentionEnable(void)
{
    uint16_t lightness            = 1234;
    IsAttentionSequenceInProgress = true;

    Luminaire_ProcessLightness(lightness);
}

void test_ProcessLightnessAttentionDisable(void)
{
    uint16_t lightness            = 1234;
    IsAttentionSequenceInProgress = false;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_ProcessLightness(lightness);
}

void test_ProcessTemperatureAttentionEnable(void)
{
    uint16_t temperature          = 5678;
    IsAttentionSequenceInProgress = true;
    IsCtlInitialized              = true;

    Luminaire_ProcessTemperature(temperature);
}

void test_ProcessTemperatureAttentionDisable(void)
{
    Lightness                     = 1234;
    uint16_t temperature          = 5678;
    IsAttentionSequenceInProgress = false;
    IsCtlInitialized              = true;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwmCold(Lightness, temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwmWarm(Lightness, temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwmCold(Lightness, temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwmWarm(Lightness, temperature));

    Luminaire_ProcessTemperature(temperature);
}

void test_SetOutputLc(void)
{
    IsCtlInitialized = false;
    Lightness        = 1234;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(Lightness));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));

    Luminaire_SetOutput();
}

void test_SetOutputCtl(void)
{
    IsCtlInitialized = true;
    Lightness        = 1234;
    Temperature      = 5678;

    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwmCold(Lightness, Temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwmWarm(Lightness, Temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwmCold(Lightness, Temperature));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwmWarm(Lightness, Temperature));

    Luminaire_SetOutput();
}

void test_ConvertLightnessToLinear(void)
{
    uint16_t i;
    for (i = 0; i < UINT16_MAX; i++)
    {
        double expected_result = ceil((double)i * (double)i / (double)UINT16_MAX);
        TEST_ASSERT_EQUAL((uint16_t)expected_result, Luminaire_ConvertLightnessToLinear(i));
    }
}

void test_ProcessStartupSequence(void)
{
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(0);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    Luminaire_ProcessStartupSequence();

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_2_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_2_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    Luminaire_ProcessStartupSequence();

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_3_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_3_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    Luminaire_ProcessStartupSequence();

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    Luminaire_ProcessStartupSequence();

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 - 1);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    Luminaire_ProcessStartupSequence();

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000 - 1);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000 - 1);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_EOL_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_EOL_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    Luminaire_ProcessStartupSequence();

    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000);
    Timestamp_GetCurrent_ExpectAndReturn(0);
    Timestamp_GetTimeElapsed_ExpectAnyArgsAndReturn(3000 + 1000 + 1000 + 1000 + 30000);
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_IDLE_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM, LightnessToPwm(0));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_COLD_1_10V, LightnessToPwm(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_IDLE_LIGHTNESS));
    PwmHal_SetPwmDuty_Expect(PWM_HAL_CHANNEL_WARM_1_10V, LightnessToPwm(0));
    SimpleScheduler_TaskStateChange_Expect(SIMPLE_SCHEDULER_TASK_ID_SENSOR_INPUT, true);
    Luminaire_ProcessStartupSequence();
}
