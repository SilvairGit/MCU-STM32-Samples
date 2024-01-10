#include "Mesh.h"

#include <stdlib.h>

#include "Assert.h"
#include "Config.h"
#include "EmgLTest.h"
#include "Log.h"
#include "Luminaire.h"
#include "SimpleScheduler.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Utils.h"

#define MESH_TASK_PERIOD_MS 1

/**
 * Used Mesh Messages len
 */
#define MESH_MESSAGE_LIGHT_L_GET_LEN 4
#define MESH_MESSAGE_GENERIC_ONOFF_SET_LEN 8
#define MESH_MESSAGE_LIGHT_L_SET_LEN 9
#define MESH_MESSAGE_GENERIC_DELTA_SET_LEN 11
#define MESH_MESSAGE_GENERIC_LEVEL_SET_LEN 9

/*
 * Mesh time conversion definitions
 */
#define MESH_NUMBER_OF_MS_IN_100_MS (100UL)
#define MESH_NUMBER_OF_MS_IN_1S (10 * MESH_NUMBER_OF_MS_IN_100_MS)
#define MESH_NUMBER_OF_MS_IN_10S (10 * MESH_NUMBER_OF_MS_IN_1S)
#define MESH_NUMBER_OF_MS_IN_10MIN (60 * MESH_NUMBER_OF_MS_IN_10S)
#define MESH_TRANSITION_TIME_STEP_RESOLUTION_100_MS 0x00
#define MESH_TRANSITION_TIME_STEP_RESOLUTION_1_S 0x40
#define MESH_TRANSITION_TIME_STEP_RESOLUTION_10_S 0x80
#define MESH_TRANSITION_TIME_STEP_RESOLUTION_10_MIN 0xC0
#define MESH_TRANSITION_TIME_NUMBER_OF_STEPS_MASK 0x3F
#define MESH_TRANSITION_TIME_NUMBER_OF_STEPS_UNKNOWN_VALUE 0x3F
#define MESH_DELAY_TIME_STEP_MS 5

/**
 * Default communication properties
 */
#define MESH_REPEATS_INTERVAL_MS 20
#define MESH_MESSAGES_QUEUE_LENGTH 10


enum MsgType
{
    GENERIC_ON_OFF_SET_MSG,
    GENERIC_DELTA_SET_MSG,
    LIGHT_L_SET_MSG,
    GENERIC_LEVEL_SET_MSG
};

struct GenericOnOffSetMsg
{
    uint8_t onoff;
    uint8_t tid;
    uint8_t transition_time;
    uint8_t delay;
};

