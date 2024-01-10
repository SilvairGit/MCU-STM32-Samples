#include "Luminaire.h"

#include <stdint.h>

#include "Assert.h"
#include "Config.h"
#include "Log.h"
#include "ModelManager.h"
#include "PwmHal.h"
#include "SimpleScheduler.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Utils.h"

#define LUMINAIRE_TASK_PERIOD_MS 1000

#define LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K 2700
#define LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K 6500

#define LUMINAIRE_ATTENTION_LIGHTNESS_HIGH UINT16_MAX
#define LUMINAIRE_ATTENTION_LIGHTNESS_LOW ((UINT16_MAX * 4) / 10)

#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_DURATION_MS 3000
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_2_DURATION_MS 1000
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_3_DURATION_MS 1000
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_DURATION_MS 1000
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_EOL_DURATION_MS 30000

#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_LIGHTNESS 0xB504
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_2_LIGHTNESS 0x18FF
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_3_LIGHTNESS 0xFFFF
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_LIGHTNESS 0x18FF
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_EOL_LIGHTNESS 0xFFFF
#define LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_IDLE_LIGHTNESS 0xFFFF

static void Luminaire_MeshMessageHandler(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void Luminaire_MeshMessageLightLStatus(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void Luminaire_MeshMessageLightCtlTempStatus(struct UartProtocolFrameMeshMessageFrame *p_frame);

static void     Luminaire_Loop(void);
static void     Luminaire_ProcessLightness(uint16_t lightness);
static void     Luminaire_ProcessTemperature(uint16_t temperature);
static void     Luminaire_SetOutput(void);
static uint16_t Luminaire_ConvertLightnessToLinear(uint16_t lightness);
static void     Luminaire_StopStartupSequence(void);
static void     Luminaire_ProcessStartupSequence(void);

static bool IsInitialized                 = false;
static bool IsCtlInitialized              = false;
static bool IsAttentionSequenceInProgress = false;
static bool IsStartupBehaviorInProgress   = false;

static uint16_t Lightness        = 0;
static uint16_t Temperature      = 0;
static uint16_t CurrentDutyCycle = 0;

static uint32_t StartupSequenceStartTimestamp = 0;

static const uint32_t MeshMessageOpcodeList[] = {
    UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS,
    UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS,
};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = NULL,
    .p_mesh_message_handler       = Luminaire_MeshMessageHandler,
    .p_uart_frame_command_list    = NULL,
    .p_mesh_message_opcode_list   = MeshMessageOpcodeList,
    .uart_frame_command_list_len  = 0,
    .mesh_message_opcode_list_len = ARRAY_SIZE(MeshMessageOpcodeList),
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

static const struct ModelParametersLightCtlServer CtlRegistrationParameters = {
    .min_temperature_range = LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K,
    .max_temperature_range = LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K,
};

struct ModelManagerRegistrationRow ModelConfigLightCtlServer = {
    .model_id            = MODEL_MANAGER_ID_LIGHT_CTL_SERVER,
    .p_model_parameter   = (uint8_t *)&CtlRegistrationParameters,
    .model_parameter_len = sizeof(CtlRegistrationParameters),
    .p_instance_index    = &MessageHandlerConfig.instance_index,
};

struct ModelManagerRegistrationRow ModelConfigLightLcServer = {
    .model_id            = MODEL_MANAGER_ID_LIGHT_LC_SERVER,
    .p_model_parameter   = NULL,
    .model_parameter_len = 0,
    .p_instance_index    = &MessageHandlerConfig.instance_index,
};

void Luminaire_Init(enum LuminaireInitMode init_mode)
{
    ASSERT(!IsInitialized && ((init_mode == LUMINAIRE_INIT_MODE_LIGHT_LC) || (init_mode == LUMINAIRE_INIT_MODE_LIGHT_CTL)));

    LOG_D("Luminaire initialization");

    if (!PwmHal_IsInitialized())
    {
        PwmHal_Init();
    }

    if (!UartProtocol_IsInitialized())
    {
        UartProtocol_Init();
    }

    if (init_mode == LUMINAIRE_INIT_MODE_LIGHT_LC)
    {
        LOG_D("LC initialization");

        ModelManager_RegisterModel(&ModelConfigLightLcServer);
    }
    else if (init_mode == LUMINAIRE_INIT_MODE_LIGHT_CTL)
    {
        LOG_D("CTL initialization");

        ModelManager_RegisterModel(&ModelConfigLightCtlServer);
        IsCtlInitialized = true;
    }

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    SimpleScheduler_TaskAdd(LUMINAIRE_TASK_PERIOD_MS, Luminaire_Loop, SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);

    IsInitialized = true;
}

bool Luminaire_IsInitialized(void)
{
    return IsInitialized;
}

void Luminaire_IndicateAttention(bool is_attention_enable, bool led_state)
{
    if (IsAttentionSequenceInProgress && !is_attention_enable)
    {
        Luminaire_SetOutput();
    }

    IsAttentionSequenceInProgress = is_attention_enable;

    if (!IsAttentionSequenceInProgress)
    {
        return;
    }

    // Buffer actual lightness
    uint16_t lightness_before_attention = Lightness;

    // Set attention lightness according to LED state
    if (led_state)
    {
        Lightness = LUMINAIRE_ATTENTION_LIGHTNESS_HIGH;
    }
    else
    {
        Lightness = LUMINAIRE_ATTENTION_LIGHTNESS_LOW;
    }

    Luminaire_SetOutput();

    // Restore buffered lightness
    Lightness = lightness_before_attention;
}

void Luminaire_StartStartupBehavior(void)
{
    StartupSequenceStartTimestamp = Timestamp_GetCurrent();

    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, true);

    Luminaire_ProcessStartupSequence();
}


void Luminaire_StopStartupBehavior(void)
{
    Luminaire_StopStartupSequence();
    IsStartupBehaviorInProgress = false;
}

uint16_t Luminaire_GetDutyCycle(void)
{
    return CurrentDutyCycle;
}

bool Luminaire_IsStartupBehaviorInProgress(void)
{
    return IsStartupBehaviorInProgress;
}

static void Luminaire_MeshMessageHandler(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    if ((MessageHandlerConfig.instance_index == UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN) && !IsStartupBehaviorInProgress)
    {
        return;
    }

    switch (p_frame->mesh_opcode)
    {
        case UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS:
            Luminaire_MeshMessageLightLStatus(p_frame);
            break;

        case UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS:
            Luminaire_MeshMessageLightCtlTempStatus(p_frame);
            break;

        default:
            break;
    }
}

static void Luminaire_MeshMessageLightLStatus(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (!((p_frame->mesh_msg_len == sizeof(struct UartProtocolFrameMeshMessageLightLStatusV1)) ||
          (p_frame->mesh_msg_len == sizeof(struct UartProtocolFrameMeshMessageLightLStatusV2))))
    {
        return;
    }

    // We are interested only in obligatory fields
    struct UartProtocolFrameMeshMessageLightLStatusV1 *p_rx_frame = (struct UartProtocolFrameMeshMessageLightLStatusV1 *)p_frame->p_mesh_msg_payload;

    Luminaire_ProcessLightness(p_rx_frame->present_lightness);
}

static void Luminaire_MeshMessageLightCtlTempStatus(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (!((p_frame->mesh_msg_len == sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV1)) ||
          (p_frame->mesh_msg_len == sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV2))))
    {
        return;
    }

    // We are interested only in obligatory fields
    struct UartProtocolFrameMeshMessageLightCtlTempStatusV1 *p_rx_frame = (struct UartProtocolFrameMeshMessageLightCtlTempStatusV1 *)
                                                                              p_frame->p_mesh_msg_payload;

    Luminaire_ProcessTemperature(p_rx_frame->present_ctl_temperature);
}

