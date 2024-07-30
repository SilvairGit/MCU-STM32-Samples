#include "TimeReceiver.h"

#include "Assert.h"
#include "Log.h"
#include "SimpleScheduler.h"
#include "Timestamp.h"
#include "UartProtocol.h"

#define MESH_TIME_TASK_PERIOD_MS 1
#define SYNC_TIME_PERIOD_MS (1000 * 60)

static void SendTimeGetReq(void);
static void ProcessTimeGetResponse(uint8_t *p_payload, uint8_t len);
static void LoopTimeReceiver(void);
static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame);

static const enum UartFrameCmd UartFrameCommandList[] = {
    UART_FRAME_CMD_TIME_GET_RESP,
};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = UartMessageHandler,
    .p_mesh_message_handler       = NULL,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = NULL,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = 0,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};


static struct TimeReceiver_MeshTimeLastSync LastSyncTime   = {0};
static bool                                 IsInitialized  = false;
static uint8_t                             *pInstanceIndex = NULL;

void TimeReceiver_Init(uint8_t *p_instance_index)
{
    SimpleScheduler_TaskAdd(MESH_TIME_TASK_PERIOD_MS, LoopTimeReceiver, SIMPLE_SCHEDULER_TASK_ID_TIME_SYNC, false);

    pInstanceIndex = p_instance_index;

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    IsInitialized = true;
}

bool TimeReceiver_IsInitialized(void)
{
    return IsInitialized;
}

struct TimeReceiver_MeshTimeLastSync *TimeReceiver_GetLastSyncTime(void)
{
    return &LastSyncTime;
}

static void SendTimeGetReq(void)
{
    struct TimeGetReq msg = {0};
    msg.instance_index    = *pInstanceIndex;

    UartProtocol_Send(UART_FRAME_CMD_TIME_GET_REQ, (uint8_t *)&msg, sizeof(msg));
}

static void ProcessTimeGetResponse(uint8_t *p_payload, uint8_t len)
{
    if ((p_payload == NULL) || (len != sizeof(struct TimeGetResp)))
    {
        return;
    }

    struct TimeGetResp *msg = (struct TimeGetResp *)p_payload;

    LastSyncTime.local_sync_timestamp_ms = Timestamp_GetCurrent();
    LastSyncTime.tai_seconds             = msg->tai_seconds;
    LastSyncTime.subsecond               = msg->subsecond;
    LastSyncTime.tai_utc_delta           = msg->tai_utc_delta;
    LastSyncTime.time_zone_offset        = msg->time_zone_offset;
}

static void LoopTimeReceiver(void)
{
    static uint32_t last_sync_time_ms = 0;

    if (Timestamp_GetTimeElapsed(last_sync_time_ms, Timestamp_GetCurrent()) > SYNC_TIME_PERIOD_MS)
    {
        last_sync_time_ms += SYNC_TIME_PERIOD_MS;

        SendTimeGetReq();
    }
}

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_TIME_GET_RESP:
            ProcessTimeGetResponse(p_frame->p_payload, p_frame->len);
            break;

        default:
            break;
    }
}