struct GenericDeltaSetMsg
{
    uint32_t delta_level;
    uint8_t  tid;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct LightLSetMsg
{
    uint16_t lightness;
    uint8_t  tid;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct GenericLevelSetMsg
{
    int16_t value;
    uint8_t tid;
    uint8_t transition_time;
    uint8_t delay;
};

struct EnqueuedMsg
{
    enum MsgType msg_type;
    uint8_t      instance_idx;
    void *       mesh_msg;
    uint32_t     dispatch_time;
};

struct EnqueuedMsg *MeshMsgsQueue[MESH_MESSAGES_QUEUE_LENGTH];

static void    SendGenericOnOffSet(uint8_t instance_idx, struct GenericOnOffSetMsg *message);
static void    SendLightLSet(uint8_t instance_idx, struct LightLSetMsg *message);
static void    SendGenericLevelSet(uint8_t instance_idx, struct GenericLevelSetMsg *message);
static void    SendGenericDeltaSet(uint8_t instance_idx, struct GenericDeltaSetMsg *message);
static uint8_t ConvertFromMsToMeshFormat(uint32_t time_ms);

static void Mesh_Loop(void);

static bool IsInitialized = false;

void Mesh_Init(void)
{
    SimpleScheduler_TaskAdd(MESH_TASK_PERIOD_MS, Mesh_Loop, SIMPLE_SCHEDULER_TASK_ID_MESH, true);

    IsInitialized = true;
}

bool Mesh_IsInitialized(void)
{
    return IsInitialized;
}

bool Mesh_IsModelAvailable(uint8_t *p_payload, uint8_t len, uint16_t expected_model_id)
{
    size_t index;
    for (index = 0; index < len;)
    {
        uint16_t model_id = ((uint16_t)p_payload[index++]);
        model_id |= ((uint16_t)p_payload[index++] << 8);

        if (expected_model_id == model_id)
        {
            return true;
        }
    }
    return false;
}

void Mesh_SendGenericOnOffSet(uint8_t instance_idx, bool value, uint32_t transition_time, uint32_t delay_ms, uint8_t num_of_repeats, uint8_t tid)
{
    Mesh_SendGenericOnOffSetWithRepeatsInterval(instance_idx, value, transition_time, delay_ms, num_of_repeats, MESH_REPEATS_INTERVAL_MS, tid);
}


void Mesh_SendGenericOnOffSetWithRepeatsInterval(uint8_t  instance_idx,
                                                 bool     value,
                                                 uint32_t transition_time,
                                                 uint32_t delay_ms,
                                                 uint8_t  num_of_repeats,
                                                 uint16_t repeats_interval_ms,
                                                 uint8_t  tid)
{
    size_t i;
    for (i = 0; i <= num_of_repeats; i++)
    {
        struct GenericOnOffSetMsg *p_msg = (struct GenericOnOffSetMsg *)calloc(1, sizeof(struct GenericOnOffSetMsg));
        p_msg->onoff                     = value;
        p_msg->tid                       = tid;
        p_msg->transition_time           = ConvertFromMsToMeshFormat(transition_time);
        p_msg->delay                     = ((num_of_repeats - i) * repeats_interval_ms + delay_ms) / MESH_DELAY_TIME_STEP_MS;

        struct EnqueuedMsg *p_enqueued_msg = (struct EnqueuedMsg *)calloc(1, sizeof(struct EnqueuedMsg));
        p_enqueued_msg->instance_idx       = instance_idx;
        p_enqueued_msg->mesh_msg           = p_msg;
        p_enqueued_msg->dispatch_time      = Timestamp_GetCurrent() + i * repeats_interval_ms;
        p_enqueued_msg->msg_type           = GENERIC_ON_OFF_SET_MSG;

        size_t j;
        for (j = 0; j < MESH_MESSAGES_QUEUE_LENGTH; j++)
        {
            if (MeshMsgsQueue[j] == NULL)
            {
                MeshMsgsQueue[j] = p_enqueued_msg;
                break;
            }
        }
    }
}

void Mesh_SendGenericLevelSet(uint8_t instance_idx, uint16_t value, uint32_t transition_time, uint32_t delay_ms, uint8_t num_of_repeats, uint8_t tid)
{
    uint16_t delay_interval = delay_ms / (num_of_repeats + 1);
    uint32_t t              = Timestamp_GetCurrent();

    size_t i;
    for (i = 0; i <= num_of_repeats; i++)
    {
        struct GenericLevelSetMsg *p_msg = (struct GenericLevelSetMsg *)calloc(1, sizeof(struct GenericLevelSetMsg));
        p_msg->value                     = value;
        p_msg->tid                       = tid;
        p_msg->transition_time           = ConvertFromMsToMeshFormat(transition_time);
        p_msg->delay                     = ((num_of_repeats - i) * delay_interval) / MESH_DELAY_TIME_STEP_MS;

        struct EnqueuedMsg *p_enqueued_msg = (struct EnqueuedMsg *)calloc(1, sizeof(struct EnqueuedMsg));
        p_enqueued_msg->instance_idx       = instance_idx;
        p_enqueued_msg->mesh_msg           = p_msg;
        p_enqueued_msg->dispatch_time      = t + i * delay_interval;
        p_enqueued_msg->msg_type           = GENERIC_LEVEL_SET_MSG;

        size_t j;
        for (j = 0; j < MESH_MESSAGES_QUEUE_LENGTH; j++)
        {
            if (MeshMsgsQueue[j] == NULL)
            {
                MeshMsgsQueue[j] = p_enqueued_msg;
                break;
            }
        }
    }
}

void Mesh_SendGenericDeltaSet(uint8_t instance_idx, int32_t value, uint32_t transition_time, uint32_t delay_ms, uint8_t num_of_repeats, uint8_t tid)
{
    Mesh_SendGenericDeltaSetWithRepeatsInterval(instance_idx, value, transition_time, delay_ms, num_of_repeats, MESH_REPEATS_INTERVAL_MS, tid);
}

void Mesh_SendGenericDeltaSetWithRepeatsInterval(uint8_t  instance_idx,
                                                 int32_t  value,
                                                 uint32_t transition_time,
                                                 uint32_t delay_ms,
                                                 uint8_t  num_of_repeats,
                                                 uint16_t repeats_interval_ms,
                                                 uint8_t  tid)
{
    size_t i;
    for (i = 0; i <= num_of_repeats; i++)
    {
        struct GenericDeltaSetMsg *p_msg = (struct GenericDeltaSetMsg *)calloc(1, sizeof(struct GenericDeltaSetMsg));
        p_msg->delta_level               = value;
        p_msg->tid                       = tid;
        p_msg->transition_time           = ConvertFromMsToMeshFormat(transition_time);
        p_msg->delay                     = ((num_of_repeats - i) * repeats_interval_ms + delay_ms) / MESH_DELAY_TIME_STEP_MS;

        struct EnqueuedMsg *p_enqueued_msg = (struct EnqueuedMsg *)calloc(1, sizeof(struct EnqueuedMsg));
        p_enqueued_msg->instance_idx       = instance_idx;
        p_enqueued_msg->mesh_msg           = p_msg;
        p_enqueued_msg->dispatch_time      = Timestamp_GetCurrent() + i * repeats_interval_ms;
        p_enqueued_msg->msg_type           = GENERIC_DELTA_SET_MSG;

        size_t j;
        for (j = 0; j < MESH_MESSAGES_QUEUE_LENGTH; j++)
        {
            if (MeshMsgsQueue[j] == NULL)
            {
                MeshMsgsQueue[j] = p_enqueued_msg;
                break;
            }
        }
    }
}

void Mesh_SendGenericDeltaSetWithDispatchTime(uint8_t  instance_idx,
                                              int32_t  value,
                                              uint32_t transition_time,
                                              uint32_t delay_ms,
                                              uint16_t dispatch_time_ms,
                                              uint8_t  tid)
{
    struct GenericDeltaSetMsg *p_msg = (struct GenericDeltaSetMsg *)calloc(1, sizeof(struct GenericDeltaSetMsg));
    p_msg->delta_level               = value;
    p_msg->tid                       = tid;
    p_msg->transition_time           = ConvertFromMsToMeshFormat(transition_time);
    p_msg->delay                     = delay_ms / MESH_DELAY_TIME_STEP_MS;

    struct EnqueuedMsg *p_enqueued_msg = (struct EnqueuedMsg *)calloc(1, sizeof(struct EnqueuedMsg));
    p_enqueued_msg->instance_idx       = instance_idx;
    p_enqueued_msg->mesh_msg           = p_msg;
    p_enqueued_msg->dispatch_time      = Timestamp_GetCurrent() + dispatch_time_ms;
    p_enqueued_msg->msg_type           = GENERIC_DELTA_SET_MSG;

    size_t i;
    for (i = 0; i < MESH_MESSAGES_QUEUE_LENGTH; i++)
    {
        if (MeshMsgsQueue[i] == NULL)
        {
            MeshMsgsQueue[i] = p_enqueued_msg;
            break;
        }
    }
}

void Mesh_IncrementTid(uint8_t *p_tid)
{
    ASSERT(p_tid != NULL);
    *p_tid = *p_tid + 1;
}

static void SendGenericOnOffSet(uint8_t instance_idx, struct GenericOnOffSetMsg *message)
{
    uint8_t buf[MESH_MESSAGE_GENERIC_ONOFF_SET_LEN];
    size_t  index = 0;

    buf[index++] = instance_idx;
    buf[index++] = 0x00;
    buf[index++] = LOW_BYTE(MESH_MESSAGE_GENERIC_ONOFF_SET_UNACKNOWLEDGED);
    buf[index++] = HIGH_BYTE(MESH_MESSAGE_GENERIC_ONOFF_SET_UNACKNOWLEDGED);
    buf[index++] = message->onoff;
    buf[index++] = message->tid;
    buf[index++] = message->transition_time;
    buf[index++] = message->delay;

    UartProtocol_Send(UART_FRAME_CMD_MESH_MESSAGE_REQUEST, buf, sizeof(buf));
}

static void SendLightLSet(uint8_t instance_idx, struct LightLSetMsg *message)
{
    uint8_t buf[MESH_MESSAGE_LIGHT_L_SET_LEN];
    size_t  index = 0;

    buf[index++] = instance_idx;
    buf[index++] = 0x00;
    buf[index++] = LOW_BYTE(MESH_MESSAGE_LIGHT_L_SET_UNACKNOWLEDGED);
    buf[index++] = HIGH_BYTE(MESH_MESSAGE_LIGHT_L_SET_UNACKNOWLEDGED);
    buf[index++] = LOW_BYTE(message->lightness);
    buf[index++] = HIGH_BYTE(message->lightness);
    buf[index++] = message->tid;
    buf[index++] = message->transition_time;
    buf[index++] = message->delay;

    UartProtocol_Send(UART_FRAME_CMD_MESH_MESSAGE_REQUEST, buf, sizeof(buf));
}

static void SendGenericLevelSet(uint8_t instance_idx, struct GenericLevelSetMsg *message)
{
    uint8_t buf[MESH_MESSAGE_GENERIC_LEVEL_SET_LEN];
    size_t  index = 0;

    buf[index++] = instance_idx;
    buf[index++] = 0x00;
    buf[index++] = LOW_BYTE(MESH_MESSAGE_GENERIC_LEVEL_SET_UNACKNOWLEDGED);
    buf[index++] = HIGH_BYTE(MESH_MESSAGE_GENERIC_LEVEL_SET_UNACKNOWLEDGED);
    buf[index++] = LOW_BYTE(message->value);
    buf[index++] = HIGH_BYTE(message->value);
    buf[index++] = message->tid;
    buf[index++] = message->transition_time;
    buf[index++] = message->delay;

    UartProtocol_Send(UART_FRAME_CMD_MESH_MESSAGE_REQUEST, buf, sizeof(buf));
}

static void SendGenericDeltaSet(uint8_t instance_idx, struct GenericDeltaSetMsg *message)
{
    uint8_t buf[MESH_MESSAGE_GENERIC_DELTA_SET_LEN];
    size_t  index = 0;

    buf[index++] = instance_idx;
    buf[index++] = 0x00;
    buf[index++] = LOW_BYTE(MESH_MESSAGE_GENERIC_DELTA_SET_UNACKNOWLEDGED);
    buf[index++] = HIGH_BYTE(MESH_MESSAGE_GENERIC_DELTA_SET_UNACKNOWLEDGED);
    buf[index++] = ((message->delta_level >> 0) & 0xFF);
    buf[index++] = ((message->delta_level >> 8) & 0xFF);
    buf[index++] = ((message->delta_level >> 16) & 0xFF);
    buf[index++] = ((message->delta_level >> 24) & 0xFF);
    buf[index++] = message->tid;
    buf[index++] = message->transition_time;
    buf[index++] = message->delay;

    UartProtocol_Send(UART_FRAME_CMD_MESH_MESSAGE_REQUEST, buf, sizeof(buf));
}

static uint8_t ConvertFromMsToMeshFormat(uint32_t time_ms)
{
    if ((time_ms / MESH_NUMBER_OF_MS_IN_100_MS) < MESH_TRANSITION_TIME_NUMBER_OF_STEPS_UNKNOWN_VALUE)
    {
        return (MESH_TRANSITION_TIME_STEP_RESOLUTION_100_MS | (time_ms / MESH_NUMBER_OF_MS_IN_100_MS));
    }
    else if ((time_ms / MESH_NUMBER_OF_MS_IN_1S) < MESH_TRANSITION_TIME_NUMBER_OF_STEPS_UNKNOWN_VALUE)
    {
        return (MESH_TRANSITION_TIME_STEP_RESOLUTION_1_S | (time_ms / MESH_NUMBER_OF_MS_IN_1S));
    }
    else if ((time_ms / MESH_NUMBER_OF_MS_IN_10S) < MESH_TRANSITION_TIME_NUMBER_OF_STEPS_UNKNOWN_VALUE)
    {
        return (MESH_TRANSITION_TIME_STEP_RESOLUTION_10_S | (time_ms / MESH_NUMBER_OF_MS_IN_10S));
    }
    else if ((time_ms / MESH_NUMBER_OF_MS_IN_10MIN) < MESH_TRANSITION_TIME_NUMBER_OF_STEPS_UNKNOWN_VALUE)
    {
        return (MESH_TRANSITION_TIME_STEP_RESOLUTION_10_MIN | (time_ms / MESH_NUMBER_OF_MS_IN_10MIN));
    }
    else
    {
        return (MESH_TRANSITION_TIME_STEP_RESOLUTION_10_MIN | (MESH_TRANSITION_TIME_NUMBER_OF_STEPS_UNKNOWN_VALUE & MESH_TRANSITION_TIME_NUMBER_OF_STEPS_MASK));
    }
}


static void Mesh_Loop(void)
{
    size_t i;
    for (i = 0; i < MESH_MESSAGES_QUEUE_LENGTH; i++)
    {
        if (MeshMsgsQueue[i] == NULL)
            continue;
        if (Timestamp_Compare(Timestamp_GetCurrent(), MeshMsgsQueue[i]->dispatch_time))
            continue;

        switch (MeshMsgsQueue[i]->msg_type)
        {
            case GENERIC_ON_OFF_SET_MSG:
            {
                SendGenericOnOffSet(MeshMsgsQueue[i]->instance_idx, (struct GenericOnOffSetMsg *)MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]);
                MeshMsgsQueue[i] = NULL;
                break;
            }
            case GENERIC_DELTA_SET_MSG:
            {
                SendGenericDeltaSet(MeshMsgsQueue[i]->instance_idx, (struct GenericDeltaSetMsg *)MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]);
                MeshMsgsQueue[i] = NULL;
                break;
            }
            case LIGHT_L_SET_MSG:
            {
                SendLightLSet(MeshMsgsQueue[i]->instance_idx, (struct LightLSetMsg *)MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]);
                MeshMsgsQueue[i] = NULL;
                break;
            }
            case GENERIC_LEVEL_SET_MSG:
            {
                SendGenericLevelSet(MeshMsgsQueue[i]->instance_idx, (struct GenericLevelSetMsg *)MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]->mesh_msg);
                free(MeshMsgsQueue[i]);
                MeshMsgsQueue[i] = NULL;
                break;
            }
            default:
                break;
        }
    }
}