static void Luminaire_Loop(void)
{
    Luminaire_ProcessStartupSequence();
}

static void Luminaire_ProcessLightness(uint16_t lightness)
{
    Lightness = lightness;

    if (IsAttentionSequenceInProgress)
    {
        return;
    }

    Luminaire_SetOutput();
}

static void Luminaire_ProcessTemperature(uint16_t temperature)
{
    Temperature = temperature;

    if (IsAttentionSequenceInProgress)
    {
        return;
    }

    Luminaire_SetOutput();
}

static void Luminaire_SetOutput(void)
{
    CurrentDutyCycle = Luminaire_ConvertLightnessToLinear(Lightness);
    uint32_t pwm_out = (uint32_t)CurrentDutyCycle * PWM_HAL_OUTPUT_DUTY_CYCLE_MAX / UINT16_MAX;

    if (IsCtlInitialized)
    {
        if (Temperature == 0)
        {
            return;
        }

        uint32_t warm;
        warm = (LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K - Temperature) * pwm_out;
        warm /= LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K - LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K;

        uint32_t cold;
        cold = (Temperature - LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K) * pwm_out;
        cold /= LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MAX_K - LUMINAIRE_LIGHT_CTL_TEMP_RANGE_MIN_K;

        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_COLD, cold);
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_WARM, warm);
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_COLD_1_10V, cold);
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_WARM_1_10V, warm);
    }
    else
    {
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_COLD, pwm_out);
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_WARM, 0);
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_COLD_1_10V, pwm_out);
        PwmHal_SetPwmDuty(PWM_HAL_CHANNEL_WARM_1_10V, 0);
    }
}

