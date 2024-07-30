#include <string.h>

#include "EmgLTest.c"
#include "MockEmergencyDriverSimulator.h"
#include "MockUartProtocol.h"
#include "unity.h"


#define INSTANCE_INDEX 0x12
#define SUB_INDEX 0x00
#define EL_SERVER_OPCODE_BE 0x0136EA
#define EL_TEST_SERVER_OPCODE_BE 0x0136E9
#define EXPECTED_RESPONSE_BUF_LEN 30

#define HI_UINT16(a) (((a)&0xFF00) >> 8)
#define LO_UINT16(a) ((a)&0x00FF)
#define LOWEST_UINT32(a) ((a)&0x000000FF)
#define LO_UINT32(a) (((a) >> 8) & 0x000000FF)
#define HI_UINT32(a) (((a) >> 16) & 0x000000FF)
#define HIGHEST_UINT32(a) (((a) >> 24) & 0x000000FF)


uint8_t  ExpectedMeshMsqReqBuf[EXPECTED_RESPONSE_BUF_LEN];
uint8_t  ExpectedMeshMsqReqLen;
uint32_t ExpectedMeshMsqReqOpcode;

uint8_t ExpectedUartCmdBuf[EXPECTED_RESPONSE_BUF_LEN];
uint8_t ExpectedUartCmdLen;
uint8_t ExpectedUartCmdId;


static void UartProtocol_Send_StubCbk(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len, int cmock_num_calls);
static void UartProtocol_SendFrame_StubCbk(struct UartFrameRxTxFrame *p_frame, int cmock_num_calls);
static void GetElState_Return(enum EmgLTest_ElState el_state);
static void ExpectMeshMsqReq(uint8_t *buf, uint8_t buf_len, uint32_t opcode);
static void ExpectUartCmd(uint8_t *buf, uint8_t buf_len, uint8_t cmd_id);


void setUp(void)
{
    EmergencyDriverSimulator_IsInitialized_IgnoreAndReturn(false);
    EmergencyDriverSimulator_Init_Ignore();
    UartProtocol_RegisterMessageHandler_Ignore();
    UartProtocol_SendFrame_StubWithCallback(UartProtocol_SendFrame_StubCbk);
    UartProtocol_Send_StubWithCallback(UartProtocol_Send_StubCbk);

    memset(ExpectedMeshMsqReqBuf, 0x00, EXPECTED_RESPONSE_BUF_LEN);
    ExpectedMeshMsqReqLen    = 0;
    ExpectedMeshMsqReqOpcode = 0;

    memset(ExpectedUartCmdBuf, 0x00, EXPECTED_RESPONSE_BUF_LEN);
    ExpectedUartCmdLen = 0;
    ExpectedUartCmdId  = 0;

    MessageHandlerConfig.instance_index = INSTANCE_INDEX;
}

void cleanUp(void)
{
    MessageHandlerConfig.instance_index = INSTANCE_INDEX;
}

void test_Init(void)
{
    IsInitialized = false;
    TEST_ASSERT_EQUAL(false, EmgLTest_IsInitialized());

    EmergencyDriverSimulator_Dtr_Expect(0);
    EmergencyDriverSimulator_StoreFunctionTestInterval_Expect();
    EmergencyDriverSimulator_Dtr_Expect(0);
    EmergencyDriverSimulator_StoreDurationTestInterval_Expect();

    EmgLTest_Init();
    TEST_ASSERT_EQUAL(true, EmgLTest_IsInitialized());
}

