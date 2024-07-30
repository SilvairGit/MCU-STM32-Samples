#include <string.h>

#include "Mesh.h"
#include "MockAssert.h"
#include "MockTimestamp.h"
#include "MockUartFrame.h"
#include "UartProtocol.c"
#include "unity.h"

static void UartMessageHandler1(struct UartFrameRxTxFrame *p_frame);
static void UartMessageHandler2(struct UartFrameRxTxFrame *p_frame);
static void UartMessageHandler3(struct UartFrameRxTxFrame *p_frame);
static void UartMeshMessageHandler1(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void UartMeshMessageHandler2(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void UartMeshMessageHandler3(struct UartProtocolFrameMeshMessageFrame *p_frame);

static const uint8_t UartFrameCommandList1[] = {
    UART_FRAME_CMD_ATTENTION_EVENT,
    UART_FRAME_CMD_SOFTWARE_RESET_REQUEST,
};

static const uint32_t UartMeshMessageOpcodeList1[] = {
    0x0034 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
    0x0078 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
    0x00CD | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
};

static struct UartProtocolHandlerConfig MessageHandlerConfig1 = {
    .p_uart_frame_command_list    = UartFrameCommandList1,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList1),
    .p_uart_message_handler       = UartMessageHandler1,
    .p_mesh_message_opcode_list   = UartMeshMessageOpcodeList1,
    .mesh_message_opcode_list_len = ARRAY_SIZE(UartMeshMessageOpcodeList1),
    .p_mesh_message_handler       = UartMeshMessageHandler1,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

static const uint8_t UartFrameCommandList2[] = {
    UART_FRAME_CMD_ATTENTION_EVENT,
    UART_FRAME_CMD_START_TEST_REQ,
    UART_FRAME_CMD_SENSOR_UPDATE_REQUEST,
    UART_FRAME_CMD_CLEAR_FAULT_REQUEST,
};

static const uint32_t UartMeshMessageOpcodeList2[] = {
    0x0A | (UART_PROTOCOL_MESH_OPCODE_SIZE_1_OCTET_MASK << 0),
    0x00AA | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
    0x00AAAA | (UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK << 16),
    0x0B | (UART_PROTOCOL_MESH_OPCODE_SIZE_1_OCTET_MASK << 0),
    0x00BB | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
    0x00BBBB | (UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK << 16),
    0x0056 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
};

static struct UartProtocolHandlerConfig MessageHandlerConfig2 = {
    .p_uart_frame_command_list    = UartFrameCommandList2,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList2),
    .p_uart_message_handler       = UartMessageHandler2,
    .p_mesh_message_opcode_list   = UartMeshMessageOpcodeList2,
    .mesh_message_opcode_list_len = ARRAY_SIZE(UartMeshMessageOpcodeList2),
    .p_mesh_message_handler       = UartMeshMessageHandler2,
    .instance_index               = 7,
};

static const uint8_t UartFrameCommandList3[] = {
    UART_FRAME_CMD_ATTENTION_EVENT,
    UART_FRAME_CMD_START_TEST_REQ,
    UART_FRAME_CMD_SENSOR_UPDATE_REQUEST,
    UART_FRAME_CMD_START_TEST_RESP,
};

static const uint32_t UartMeshMessageOpcodeList3[] = {
    0x0B | (UART_PROTOCOL_MESH_OPCODE_SIZE_1_OCTET_MASK << 0),
    0x00BB | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
    0x00BBBB | (UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK << 16),
    0x0056 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
    0x0076 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
};

static struct UartProtocolHandlerConfig MessageHandlerConfig3 = {
    .p_uart_frame_command_list    = UartFrameCommandList3,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList3),
    .p_uart_message_handler       = UartMessageHandler3,
    .p_mesh_message_opcode_list   = UartMeshMessageOpcodeList3,
    .mesh_message_opcode_list_len = ARRAY_SIZE(UartMeshMessageOpcodeList3),
    .p_mesh_message_handler       = UartMeshMessageHandler3,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

static uint8_t UartMessageExpetedCmd1 = 0;
static uint8_t UartMessageExpetedCmd2 = 0;
static uint8_t UartMessageExpetedCmd3 = 0;

static uint32_t UartMeshMessageExpetedOpcode1 = 0;
static uint32_t UartMeshMessageExpetedOpcode2 = 0;
static uint32_t UartMeshMessageExpetedOpcode3 = 0;

static struct UartFrameRxTxFrame *RxFrame;
static size_t                     RxFrameSize = 0;

static void UartMessageHandler1(struct UartFrameRxTxFrame *p_frame)
{
    struct UartFrameRxTxFrame *p_rx_frame = (struct UartFrameRxTxFrame *)p_frame;

    UartMessageExpetedCmd1 = p_rx_frame->cmd;
}

static void UartMessageHandler2(struct UartFrameRxTxFrame *p_frame)
{
    struct UartFrameRxTxFrame *p_rx_frame = (struct UartFrameRxTxFrame *)p_frame;

    UartMessageExpetedCmd2 = p_rx_frame->cmd;
}

static void UartMessageHandler3(struct UartFrameRxTxFrame *p_frame)
{
    struct UartFrameRxTxFrame *p_rx_frame = (struct UartFrameRxTxFrame *)p_frame;

    UartMessageExpetedCmd3 = p_rx_frame->cmd;
}

static void UartMeshMessageHandler1(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    UartMeshMessageExpetedOpcode1 = p_frame->mesh_opcode;

    uint8_t mesh_msg[] = {0x12, 0x34, 0x56};

    TEST_ASSERT_EQUAL(p_frame->instance_index, 7);
    TEST_ASSERT_EQUAL(p_frame->sub_index, 0xAB);
    TEST_ASSERT_EQUAL(p_frame->mesh_msg_len, 3);
    TEST_ASSERT_TRUE(memcmp(p_frame->p_mesh_msg_payload, mesh_msg, p_frame->mesh_msg_len) == 0);
}

static void UartMeshMessageHandler2(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    UartMeshMessageExpetedOpcode2 = p_frame->mesh_opcode;

    uint8_t mesh_msg[] = {0x12, 0x34, 0x56};

    TEST_ASSERT_EQUAL(p_frame->instance_index, 7);
    TEST_ASSERT_EQUAL(p_frame->sub_index, 0xAB);
    TEST_ASSERT_EQUAL(p_frame->mesh_msg_len, 3);
    TEST_ASSERT_TRUE(memcmp(p_frame->p_mesh_msg_payload, mesh_msg, p_frame->mesh_msg_len) == 0);
}

static void UartMeshMessageHandler3(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    UartMeshMessageExpetedOpcode3 = p_frame->mesh_opcode;

    uint8_t mesh_msg[] = {0x12, 0x34, 0x56};

    TEST_ASSERT_EQUAL(p_frame->instance_index, 7);
    TEST_ASSERT_EQUAL(p_frame->sub_index, 0xAB);
    TEST_ASSERT_EQUAL(p_frame->mesh_msg_len, 3);
    TEST_ASSERT_TRUE(memcmp(p_frame->p_mesh_msg_payload, mesh_msg, p_frame->mesh_msg_len) == 0);
}

void setUp(void)
{
    UartMessageExpetedCmd1 = 0;
    UartMessageExpetedCmd2 = 0;
    UartMessageExpetedCmd3 = 0;

    UartMeshMessageExpetedOpcode1 = 0;
    UartMeshMessageExpetedOpcode2 = 0;
    UartMeshMessageExpetedOpcode3 = 0;

    HandlerConfigCnt = 0;
}

void test_Init(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, UartProtocol_IsInitialized());

    Timestamp_DelayMs_Expect(UART_PROTOCOL_FRAME_TIMEOUT_ELAPSED_MS);
    Timestamp_DelayMs_Expect(1000 - UART_PROTOCOL_FRAME_TIMEOUT_ELAPSED_MS);
    UartFrame_IsInitialized_ExpectAndReturn(false);
    UartFrame_Init_Expect();

    UartProtocol_Init();

    TEST_ASSERT_EQUAL(true, UartProtocol_IsInitialized());
}

