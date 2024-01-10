#include "EmgLTest.h"

#include <string.h>

#include "Assert.h"
#include "EmergencyDriverSimulator.h"
#include "Log.h"
#include "MeshGenericBattery.h"
#include "ModelManager.h"
#include "SimpleScheduler.h"
#include "UartProtocol.h"
#include "UartProtocolTypes.h"
#include "Utils.h"


#define EMERGENCY_LIGHTING_TESTING_TASK_PERIOD_MS 1000

#define EL_SERVER_OPCODE 0x00EA3601
#define EL_TEST_SERVER_OPCODE 0x00E93601
#define EMG_SUBOPCODE_SIZE 1
#define MESH_OPCODE_SIZE_3_OCTET_MASK 0xC0

#define DURATION_TEST_RESULT_STEP_S 120
#define PROLONG_TIME_STEP_S 30
#define LAMP_EMERGENCY_TIME_STEP_S (60 * 60)
#define LAMP_TOTAL_OPERATION_TIME_STEP_S (60 * 60 * 4)
#define LAMP_EMERGENCY_TIME_MAX_VAL UINT8_MAX
#define LAMP_TOTAL_OPERATION_TIME_MAX_VAL UINT8_MAX
#define BATTERY_CHARGE_UNKNOWN 255
#define BATTERY_CHARGE_MAX (BATTERY_CHARGE_UNKNOWN - 1)
#define EMERGENCY_MAX_LEVEL_MAX_VAL 254
#define FUNCTION_TEST_INTERVAL_DISABLED 0
#define DURATION_TEST_INTERVAL_DISABLED 0

#define EL_TOTAL_OPERATION_TIME_UNKNOWN 0xFFFFFFFF
#define EL_EMERGENCY_TIME_UNKNOWN 0xFFFFFFFF
#define EL_LIGHTNESS_MAX_VAL UINT16_MAX

#define INHIBIT_TIMER_REFRESH_TIME_S 60
#define BATTERY_STATUS_UPDATE_TIME_S 60
#define BATTERY_LEVEL_LOW_PERCENT 30
#define BATTERY_LEVEL_CRITICAL_LOW_PERCENT 10