void test_ElInhibitEnter(void)
{
    uint8_t payload[] = {
        0x00,    // EL Inhibit Enter
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    EmergencyDriverSimulator_Inhibit_Expect();
    GetElState_Return(EMG_L_TEST_EL_STATE_INHIBIT);

    uint8_t response[] = {
        0x05,    // El State Status
        0x0A,    // Inhibit
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElInhibitEnterWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x00,    // EL Inhibit Enter
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_ElInhibitExitWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x02,    // EL Inhibit Exit
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_ElInhibitExitInInhibit(void)
{
    uint8_t payload[] = {
        0x02,    // EL Inhibit Exit
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };
    GetElState_Return(EMG_L_TEST_EL_STATE_INHIBIT);
    EmergencyDriverSimulator_ReLightResetInhibit_Expect();
    GetElState_Return(EMG_L_TEST_EL_STATE_NORMAL);

    uint8_t response[] = {
        0x05,    // El State Status
        0x03,    // Normal
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElInhibitExitInRest(void)
{
    uint8_t payload[] = {
        0x02,    // EL Inhibit Exit
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };
    GetElState_Return(EMG_L_TEST_EL_STATE_REST);
    GetElState_Return(EMG_L_TEST_EL_STATE_REST);

    uint8_t response[] = {
        0x05,    // El State Status
        0x08,    // Rest
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElRestEnter(void)
{
    uint8_t payload[] = {
        0x0E,    // EL Rest Enter
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    EmergencyDriverSimulator_Rest_Expect();
    GetElState_Return(EMG_L_TEST_EL_STATE_REST);

    uint8_t response[] = {
        0x05,    // El State Status
        0x08,    // Rest
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElRestEnterWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x0E,    // EL Rest Enter
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_ElRestExitInRest(void)
{
    uint8_t payload[] = {
        0x10,    // EL Rest Exit
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };
    GetElState_Return(EMG_L_TEST_EL_STATE_REST);
    EmergencyDriverSimulator_ReLightResetInhibit_Expect();
    GetElState_Return(EMG_L_TEST_EL_STATE_EMERGENCY);

    uint8_t response[] = {
        0x05,    // El State Status
        0x05,    // Emergency
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElRestExitWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x10,    // EL Rest Exit
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_ElRestExitInInhibit(void)
{
    uint8_t payload[] = {
        0x10,    // EL Rest Exit
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };
    GetElState_Return(EMG_L_TEST_EL_STATE_INHIBIT);
    GetElState_Return(EMG_L_TEST_EL_STATE_INHIBIT);

    uint8_t response[] = {
        0x05,    // El State Status
        0x0A,    // Inhibit
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElStateGet(void)
{
    uint8_t payload[] = {
        0x04,    // EL State Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    GetElState_Return(EMG_L_TEST_EL_STATE_NORMAL);

    uint8_t response[] = {
        0x05,    // El State Status
        0x03,    // Normal
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElStateGetWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x04,    // EL State Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_ElPropertyStatus_ELLightness(void)
{
    uint16_t property_val[] = {
        0,
        515,
        516,
        65535,
    };
    uint8_t expected_emergency_level[] = {
        1,
        1,
        2,
        254,
    };

    for (size_t i = 0; i < ARRAY_SIZE(property_val); i++)
    {
        uint8_t payload[] = {
            0x09,                          // EL Property Status
            0x80,                          // Property ID: EL Lightness
            0xFF,                          // Property ID: EL Lightness
            LO_UINT16(property_val[i]),    // Property value
            HI_UINT16(property_val[i]),    // Property value
        };
        struct UartProtocolFrameMeshMessageFrame frame = {
            .instance_index     = INSTANCE_INDEX,
            .sub_index          = SUB_INDEX,
            .mesh_opcode        = EL_SERVER_OPCODE,
            .p_mesh_msg_payload = payload,
            .mesh_msg_len       = sizeof(payload),
        };

        EmergencyDriverSimulator_QueryEmergencyMinLevel_ExpectAndReturn(1);
        EmergencyDriverSimulator_QueryEmergencyMaxLevel_ExpectAndReturn(254);

        EmergencyDriverSimulator_Dtr_Expect(expected_emergency_level[i]);
        EmergencyDriverSimulator_StoreDtrAsEmergencyLevel_Expect();

        MeshMessageHandler(&frame);
    }
}

void test_ElPropertyStatus_ELProlongTime(void)
{
    uint16_t property_val[] = {
        0,
        29,
        30,
        7649,
        7650,
        65535,
    };
    uint8_t expected_emergency_level[] = {
        0,
        0,
        1,
        254,
        255,
        255,
    };

    for (size_t i = 0; i < ARRAY_SIZE(property_val); i++)
    {
        uint8_t payload[] = {
            0x09,                          // EL Property Status
            0x83,                          // Property ID: EL Lightness
            0xFF,                          // Property ID: EL Lightness
            LO_UINT16(property_val[i]),    // Property value
            HI_UINT16(property_val[i]),    // Property value
        };
        struct UartProtocolFrameMeshMessageFrame frame = {
            .instance_index     = INSTANCE_INDEX,
            .sub_index          = SUB_INDEX,
            .mesh_opcode        = EL_SERVER_OPCODE,
            .p_mesh_msg_payload = payload,
            .mesh_msg_len       = sizeof(payload),
        };

        EmergencyDriverSimulator_Dtr_Expect(expected_emergency_level[i]);
        EmergencyDriverSimulator_StoreProlongTime_Expect();

        MeshMessageHandler(&frame);
    }
}

void test_ElOperationTimeGet(void)
{
    uint8_t emergency_time_val[] = {
        0,
        1,
        255,
    };
    uint8_t total_operation_time_val[] = {
        0,
        1,
        255,
    };
    uint32_t expected_el_total_operation_time[] = {
        0,
        14400,
        UINT32_MAX,
    };
    uint32_t expected_el_emergency_time[] = {
        0,
        3600,
        UINT32_MAX,
    };

    for (size_t i = 0; i < ARRAY_SIZE(emergency_time_val); i++)
    {
        uint8_t payload[] = {
            0x0A,    // EL Operation Time Get
        };
        struct UartProtocolFrameMeshMessageFrame frame = {
            .instance_index     = INSTANCE_INDEX,
            .sub_index          = SUB_INDEX,
            .mesh_opcode        = EL_SERVER_OPCODE,
            .p_mesh_msg_payload = payload,
            .mesh_msg_len       = sizeof(payload),
        };

        EmergencyDriverSimulator_QueryLampEmergencyTime_ExpectAndReturn(emergency_time_val[i]);
        EmergencyDriverSimulator_QueryLampTotalOperationTime_ExpectAndReturn(total_operation_time_val[i]);

        uint8_t response[] = {
            0x0D,    // El Operation Time Status
            LOWEST_UINT32(expected_el_total_operation_time[i]),
            LO_UINT32(expected_el_total_operation_time[i]),
            HI_UINT32(expected_el_total_operation_time[i]),
            HIGHEST_UINT32(expected_el_total_operation_time[i]),
            LOWEST_UINT32(expected_el_emergency_time[i]),
            LO_UINT32(expected_el_emergency_time[i]),
            HI_UINT32(expected_el_emergency_time[i]),
            HIGHEST_UINT32(expected_el_emergency_time[i]),
        };
        ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

        MeshMessageHandler(&frame);
    }
}

void test_ElOperationTimeGetWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x0A,    // EL Operation Time Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_ElOperationTimeClear(void)
{
    uint8_t payload[] = {
        0x0B,    // EL Operation Time Clear
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    EmergencyDriverSimulator_ResetLampTime_Expect();

    EmergencyDriverSimulator_QueryLampEmergencyTime_ExpectAndReturn(0);
    EmergencyDriverSimulator_QueryLampTotalOperationTime_ExpectAndReturn(0);

    uint8_t response[] = {
        0x0D,    // El Operation Time Status
        0x00,    // EL Total Operation Time
        0x00,    // EL Total Operation Time
        0x00,    // EL Total Operation Time
        0x00,    // EL Total Operation Time
        0x00,    // EL Emergency Time
        0x00,    // EL Emergency Time
        0x00,    // EL Emergency Time
        0x00,    // EL Emergency Time
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_ElOperationTimeClearWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x0B,    // EL Operation Time Clear
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestStart_FromNormal(void)
{
    uint8_t payload[] = {
        0x01,    // ELT Functional Test Start
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    GetElState_Return(EMG_L_TEST_EL_STATE_NORMAL);
    EmergencyDriverSimulator_StartFunctionTest_Expect();
    GetElState_Return(EMG_L_TEST_EL_STATE_FUNCTIONAL_TEST_IN_PROGRESS);

    uint8_t response[] = {
        0x05,    // El State Status
        0x0E,    // Functional Test In Progress
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestStart_FromBatteryDischarged(void)
{
    uint8_t payload[] = {
        0x01,    // ELT Functional Test Start
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    GetElState_Return(EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED);

    uint8_t response[] = {
        0x05,    // El State Status
        0x0F,    // Battery Discharged
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestStartWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x01,    // ELT Functional Test Start
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestStop(void)
{
    uint8_t payload[] = {
        0x02,    // ELT Functional Test Stop
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    EmergencyDriverSimulator_StopTest_Expect();

    GetElState_Return(EMG_L_TEST_EL_STATE_NORMAL);

    uint8_t response[] = {
        0x05,    // El State Status
        0x03,    // Normal
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestStopWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x02,    // ELT Functional Test Stop
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestGet_FinishedWithFailure(void)
{
    uint8_t payload[] = {
        0x00,    // ELT Functional Test Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    uint8_t                 emergency_status                     = 0;
    struct EmergencyStatus *p_emergency_status                   = (struct EmergencyStatus *)&emergency_status;
    p_emergency_status->function_test_done_and_results_available = true;
    EmergencyDriverSimulator_QueryEmergencyStatus_ExpectAndReturn(emergency_status);

    uint8_t               failure_status             = 0;
    struct FailureStatus *p_failure_status           = (struct FailureStatus *)&failure_status;
    p_failure_status->emergency_control_gear_failure = true;
    p_failure_status->battery_failure                = true;
    EmergencyDriverSimulator_QueryFailureStatus_ExpectAndReturn(failure_status);

    uint8_t response[] = {
        0x03,    // El Functional Test Status
        0x00,    // Status: Test finished
        0x06,    // Result: Battery fault, Circuit fault
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_TEST_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestGetWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x00,    // ELT Functional Test Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_EltFunctionalTestGet_UnknownNoFaults(void)
{
    uint8_t payload[] = {
        0x00,    // ELT Functional Test Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    uint8_t emergency_status = 0;
    EmergencyDriverSimulator_QueryEmergencyStatus_ExpectAndReturn(emergency_status);

    uint8_t failure_status = 0;
    EmergencyDriverSimulator_QueryFailureStatus_ExpectAndReturn(failure_status);

    uint8_t response[] = {
        0x03,    // El Functional Test Status
        0x07,    // Status: Test result unknown
        0x00,    // Result: No faults
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_TEST_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltDurationTestStart_FromNormal(void)
{
    uint8_t payload[] = {
        0x05,    // ELT Duration Test Start
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    GetElState_Return(EMG_L_TEST_EL_STATE_NORMAL);
    EmergencyDriverSimulator_StartDurationTest_Expect();
    GetElState_Return(EMG_L_TEST_EL_STATE_DURATION_TEST_IN_PROGRESS);

    uint8_t response[] = {
        0x05,    // El State Status
        0x0C,    // Duration Test In Progress
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltDurationTestStart_FromBatteryDischarged(void)
{
    uint8_t payload[] = {
        0x05,    // ELT Duration Test Start
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    GetElState_Return(EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED);

    uint8_t response[] = {
        0x05,    // El State Status
        0x0F,    // Battery Discharged
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltDurationTestStartWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x05,    // ELT Duration Test Start
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_EltDurationTestStop(void)
{
    uint8_t payload[] = {
        0x06,    // ELT Duration Test Stop
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    EmergencyDriverSimulator_StopTest_Expect();

    GetElState_Return(EMG_L_TEST_EL_STATE_NORMAL);

    uint8_t response[] = {
        0x05,    // El State Status
        0x03,    // Normal
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltDurationTestStopWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x06,    // ELT Duration Test Stop
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_EltDurationTestGet_FinishedWithFailure(void)
{
    uint8_t payload[] = {
        0x04,    // ELT Duration Test Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    uint8_t                 emergency_status                     = 0;
    struct EmergencyStatus *p_emergency_status                   = (struct EmergencyStatus *)&emergency_status;
    p_emergency_status->duration_test_done_and_results_available = true;
    EmergencyDriverSimulator_QueryEmergencyStatus_ExpectAndReturn(emergency_status);

    uint8_t               failure_status             = 0;
    struct FailureStatus *p_failure_status           = (struct FailureStatus *)&failure_status;
    p_failure_status->emergency_control_gear_failure = true;
    p_failure_status->battery_failure                = true;
    EmergencyDriverSimulator_QueryFailureStatus_ExpectAndReturn(failure_status);

    EmergencyDriverSimulator_QueryDurationTestResult_ExpectAndReturn(10);

    uint8_t response[] = {
        0x07,    // El Duration Test Status
        0x00,    // Status: Test finished
        0x06,    // Result: Battery fault, Circuit fault
        0xB0,    // Test Length
        0x04,    // Test Length
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_TEST_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltDurationTestGet_UnknownNoFaults(void)
{
    uint8_t payload[] = {
        0x04,    // ELT Duration Test Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    uint8_t emergency_status = 0;
    EmergencyDriverSimulator_QueryEmergencyStatus_ExpectAndReturn(emergency_status);

    uint8_t failure_status = 0;
    EmergencyDriverSimulator_QueryFailureStatus_ExpectAndReturn(failure_status);

    EmergencyDriverSimulator_QueryDurationTestResult_ExpectAndReturn(0);

    uint8_t response[] = {
        0x07,    // El Duration Test Status
        0x07,    // Status: Test result unknown
        0x00,    // Result: No faults
        0x00,    // Test Length
        0x00,    // Test Length
    };
    ExpectMeshMsqReq(response, sizeof(response), EL_TEST_SERVER_OPCODE_BE);

    MeshMessageHandler(&frame);
}

void test_EltDurationTestGetWithInstanceIndexUnknown(void)
{
    MessageHandlerConfig.instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

    uint8_t payload[] = {
        0x04,    // ELT Duration Test Get
    };
    struct UartProtocolFrameMeshMessageFrame frame = {
        .instance_index     = INSTANCE_INDEX,
        .sub_index          = SUB_INDEX,
        .mesh_opcode        = EL_TEST_SERVER_OPCODE,
        .p_mesh_msg_payload = payload,
        .mesh_msg_len       = sizeof(payload),
    };

    MeshMessageHandler(&frame);
}

void test_UpdateBatteryStatus(void)
{
    EmergencyDriverSimulator_QueryBatteryCharge_ExpectAndReturn(254);

    uint8_t expected_cmd       = 0x26;    // BatteryStatusSetRequest
    uint8_t expected_payload[] = {
        INSTANCE_INDEX,    // instanceIndex
        0x64,              // batteryLevel
        0xFF,              // timeToDischarge
        0xFF,              // timeToDischarge
        0xFF,              // timeToDischarge
        0xFF,              // timeToCharge
        0xFF,              // timeToCharge
        0xFF,              // timeToCharge
        0x5A,              // flags
    };
    ExpectUartCmd(expected_payload, sizeof(expected_payload), expected_cmd);

    UpdateBatteryStatus();
}

static void UartProtocol_Send_StubCbk(enum UartFrameCmd cmd, uint8_t *p_payload, uint8_t len, int cmock_num_calls)
{
    UNUSED(cmock_num_calls);

    TEST_ASSERT_EQUAL(ExpectedUartCmdId, cmd);
    TEST_ASSERT_EQUAL(ExpectedUartCmdLen, len);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(ExpectedUartCmdBuf, p_payload, ExpectedUartCmdLen);
}

static void UartProtocol_SendFrame_StubCbk(struct UartFrameRxTxFrame *p_frame, int cmock_num_calls)
{
    UNUSED(cmock_num_calls);

    struct UartProtocolFrameMeshMessageRequest1Opcode3B *p_response = (struct UartProtocolFrameMeshMessageRequest1Opcode3B *)p_frame;

    TEST_ASSERT_EQUAL(5 + ExpectedMeshMsqReqLen, p_response->len);
    TEST_ASSERT_EQUAL(UART_FRAME_CMD_MESH_MESSAGE_REQUEST1, p_response->cmd);
    TEST_ASSERT_EQUAL(INSTANCE_INDEX, p_response->instance_index);
    TEST_ASSERT_EQUAL(SUB_INDEX, p_response->sub_index);
    TEST_ASSERT_EQUAL(ExpectedMeshMsqReqOpcode, p_response->mesh_opcode_be);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(ExpectedMeshMsqReqBuf, p_response->p_data, ExpectedMeshMsqReqLen);
}

static void GetElState_Return(enum EmgLTest_ElState el_state)
{
    // Function will always return Normal mode
    uint8_t em_status   = 0;
    uint8_t em_mode     = 0;
    uint8_t batt_charge = 100;

    switch (el_state)
    {
        case EMG_L_TEST_EL_STATE_REST:
        {
            em_mode = 0x01;
            break;
        }

        case EMG_L_TEST_EL_STATE_EMERGENCY:
        {
            em_mode = 0x04;
            break;
        }
        case EMG_L_TEST_EL_STATE_EXTENDED_EMERGENCY:
        {
            em_mode = 0x08;
            break;
        }
        case EMG_L_TEST_EL_STATE_FUNCTIONAL_TEST_IN_PROGRESS:
        {
            em_mode = 0x10;
            break;
        }
        case EMG_L_TEST_EL_STATE_DURATION_TEST_IN_PROGRESS:
        {
            em_mode = 0x20;
            break;
        }
        case EMG_L_TEST_EL_STATE_INHIBIT:
        {
            em_status = 0x01;
            em_mode   = 0x02;
            break;
        }
        case EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED:
        {
            em_mode     = 0x02;
            batt_charge = 0;
            break;
        }
        case EMG_L_TEST_EL_STATE_NORMAL:
        default:
        {
            em_mode = 0x02;
            break;
        }
    }
    EmergencyDriverSimulator_QueryEmergencyStatus_ExpectAndReturn(em_status);
    EmergencyDriverSimulator_QueryEmergencyMode_ExpectAndReturn(em_mode);
    EmergencyDriverSimulator_QueryBatteryCharge_ExpectAndReturn(batt_charge);
}

static void ExpectMeshMsqReq(uint8_t *buf, uint8_t buf_len, uint32_t opcode)
{
    ExpectedMeshMsqReqLen = buf_len;
    memcpy(ExpectedMeshMsqReqBuf, buf, ExpectedMeshMsqReqLen);
    ExpectedMeshMsqReqOpcode = opcode;
}

static void ExpectUartCmd(uint8_t *buf, uint8_t buf_len, uint8_t cmd_id)
{
    ExpectedUartCmdLen = buf_len;
    memcpy(ExpectedUartCmdBuf, buf, ExpectedUartCmdLen);
    ExpectedUartCmdId = cmd_id;
}