void test_IsInitialized(void)
{
    IsInitialized = false;

    TEST_ASSERT_EQUAL(false, UartProtocol_IsInitialized());

    IsInitialized = true;

    TEST_ASSERT_EQUAL(true, UartProtocol_IsInitialized());
}

void test_RegisterMessageHandler(void)
{
    struct UartProtocolHandlerConfig config1;
    struct UartProtocolHandlerConfig config2;

    UartProtocol_RegisterMessageHandler(&config1);
    UartProtocol_RegisterMessageHandler(&config2);

    TEST_ASSERT_EQUAL(2, HandlerConfigCnt);

    TEST_ASSERT_EQUAL(&config1, HandlerConfig[0]);
    TEST_ASSERT_EQUAL(&config2, HandlerConfig[1]);
}

void test_RegisterMaxMessageHandler(void)
{
    struct UartProtocolHandlerConfig config1;

    size_t i;
    for (i = 0; i < UART_PROTOCOL_MAX_NUMBER_OF_HANDLERS; i++)
    {
        UartProtocol_RegisterMessageHandler(&config1);
    }

    Assert_Callback_ExpectAnyArgs();
    UartProtocol_RegisterMessageHandler(&config1);
}

void test_CheckIfInstanceIndexExist(void)
{
    uint8_t instance_index;

    struct UartFrameRxTxFrame frame1 = {.len = 0, .cmd = UART_FRAME_CMD_MESH_MESSAGE_REQUEST, .p_payload = {1}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame1);
    TEST_ASSERT_EQUAL(instance_index, 1);

    struct UartFrameRxTxFrame frame2 = {.len = 0, .cmd = UART_FRAME_CMD_MESH_MESSAGE_RESPONSE, .p_payload = {2}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame2);
    TEST_ASSERT_EQUAL(instance_index, 2);

    struct UartFrameRxTxFrame frame3 = {.len = 0, .cmd = UART_FRAME_CMD_SENSOR_UPDATE_REQUEST, .p_payload = {3}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame3);
    TEST_ASSERT_EQUAL(instance_index, 3);

    struct UartFrameRxTxFrame frame4 = {.len = 0, .cmd = UART_FRAME_CMD_SET_FAULT_REQUEST, .p_payload = {0x00, 0x00, 0x00, 4}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame4);
    TEST_ASSERT_EQUAL(instance_index, 4);

    struct UartFrameRxTxFrame frame5 = {.len = 0, .cmd = UART_FRAME_CMD_CLEAR_FAULT_REQUEST, .p_payload = {0x00, 0x00, 0x00, 5}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame5);
    TEST_ASSERT_EQUAL(instance_index, 5);

    struct UartFrameRxTxFrame frame6 = {.len = 0, .cmd = UART_FRAME_CMD_START_TEST_REQ, .p_payload = {0x00, 0x00, 0x00, 6}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame6);
    TEST_ASSERT_EQUAL(instance_index, 6);

    struct UartFrameRxTxFrame frame7 = {.len = 0, .cmd = UART_FRAME_CMD_TEST_FINISHED_REQ, .p_payload = {0x00, 0x00, 0x00, 7}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame7);
    TEST_ASSERT_EQUAL(instance_index, 7);

    struct UartFrameRxTxFrame frame8 = {.len = 0, .cmd = UART_FRAME_CMD_BATTERY_STATUS_SET_REQ, .p_payload = {8}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame8);
    TEST_ASSERT_EQUAL(instance_index, 8);

    struct UartFrameRxTxFrame frame9 = {.len = 0, .cmd = UART_FRAME_CMD_MESH_MESSAGE_REQUEST1, .p_payload = {9}};
    instance_index                   = UartProtocol_CheckIfInstanceIndexExist(&frame9);
    TEST_ASSERT_EQUAL(instance_index, 9);

    struct UartFrameRxTxFrame frame10 = {.len = 0, .cmd = UART_FRAME_CMD_TIME_SOURCE_SET_REQ, .p_payload = {10}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame10);
    TEST_ASSERT_EQUAL(instance_index, 10);

    struct UartFrameRxTxFrame frame11 = {.len = 0, .cmd = UART_FRAME_CMD_TIME_SOURCE_SET_RESP, .p_payload = {11}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame11);
    TEST_ASSERT_EQUAL(instance_index, 11);

    struct UartFrameRxTxFrame frame12 = {.len = 0, .cmd = UART_FRAME_CMD_TIME_SOURCE_GET_REQ, .p_payload = {12}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame12);
    TEST_ASSERT_EQUAL(instance_index, 12);

    struct UartFrameRxTxFrame frame13 = {.len = 0, .cmd = UART_FRAME_CMD_TIME_SOURCE_GET_RESP, .p_payload = {13}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame13);
    TEST_ASSERT_EQUAL(instance_index, 13);

    struct UartFrameRxTxFrame frame14 = {.len = 0, .cmd = UART_FRAME_CMD_TIME_GET_REQ, .p_payload = {14}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame14);
    TEST_ASSERT_EQUAL(instance_index, 14);

    struct UartFrameRxTxFrame frame15 = {.len = 0, .cmd = UART_FRAME_CMD_TIME_GET_RESP, .p_payload = {15}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame15);
    TEST_ASSERT_EQUAL(instance_index, 15);

    struct UartFrameRxTxFrame frame16 = {.len = 0, .cmd = UART_FRAME_CMD_ATTENTION_EVENT, .p_payload = {16}};
    instance_index                    = UartProtocol_CheckIfInstanceIndexExist(&frame16);
    TEST_ASSERT_EQUAL(instance_index, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
}

bool UartFrame_ProcessIncomingData_StubCbk(struct UartFrameRxTxFrame *p_rx_frame, int cmock_num_calls)
{
    memcpy(p_rx_frame, RxFrame, RxFrameSize);

    UNUSED(cmock_num_calls);

    return true;
}

void test_ProcessIncomingDataInstanceIndexMach(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    struct UartProtocolFrameClearFaultRequest rx_frame = {
        .len            = 0,
        .cmd            = UART_FRAME_CMD_CLEAR_FAULT_REQUEST,
        .instance_index = 7,
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame);

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, UART_FRAME_CMD_CLEAR_FAULT_REQUEST);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataInstanceIndexUnknown(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    struct UartProtocolFrameClearFaultRequest rx_frame = {
        .len            = 0,
        .cmd            = UART_FRAME_CMD_START_TEST_RESP,
        .instance_index = 7,
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame);

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, UART_FRAME_CMD_START_TEST_RESP);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataInstanceIndexNotMach(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    struct UartProtocolFrameClearFaultRequest rx_frame = {
        .len            = 0,
        .cmd            = UART_FRAME_CMD_CLEAR_FAULT_REQUEST,
        .instance_index = 8,
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame);

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataUartMessageMatch(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    struct UartProtocolFrameSoftResetRequest rx_frame = {
        .len = 0,
        .cmd = UART_FRAME_CMD_SOFTWARE_RESET_REQUEST,
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame);

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, UART_FRAME_CMD_SOFTWARE_RESET_REQUEST);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataTwoUartMessageMatch(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    struct UartProtocolFrameSensorUpdateRequest rx_frame = {
        .len            = 0,
        .cmd            = UART_FRAME_CMD_SENSOR_UPDATE_REQUEST,
        .instance_index = 7,
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame);

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, UART_FRAME_CMD_SENSOR_UPDATE_REQUEST);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, UART_FRAME_CMD_SENSOR_UPDATE_REQUEST);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataMeshMessageRequestMatch(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    static struct UartProtocolFrameMeshMessageRequest rx_frame = {
        .len            = 4 + 3,
        .cmd            = UART_FRAME_CMD_MESH_MESSAGE_REQUEST,
        .instance_index = 7,
        .sub_index      = 0xAB,
        .mesh_opcode    = 0x0076 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
        .p_data         = {0x12, 0x34, 0x56},
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame) + 3;

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0x0076 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8));
}