static uint16_t Luminaire_ConvertLightnessToLinear(uint16_t lightness)
{
    uint32_t pow_val = (uint32_t)lightness * (uint32_t)lightness;
    return (pow_val + UINT16_MAX - 1) / UINT16_MAX;
}

static void Luminaire_StopStartupSequence(void)
{
    SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_LIGHT_LIGHTNESS, false);
    Luminaire_ProcessLightness(LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_IDLE_LIGHTNESS);
}

static void Luminaire_ProcessStartupSequence(void)
{
    const uint32_t startup_sequence_stage_duration_ms[] = {
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_DURATION_MS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_2_DURATION_MS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_3_DURATION_MS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_DURATION_MS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_EOL_DURATION_MS,
    };

    const uint16_t startup_sequence_lightness[] = {
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_1_LIGHTNESS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_2_LIGHTNESS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_3_LIGHTNESS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_4_LIGHTNESS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_EOL_LIGHTNESS,
        LUMINAIRE_LIGHT_STARTUP_SEQENCE_STAGE_IDLE_LIGHTNESS,
    };

    uint32_t accumulated_stage_duration_time_ms = 0;
    size_t   detected_stage                     = ARRAY_SIZE(startup_sequence_stage_duration_ms);

    size_t i;
    for (i = 0; i < ARRAY_SIZE(startup_sequence_stage_duration_ms); i++)
    {
        accumulated_stage_duration_time_ms += startup_sequence_stage_duration_ms[i];

        if (Timestamp_GetTimeElapsed(StartupSequenceStartTimestamp, Timestamp_GetCurrent()) < accumulated_stage_duration_time_ms)
        {
            detected_stage = i;
            break;
        }
    }

    if (detected_stage == ARRAY_SIZE(startup_sequence_stage_duration_ms))
    {
        Luminaire_StopStartupSequence();
        IsStartupBehaviorInProgress = true;
        SimpleScheduler_TaskStateChange(SIMPLE_SCHEDULER_TASK_ID_SENSOR_INPUT, true);
        return;
    }

    Luminaire_ProcessLightness(startup_sequence_lightness[detected_stage]);
}