static void MeshMessageHandler(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ProcessElMessage(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ProcessEltMessage(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElInhibitEnter(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElInhibitExit(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElRestEnter(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElRestExit(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElStateGet(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElPropertyStatus(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElOperationTimeGet(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void ElOperationTimeClear(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void EltFunctionalTestStart(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void EltFunctionalTestStop(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void EltFunctionalTestGet(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void EltDurationTestStart(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void EltDurationTestStop(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void EltDurationTestGet(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void MeshMessageRequest1Send(struct UartProtocolFrameMeshMessageFrame *p_frame, uint8_t subopcode, uint8_t *p_payload, size_t len);

static enum EmgLTest_ElState GetElState(void);
static void                  UpdateBatteryStatus(void);
static void                  StopTestIfPending(void);
static void                  DisableAutoTest(void);
static uint8_t               FitEmergencyLevelToMinMax(uint8_t emergency_level);


static const uint32_t MeshMessageOpcodeList[] = {
    MESH_MESSAGE_LIGHT_EL,
    MESH_MESSAGE_LIGHT_EL_TEST,
};

static const char *ElStateIdToString[] = {
    "RFU",
    "RFU",
    "RFU",
    "Normal",
    "RFU",
    "Emergency",
    "Extended Emergency",
    "RFU",
    "Rest",
    "RFU",
    "Inhibit",
    "RFU",
    "Duration test in progress",
    "RFU",
    "Functional test in progress",
    "Battery Discharged",
};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = NULL,
    .p_mesh_message_handler       = MeshMessageHandler,
    .p_uart_frame_command_list    = NULL,
    .p_mesh_message_opcode_list   = MeshMessageOpcodeList,
    .uart_frame_command_list_len  = 0,
    .mesh_message_opcode_list_len = ARRAY_SIZE(MeshMessageOpcodeList),
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

struct ModelManagerRegistrationRow ModelConfigLightEltServer = {
    .model_id            = MODEL_MANAGER_ID_LIGHT_ELT_SERVER,
    .p_model_parameter   = NULL,
    .model_parameter_len = 0,
    .p_instance_index    = &MessageHandlerConfig.instance_index,
};

struct PACKED EmergencyStatus
{
    uint8_t inhibit_mode_active : 1;
    uint8_t function_test_done_and_results_available : 1;
    uint8_t duration_test_done_and_results_available : 1;
    uint8_t battery_charged : 1;
    uint8_t function_test_pending : 1;
    uint8_t duration_test_pending : 1;
    uint8_t identify_active : 1;
    uint8_t physical_selection_active : 1;
};

struct PACKED FailureStatus
{
    uint8_t emergency_control_gear_failure : 1;
    uint8_t battery_duration_failure : 1;
    uint8_t battery_failure : 1;
    uint8_t emergency_lamp_failure : 1;
    uint8_t function_test_max_delay_exceeded : 1;
    uint8_t duration_test_max_delay_exceeded : 1;
    uint8_t function_test_failed : 1;
    uint8_t duration_test_failed : 1;
};

struct PACKED EmergencyMode
{
    uint8_t rest_mode_active : 1;
    uint8_t normal_mode_active : 1;
    uint8_t emergency_mode_active : 1;
    uint8_t extended_emergency_mode_active : 1;
    uint8_t mode_function_test_in_progress_active : 1;
    uint8_t mode_duration_test_in_progress_active : 1;
    uint8_t hardwired_inhibit_input_active : 1;
    uint8_t hardwired_switch_is_on : 1;
};


static bool    IsInitialized                     = false;
static uint8_t InhibitRefreshCounterSeconds      = INHIBIT_TIMER_REFRESH_TIME_S;
static uint8_t BatteryStatusUpdateCounterSeconds = BATTERY_STATUS_UPDATE_TIME_S;


static void LoopEmgLTest(void)
{
    if (GetElState() == EMG_L_TEST_EL_STATE_INHIBIT)
    {
        if (InhibitRefreshCounterSeconds == INHIBIT_TIMER_REFRESH_TIME_S)
        {
            EmergencyDriverSimulator_Inhibit();
            InhibitRefreshCounterSeconds = 0;
        }
        else
        {
            InhibitRefreshCounterSeconds++;
        }
    }

    if (*ModelConfigLightEltServer.p_instance_index != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN)
    {
        if (BatteryStatusUpdateCounterSeconds == BATTERY_STATUS_UPDATE_TIME_S)
        {
            UpdateBatteryStatus();
            BatteryStatusUpdateCounterSeconds = 0;
        }
        else
        {
            BatteryStatusUpdateCounterSeconds++;
        }
    }

    StopTestIfPending();
}

void EmgLTest_Init(void)
{
    ASSERT(!IsInitialized);

    if (!EmergencyDriverSimulator_IsInitialized())
    {
        EmergencyDriverSimulator_Init();
    }

    ModelManager_RegisterModel(&ModelConfigLightEltServer);
    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);

    InhibitRefreshCounterSeconds = 0;

    DisableAutoTest();

    SimpleScheduler_TaskAdd(EMERGENCY_LIGHTING_TESTING_TASK_PERIOD_MS, LoopEmgLTest, SIMPLE_SCHEDULER_TASK_ID_EMG_L_TEST, false);

    IsInitialized = true;
}

bool EmgLTest_IsInitialized(void)
{
    return IsInitialized;
}

static void MeshMessageHandler(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (*ModelConfigLightEltServer.p_instance_index == UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN)
    {
        return;
    }

    switch (p_frame->mesh_opcode)
    {
        case MESH_MESSAGE_LIGHT_EL:
            ProcessElMessage(p_frame);
            break;

        case MESH_MESSAGE_LIGHT_EL_TEST:
            ProcessEltMessage(p_frame);
            break;

        default:
            break;
    }
}

static void ProcessElMessage(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if ((p_frame == NULL) || (p_frame->p_mesh_msg_payload == NULL) || (p_frame->mesh_msg_len < EMG_SUBOPCODE_SIZE))
    {
        return;
    }

    enum EmgLTest_ELSubOpcode subopcode = (enum EmgLTest_ELSubOpcode)p_frame->p_mesh_msg_payload[0];

    switch (subopcode)
    {
        case EMG_L_TEST_EL_SUBOPCODE_INHIBIT_ENTER:
            ElInhibitEnter(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_INHIBIT_EXIT:
            ElInhibitExit(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_STATE_GET:
            ElStateGet(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_PROPERTY_STATUS:
            ElPropertyStatus(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_GET:
            ElOperationTimeGet(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_CLEAR:
            ElOperationTimeClear(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_REST_ENTER:
            ElRestEnter(p_frame);
            break;

        case EMG_L_TEST_EL_SUBOPCODE_REST_EXIT:
            ElRestExit(p_frame);
            break;

        default:
            break;
    }
}

static void ProcessEltMessage(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if ((p_frame == NULL) || (p_frame->p_mesh_msg_payload == NULL) || (p_frame->mesh_msg_len < EMG_SUBOPCODE_SIZE))
    {
        return;
    }

    enum EmgLTest_ELTestSubOpcode subopcode = (enum EmgLTest_ELTestSubOpcode)p_frame->p_mesh_msg_payload[0];

    switch (subopcode)
    {
        case EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_GET:
            EltFunctionalTestGet(p_frame);
            break;

        case EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_START:
            EltFunctionalTestStart(p_frame);
            break;

        case EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_STOP:
            EltFunctionalTestStop(p_frame);
            break;

        case EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_GET:
            EltDurationTestGet(p_frame);
            break;

        case EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_START:
            EltDurationTestStart(p_frame);
            break;

        case EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_STOP:
            EltDurationTestStop(p_frame);
            break;

        default:
            break;
    }
}

static void ElInhibitEnter(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL Inhibit Enter");

    EmergencyDriverSimulator_Inhibit();

    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();

    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void ElInhibitExit(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL Inhibit Exit");

    if (GetElState() != EMG_L_TEST_EL_STATE_REST)
    {
        EmergencyDriverSimulator_ReLightResetInhibit();
    }
    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void ElRestEnter(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL Rest Enter");

    EmergencyDriverSimulator_Rest();

    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void ElRestExit(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL Rest Exit");

    if (GetElState() != EMG_L_TEST_EL_STATE_INHIBIT)
    {
        EmergencyDriverSimulator_ReLightResetInhibit();
    }
    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void ElStateGet(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL State Get");

    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void ElPropertyStatus(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != (sizeof(struct EmgLTest_ElPropertyStatus) + EMG_SUBOPCODE_SIZE))
    {
        return;
    }

    struct EmgLTest_ElPropertyStatus *frame = (struct EmgLTest_ElPropertyStatus *)(p_frame->p_mesh_msg_payload + EMG_SUBOPCODE_SIZE);

    if (frame->property_id == EMG_L_TEST_PROPERTY_ID_LIGHTNESS)
    {
        LOG_D("EL Property Status, EL Lightness: %d", frame->property_value);

        uint8_t emergency_level = (frame->property_value) / (EL_LIGHTNESS_MAX_VAL / EMERGENCY_MAX_LEVEL_MAX_VAL);
        emergency_level         = FitEmergencyLevelToMinMax(emergency_level);

        EmergencyDriverSimulator_Dtr(emergency_level);
        EmergencyDriverSimulator_StoreDtrAsEmergencyLevel();
    }
    else if (frame->property_id == EMG_L_TEST_PROPERTY_ID_PROLONG_TIME)
    {
        LOG_D("EL Property Status, EL Prolong Time: %d", frame->property_value);

        uint8_t prolong_time;
        if (frame->property_value <= PROLONG_TIME_STEP_S * UINT8_MAX)
        {
            prolong_time = frame->property_value / PROLONG_TIME_STEP_S;
        }
        else
        {
            prolong_time = UINT8_MAX;
        }

        EmergencyDriverSimulator_Dtr(prolong_time);
        EmergencyDriverSimulator_StoreProlongTime();
    }
}

static void ElOperationTimeGet(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    // In this module no non-volatile memory support is implemented. LAMP EMERGENCY TIME and
    // LAMP TOTAL OPERATION TIME are only read from EmergencyDriverSimulator and translated to
    // UART protocol units. To get correct time over more than maximum period expressed by DALI
    // protocol, MCU should periodically read the value, accumulate it and reset the timer, so
    // it does not reach "xx or longer" value.

    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL Operation Time Get");

    uint8_t emergency_time       = EmergencyDriverSimulator_QueryLampEmergencyTime();
    uint8_t total_operation_time = EmergencyDriverSimulator_QueryLampTotalOperationTime();

    struct EmgLTest_ElOperationTimeStatus resp;

    if (emergency_time != LAMP_EMERGENCY_TIME_MAX_VAL)
    {
        resp.emergency_time = emergency_time * LAMP_EMERGENCY_TIME_STEP_S;
    }
    else
    {
        resp.emergency_time = EL_EMERGENCY_TIME_UNKNOWN;
    }

    if (total_operation_time != LAMP_TOTAL_OPERATION_TIME_MAX_VAL)
    {
        resp.total_operation_time = total_operation_time * LAMP_TOTAL_OPERATION_TIME_STEP_S;
    }
    else
    {
        resp.total_operation_time = EL_TOTAL_OPERATION_TIME_UNKNOWN;
    }

    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL Operation Time Status, Emergency Time: %ld, Total Operation Time: %ld", resp.emergency_time, resp.total_operation_time);
}

static void ElOperationTimeClear(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("EL Operation Time Clear");

    EmergencyDriverSimulator_ResetLampTime();

    uint8_t emergency_time       = EmergencyDriverSimulator_QueryLampEmergencyTime();
    uint8_t total_operation_time = EmergencyDriverSimulator_QueryLampTotalOperationTime();

    struct EmgLTest_ElOperationTimeStatus resp;

    if (emergency_time != LAMP_EMERGENCY_TIME_MAX_VAL)
    {
        resp.emergency_time = emergency_time * LAMP_EMERGENCY_TIME_STEP_S;
    }
    else
    {
        resp.emergency_time = EL_EMERGENCY_TIME_UNKNOWN;
    }

    if (total_operation_time != LAMP_TOTAL_OPERATION_TIME_MAX_VAL)
    {
        resp.total_operation_time = total_operation_time * LAMP_TOTAL_OPERATION_TIME_STEP_S;
    }
    else
    {
        resp.total_operation_time = EL_TOTAL_OPERATION_TIME_UNKNOWN;
    }

    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL Operation Time Status, Emergency Time: %ld, Total Operation Time: %ld", resp.emergency_time, resp.total_operation_time);
}

static void EltFunctionalTestStart(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("ELT Functional Test Start");

    struct EmgLTest_ElStateStatus resp;
    enum EmgLTest_ElState         el_state = GetElState();
    if (el_state != EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED)
    {
        EmergencyDriverSimulator_StartFunctionTest();
        resp.state = GetElState();
    }
    else
    {
        resp.state = EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED;
    }

    p_frame->mesh_opcode = EL_SERVER_OPCODE;
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void EltFunctionalTestStop(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("ELT Functional Test Stop");

    EmergencyDriverSimulator_StopTest();

    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();

    p_frame->mesh_opcode = EL_SERVER_OPCODE;
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void EltFunctionalTestGet(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("ELT Functional Test Get");

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    uint8_t               failure_status   = EmergencyDriverSimulator_QueryFailureStatus();
    struct FailureStatus *p_failure_status = (struct FailureStatus *)&failure_status;

    struct EmgLTest_EltFunctionalTestStatus resp = {0};
    if (p_emergency_status->function_test_done_and_results_available)
    {
        resp.status               = EMG_L_TEST_TEST_STATUS_FINISHED;
        resp.result.lamp_fault    = p_failure_status->emergency_lamp_failure;
        resp.result.battery_fault = p_failure_status->battery_failure;
        resp.result.circuit_fault = p_failure_status->emergency_control_gear_failure;
        resp.result.rfu           = 0;
    }
    else
    {
        resp.status               = EMG_L_TEST_TEST_STATUS_UNKNOWN;
        resp.result.lamp_fault    = 0;
        resp.result.battery_fault = 0;
        resp.result.circuit_fault = 0;
        resp.result.rfu           = 0;
    }

    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("ELT Functional Test Status: %s", (resp.status == EMG_L_TEST_TEST_STATUS_FINISHED) ? "Finished" : "Unknown");
}

static void EltDurationTestStart(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("ELT Duration Test Start");

    struct EmgLTest_ElStateStatus resp;
    enum EmgLTest_ElState         el_state = GetElState();
    if (el_state != EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED)
    {
        EmergencyDriverSimulator_StartDurationTest();
        resp.state = GetElState();
    }
    else
    {
        resp.state = EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED;
    }

    p_frame->mesh_opcode = EL_SERVER_OPCODE;
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void EltDurationTestStop(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("ELT Duration Test Stop");

    EmergencyDriverSimulator_StopTest();

    struct EmgLTest_ElStateStatus resp;
    resp.state = GetElState();

    p_frame->mesh_opcode = EL_SERVER_OPCODE;
    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("EL State Status: %s", ElStateIdToString[resp.state]);
}

static void EltDurationTestGet(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    if (p_frame->mesh_msg_len != EMG_SUBOPCODE_SIZE)
    {
        return;
    }
    LOG_D("ELT Duration Test Get");

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    uint8_t               failure_status   = EmergencyDriverSimulator_QueryFailureStatus();
    struct FailureStatus *p_failure_status = (struct FailureStatus *)&failure_status;

    uint8_t  duration_test_result   = EmergencyDriverSimulator_QueryDurationTestResult();
    uint16_t duration_test_result_s = duration_test_result * DURATION_TEST_RESULT_STEP_S;

    struct EmgLTest_EltDurationTestStatus resp;
    if (p_emergency_status->duration_test_done_and_results_available)
    {
        resp.status                        = EMG_L_TEST_TEST_STATUS_FINISHED;
        resp.result.lamp_fault             = p_failure_status->emergency_lamp_failure;
        resp.result.battery_fault          = p_failure_status->battery_failure;
        resp.result.circuit_fault          = p_failure_status->emergency_control_gear_failure;
        resp.result.battery_duration_fault = p_failure_status->battery_duration_failure;
        resp.result.rfu                    = 0;
        resp.test_length                   = duration_test_result_s;
    }
    else
    {
        resp.status                        = EMG_L_TEST_TEST_STATUS_UNKNOWN;
        resp.result.lamp_fault             = 0;
        resp.result.battery_fault          = 0;
        resp.result.circuit_fault          = 0;
        resp.result.battery_duration_fault = p_failure_status->battery_duration_failure;
        resp.result.rfu                    = 0;
        resp.test_length                   = 0;
    }

    MeshMessageRequest1Send(p_frame, EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_STATUS, (uint8_t *)&resp, sizeof(resp));
    LOG_D("ELT Duration Test Status: %s", (resp.status == EMG_L_TEST_TEST_STATUS_FINISHED) ? "Finished" : "Unknown");
}

static void MeshMessageRequest1Send(struct UartProtocolFrameMeshMessageFrame *p_frame, uint8_t subopcode, uint8_t *p_payload, size_t len)
{
    ASSERT((p_frame->mesh_opcode & (MESH_OPCODE_SIZE_3_OCTET_MASK << 16)) == (MESH_OPCODE_SIZE_3_OCTET_MASK << 16));

    size_t size = sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode3B) + sizeof(subopcode) + len;

    uint8_t                                              buff[size];
    struct UartProtocolFrameMeshMessageRequest1Opcode3B *p_tx_frame = (struct UartProtocolFrameMeshMessageRequest1Opcode3B *)buff;
    p_tx_frame->cmd                                                 = UART_FRAME_CMD_MESH_MESSAGE_REQUEST1;
    p_tx_frame->len                                                 = size - UART_PROTOCOL_FRAME_HEADER_LEN;
    p_tx_frame->instance_index                                      = p_frame->instance_index;
    p_tx_frame->mesh_opcode_be                                      = SWAP_3_BYTES(p_frame->mesh_opcode);
    p_tx_frame->sub_index                                           = p_frame->sub_index;
    p_tx_frame->p_data[0]                                           = subopcode;
    memcpy(p_tx_frame->p_data + 1, p_payload, len);

    UartProtocol_SendFrame((struct UartFrameRxTxFrame *)p_tx_frame);
}

static enum EmgLTest_ElState GetElState(void)
{
    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    uint8_t battery_charge = EmergencyDriverSimulator_QueryBatteryCharge();

    if (p_emergency_mode->rest_mode_active)
    {
        return EMG_L_TEST_EL_STATE_REST;
    }

    if (p_emergency_mode->emergency_mode_active)
    {
        return EMG_L_TEST_EL_STATE_EMERGENCY;
    }

    if (p_emergency_mode->extended_emergency_mode_active)
    {
        return EMG_L_TEST_EL_STATE_EXTENDED_EMERGENCY;
    }

    if (p_emergency_mode->mode_function_test_in_progress_active)
    {
        return EMG_L_TEST_EL_STATE_FUNCTIONAL_TEST_IN_PROGRESS;
    }

    if (p_emergency_mode->mode_duration_test_in_progress_active)
    {
        return EMG_L_TEST_EL_STATE_DURATION_TEST_IN_PROGRESS;
    }

    if (p_emergency_status->inhibit_mode_active)
    {
        return EMG_L_TEST_EL_STATE_INHIBIT;
    }

    if (battery_charge == 0)
    {
        return EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED;
    }

    if (p_emergency_mode->normal_mode_active)
    {
        return EMG_L_TEST_EL_STATE_NORMAL;
    }

    return EMG_L_TEST_EL_STATE_NORMAL;
}

static void UpdateBatteryStatus(void)
{
    uint8_t                                          payload[sizeof(struct UartProtocolFrameBatteryStatusSetRequest)] = {0};
    struct UartProtocolFrameBatteryStatusSetRequest *p_payload = (struct UartProtocolFrameBatteryStatusSetRequest *)payload;

    p_payload->flags = BATTERY_FLAGS_PRESENCE_PRESENT_AND_NON_REMOVABLE | BATTERY_FLAGS_CHARGING_IS_CHARGEABLE_AND_IS_NOT_CHARGING |
                       BATTERY_FLAGS_SERVICEABILITY_BATTERY_DOES_NOT_REQUIRE_SERVICE;

    uint8_t battery_charge = EmergencyDriverSimulator_QueryBatteryCharge();
    if (battery_charge == BATTERY_CHARGE_UNKNOWN)
    {
        p_payload->battery_level = BATTERY_LEVEL_UNKNOWN;
    }
    else
    {
        p_payload->battery_level = (battery_charge * BATTERY_LEVEL_MAX) / BATTERY_CHARGE_MAX;
    }

    if (p_payload->battery_level <= BATTERY_LEVEL_CRITICAL_LOW_PERCENT)
    {
        p_payload->flags |= BATTERY_FLAGS_INDICATOR_CRITICALLY_LOW_LEVEL;
    }
    else if (p_payload->battery_level <= BATTERY_LEVEL_LOW_PERCENT)
    {
        p_payload->flags |= BATTERY_FLAGS_INDICATOR_LOW_LEVEL;
    }
    else
    {
        p_payload->flags |= BATTERY_FLAGS_INDICATOR_GOOD_LEVEL;
    }

    p_payload->instance_index    = MessageHandlerConfig.instance_index;
    p_payload->time_to_discharge = BATTERY_TIME_TO_DISCHARGE_UNKNOWN;
    p_payload->time_to_charge    = BATTERY_TIME_TO_CHARGE_UNKNOWN;

    UartProtocol_Send(UART_FRAME_CMD_BATTERY_STATUS_SET_REQ, payload + UART_PROTOCOL_FRAME_HEADER_LEN, sizeof(payload) - UART_PROTOCOL_FRAME_HEADER_LEN);

    if (p_payload->battery_level != BATTERY_LEVEL_UNKNOWN)
    {
        LOG_D("BatteryStatusSetRequest: %d%%", p_payload->battery_level);
    }
    else
    {
        LOG_D("BatteryStatusSetRequest: level unknown");
    }
}

static void StopTestIfPending(void)
{
    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    // If the test could not be started instantly for some reason, it should be stopped.
    // Tests should be performed only, when UARTModem firmware requests for it.
    // Otherwise, test results will not be collected.
    if (p_emergency_status->function_test_pending || p_emergency_status->duration_test_pending)
    {
        EmergencyDriverSimulator_StopTest();
    }
}

static void DisableAutoTest(void)
{
    // Auto testing functionality should be disabled for both types of tests.
    // From perspective of DALI driver, tests should be run only in the moment
    // of reception of "START FUNCTION/DURATION TEST" command.
    EmergencyDriverSimulator_Dtr(FUNCTION_TEST_INTERVAL_DISABLED);
    EmergencyDriverSimulator_StoreFunctionTestInterval();
    EmergencyDriverSimulator_Dtr(DURATION_TEST_INTERVAL_DISABLED);
    EmergencyDriverSimulator_StoreDurationTestInterval();
}

static uint8_t FitEmergencyLevelToMinMax(uint8_t emergency_level)
{
    uint8_t min_level = EmergencyDriverSimulator_QueryEmergencyMinLevel();
    if (emergency_level < min_level)
    {
        emergency_level = min_level;
    }

    uint8_t max_level = EmergencyDriverSimulator_QueryEmergencyMaxLevel();
    if (emergency_level > max_level)
    {
        emergency_level = max_level;
    }

    return emergency_level;
}