void test_ProcessIncomingDataTwoMeshMessageRequestMatch(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    static struct UartProtocolFrameMeshMessageRequest rx_frame = {
        .len            = 4 + 3,
        .cmd            = UART_FRAME_CMD_MESH_MESSAGE_REQUEST,
        .instance_index = 7,
        .sub_index      = 0xAB,
        .mesh_opcode    = 0x0056 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8),
        .p_data         = {0x12, 0x34, 0x56},
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame) + 3;

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0x0056 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8));
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0x0056 | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8));
}

void test_ProcessIncomingDataMeshMessageRequest1Match1B(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    static struct UartProtocolFrameMeshMessageRequest1Opcode1B rx_frame = {
        .len            = 3 + 3,
        .cmd            = UART_FRAME_CMD_MESH_MESSAGE_REQUEST1,
        .instance_index = 7,
        .sub_index      = 0xAB,
        .mesh_opcode    = 0x0A | (UART_PROTOCOL_MESH_OPCODE_SIZE_1_OCTET_MASK << 0),
        .p_data         = {0x12, 0x34, 0x56},
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame) + 3;

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0x0A | (UART_PROTOCOL_MESH_OPCODE_SIZE_1_OCTET_MASK << 0));
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataMeshMessageRequest1Match2(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    static struct UartProtocolFrameMeshMessageRequest1Opcode2B rx_frame = {
        .len            = 4 + 3,
        .cmd            = UART_FRAME_CMD_MESH_MESSAGE_REQUEST1,
        .instance_index = 7,
        .sub_index      = 0xAB,
        .mesh_opcode_be = SWAP_2_BYTES(0x00AA | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8)),
        .p_data         = {0x12, 0x34, 0x56},
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame) + 3;

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0x00AA | (UART_PROTOCOL_MESH_OPCODE_SIZE_2_OCTET_MASK << 8));
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}

void test_ProcessIncomingDataMeshMessageRequest1Match3B(void)
{
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig1);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig2);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig3);

    static struct UartProtocolFrameMeshMessageRequest1Opcode3B rx_frame = {
        .len            = 5 + 3,
        .cmd            = UART_FRAME_CMD_MESH_MESSAGE_REQUEST1,
        .instance_index = 7,
        .sub_index      = 0xAB,
        .mesh_opcode_be = SWAP_3_BYTES(0x00AAAA | (UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK << 16)),
        .p_data         = {0x12, 0x34, 0x56},
    };

    RxFrame     = (struct UartFrameRxTxFrame *)&rx_frame;
    RxFrameSize = sizeof(rx_frame) + 3;

    UartFrame_ProcessIncomingData_StubWithCallback(UartFrame_ProcessIncomingData_StubCbk);
    UartProtocol_ProcessIncomingData();

    TEST_ASSERT_EQUAL(UartMessageExpetedCmd1, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd2, 0);
    TEST_ASSERT_EQUAL(UartMessageExpetedCmd3, 0);

    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode1, 0);
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode2, 0x00AAAA | (UART_PROTOCOL_MESH_OPCODE_SIZE_3_OCTET_MASK << 16));
    TEST_ASSERT_EQUAL(UartMeshMessageExpetedOpcode3, 0);
}
