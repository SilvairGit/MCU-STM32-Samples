#include <string.h>

#include "EmergencyDriverSimulator.c"
#include "MockAdcHal.h"
#include "MockAssert.h"
#include "MockGpioHal.h"
#include "unity.h"


static bool    IsMainsPowerOn;
static uint8_t BatteryLevelPercent;
static bool    ShortenDurationTest;


static void     SetMainsPowerOff(void);
static void     SetMainsPowerOn(void);
static void     ShortenCurrentDurationTest(void);
static uint16_t AdcHal_ReadChannel_StubCbk(enum AdcHalChannel adc_channel, int cmock_num_calls);
static bool     GpioHal_PinRead_StubCbk(enum GpioHalPin gpio, int cmock_num_calls);
static void     SetBatteryLevel(uint8_t battery_level_percent);


void setUp(void)
{
    AdcHal_IsInitialized_IgnoreAndReturn(false);
    AdcHal_Init_Ignore();
    GpioHal_IsInitialized_IgnoreAndReturn(false);
    GpioHal_Init_Ignore();
    GpioHal_PinMode_Ignore();

    AdcHal_ReadChannel_StubWithCallback(AdcHal_ReadChannel_StubCbk);
    GpioHal_PinRead_StubWithCallback(GpioHal_PinRead_StubCbk);

    IsInitialized                 = 0;
    Dtr                           = 0;
    EmergencyLevel                = 0;
    ProlongTimeSeconds            = 0;
    ProlongTimeCounterSeconds     = 0;
    LampEmergencyTimeSeconds      = 0;
    LampTotalOperationTimeSeconds = 0;
    DurationTestResultSeconds     = 0;
    memset(&EmergencyStatus, 0, sizeof(EmergencyStatus));
    memset(&FailureStatus, 0, sizeof(FailureStatus));
    memset(&EmergencyMode, 0, sizeof(EmergencyMode));

    IsMainsPowerOn      = true;
    BatteryLevelPercent = 100;
    ShortenDurationTest = false;

    SetModeOfOperation(NORMAL_MODE);
}

void test_Init(void)
{
    IsInitialized = false;
    TEST_ASSERT_EQUAL(false, EmergencyDriverSimulator_IsInitialized());

    EmergencyDriverSimulator_Init();
    TEST_ASSERT_EQUAL(true, EmergencyDriverSimulator_IsInitialized());
}

void test_ModeOfOperation_GetSet(void)
{
    enum ModeOfOperation mode_of_operation[] = {
        FUNCTION_TEST_IN_PROGRESS,
        DURATION_TEST_IN_PROGRESS,
        NORMAL_MODE,
        INHIBIT_MODE,
        REST_MODE,
        EMERGENCY_MODE,
        EXTENDED_EMERGENCY_MODE,
    };

    for (size_t i = 0; i < ARRAY_SIZE(mode_of_operation); i++)
    {
        SetModeOfOperation(mode_of_operation[i]);
        TEST_ASSERT_EQUAL(mode_of_operation[i], GetModeOfOperation());
    }
}

void test_Init_Normal(void)
{
    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_Normal_to_Inhibit(void)
{
    SetModeOfOperation(NORMAL_MODE);

    EmergencyDriverSimulator_Inhibit();

    TEST_ASSERT_EQUAL(INHIBIT_MODE, GetModeOfOperation());
}

void test_transition_Inhibit_to_Normal_ResetInhibit(void)
{
    SetModeOfOperation(INHIBIT_MODE);

    EmergencyDriverSimulator_ReLightResetInhibit();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_Inhibit_to_Normal_Timeout(void)
{
    SetModeOfOperation(NORMAL_MODE);
    EmergencyDriverSimulator_Inhibit();

    for (size_t i = 0; i < INHIBIT_TIMER_EXPIRATION_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
        TEST_ASSERT_EQUAL(INHIBIT_MODE, GetModeOfOperation());
    }

    EmergencyDriverSimulator_Loop();
    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_Normal_to_FunctionTestInProgress(void)
{
    SetModeOfOperation(NORMAL_MODE);

    EmergencyDriverSimulator_StartFunctionTest();

    TEST_ASSERT_EQUAL(FUNCTION_TEST_IN_PROGRESS, GetModeOfOperation());
}

void test_transition_FunctionTestInProgress_to_Normal_StopTest(void)
{
    SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);

    EmergencyDriverSimulator_StopTest();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_FunctionTestInProgress_to_Normal_TestFinished(void)
{
    SetModeOfOperation(NORMAL_MODE);
    EmergencyDriverSimulator_StartFunctionTest();

    for (size_t i = 0; i < ELT_FUNCTION_TEST_TIME_S; i++)
    {
        TEST_ASSERT_EQUAL(FUNCTION_TEST_IN_PROGRESS, GetModeOfOperation());
        EmergencyDriverSimulator_Loop();
    }

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_Normal_to_DurationTestInProgress(void)
{
    SetModeOfOperation(NORMAL_MODE);

    EmergencyDriverSimulator_StartDurationTest();

    TEST_ASSERT_EQUAL(DURATION_TEST_IN_PROGRESS, GetModeOfOperation());
}

void test_transition_DurationTestInProgress_to_Normal_StopTest(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    EmergencyDriverSimulator_StopTest();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_DurationTestInProgress_to_Normal_TestFinished(void)
{
    SetModeOfOperation(NORMAL_MODE);
    EmergencyDriverSimulator_StartDurationTest();

    for (size_t i = 0; i < ELT_DURATION_TEST_TIME_S; i++)
    {
        TEST_ASSERT_EQUAL(DURATION_TEST_IN_PROGRESS, GetModeOfOperation());
        EmergencyDriverSimulator_Loop();
    }

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_FunctionTestInProgress_to_Emergency(void)
{
    SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);

    SetMainsPowerOff();

    TEST_ASSERT_EQUAL(EMERGENCY_MODE, GetModeOfOperation());
}

void test_transition_DurationTestInProgress_to_Emergency(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    SetMainsPowerOff();

    TEST_ASSERT_EQUAL(EMERGENCY_MODE, GetModeOfOperation());
}

void test_transition_Normal_to_Emergency(void)
{
    SetModeOfOperation(NORMAL_MODE);

    SetMainsPowerOff();

    TEST_ASSERT_EQUAL(EMERGENCY_MODE, GetModeOfOperation());
}

void test_transition_ExtendedEmergency_to_Emergency(void)
{
    SetModeOfOperation(EXTENDED_EMERGENCY_MODE);

    SetMainsPowerOff();

    TEST_ASSERT_EQUAL(EMERGENCY_MODE, GetModeOfOperation());
}

void test_transition_Emergency_to_ExtendedEmergency_NonZeroProlongTime(void)
{
    SetModeOfOperation(EMERGENCY_MODE);
    EmergencyDriverSimulator_Dtr(1);
    EmergencyDriverSimulator_StoreProlongTime();

    SetMainsPowerOn();

    TEST_ASSERT_EQUAL(EXTENDED_EMERGENCY_MODE, GetModeOfOperation());
}

void test_transition_Emergency_to_ExtendedEmergency_ZeroProlongTime(void)
{
    SetModeOfOperation(EMERGENCY_MODE);
    EmergencyDriverSimulator_Dtr(0);
    EmergencyDriverSimulator_StoreProlongTime();

    SetMainsPowerOn();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_Emergency_to_Rest_DischargedBattery(void)
{
    SetMainsPowerOff();
    SetModeOfOperation(EMERGENCY_MODE);
    EmergencyDriverSimulator_Dtr(1);
    EmergencyDriverSimulator_StoreProlongTime();

    SetBatteryLevel(0);

    TEST_ASSERT_EQUAL(REST_MODE, GetModeOfOperation());
}

void test_transition_Inhibit_to_Rest(void)
{
    SetModeOfOperation(INHIBIT_MODE);

    SetMainsPowerOff();

    TEST_ASSERT_EQUAL(REST_MODE, GetModeOfOperation());
}

void test_transition_Rest_to_Normal(void)
{
    SetModeOfOperation(REST_MODE);

    SetMainsPowerOn();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_Rest_to_Emergency(void)
{
    SetModeOfOperation(REST_MODE);

    EmergencyDriverSimulator_ReLightResetInhibit();

    TEST_ASSERT_EQUAL(EMERGENCY_MODE, GetModeOfOperation());
}

void test_transition_Emergency_to_Rest_RestCmd(void)
{
    SetModeOfOperation(EMERGENCY_MODE);

    EmergencyDriverSimulator_Rest();

    TEST_ASSERT_EQUAL(REST_MODE, GetModeOfOperation());
}

void test_transition_ExtendedEmergency_to_Normal_TimeExpired(void)
{
    SetModeOfOperation(EXTENDED_EMERGENCY_MODE);
    uint8_t prolong_time = 1;
    EmergencyDriverSimulator_Dtr(prolong_time);
    EmergencyDriverSimulator_StoreProlongTime();

    for (size_t i = 0; i < (PROLONG_TIME_STEP_S * prolong_time); i++)
    {
        EmergencyDriverSimulator_Loop();
        TEST_ASSERT_EQUAL(EXTENDED_EMERGENCY_MODE, GetModeOfOperation());
    }

    EmergencyDriverSimulator_Loop();
    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_transition_ExtendedEmergency_to_Normal_DischargedBattery(void)
{
    SetModeOfOperation(EXTENDED_EMERGENCY_MODE);
    uint8_t prolong_time = 1;
    EmergencyDriverSimulator_Dtr(prolong_time);
    EmergencyDriverSimulator_StoreProlongTime();

    SetBatteryLevel(0);

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());
}

void test_Normal_EmergencyMode(void)
{
    SetModeOfOperation(NORMAL_MODE);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_FALSE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_TRUE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_Inhibit_EmergencyMode(void)
{
    SetModeOfOperation(INHIBIT_MODE);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_FALSE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_TRUE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_Rest_EmergencyMode(void)
{
    SetModeOfOperation(REST_MODE);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_TRUE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_EmergencyMode_EmergencyMode(void)
{
    SetModeOfOperation(EMERGENCY_MODE);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_FALSE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_TRUE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_DurationTestInProgress_EmergencyMode(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_FALSE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_TRUE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_FunctionTestInProgress_EmergencyMode(void)
{
    SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_FALSE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_TRUE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_ExtendedEmergencyMode_EmergencyMode(void)
{
    SetModeOfOperation(EXTENDED_EMERGENCY_MODE);

    uint8_t               emergency_mode   = EmergencyDriverSimulator_QueryEmergencyMode();
    struct EmergencyMode *p_emergency_mode = (struct EmergencyMode *)&emergency_mode;

    TEST_ASSERT_FALSE(p_emergency_mode->rest_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->normal_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->emergency_mode_active);
    TEST_ASSERT_TRUE(p_emergency_mode->extended_emergency_mode_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_function_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->mode_duration_test_in_progress_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_inhibit_input_active);
    TEST_ASSERT_FALSE(p_emergency_mode->hardwired_switch_is_on);
}

void test_Normal_EmergencyStatus(void)
{
    SetModeOfOperation(NORMAL_MODE);

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_FALSE(p_emergency_status->inhibit_mode_active);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->identify_active);
    TEST_ASSERT_FALSE(p_emergency_status->physical_selection_active);
}

void test_Inhibit_EmergencyStatus(void)
{
    SetModeOfOperation(INHIBIT_MODE);

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_TRUE(p_emergency_status->inhibit_mode_active);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->identify_active);
    TEST_ASSERT_FALSE(p_emergency_status->physical_selection_active);
}

void test_Emergency_EmergencyStatus(void)
{
    SetModeOfOperation(EMERGENCY_MODE);

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_FALSE(p_emergency_status->inhibit_mode_active);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->identify_active);
    TEST_ASSERT_FALSE(p_emergency_status->physical_selection_active);
}

void test_ExtendedEmergency_EmergencyStatus(void)
{
    SetModeOfOperation(EXTENDED_EMERGENCY_MODE);

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_FALSE(p_emergency_status->inhibit_mode_active);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->identify_active);
    TEST_ASSERT_FALSE(p_emergency_status->physical_selection_active);
}

void test_FunctionTestIsInProgress_EmergencyStatus(void)
{
    SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_FALSE(p_emergency_status->inhibit_mode_active);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->identify_active);
    TEST_ASSERT_FALSE(p_emergency_status->physical_selection_active);
}

void test_DurationTestIsInProgress_EmergencyStatus(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_FALSE(p_emergency_status->inhibit_mode_active);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_done_and_results_available);
    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);
    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->duration_test_pending);
    TEST_ASSERT_FALSE(p_emergency_status->identify_active);
    TEST_ASSERT_FALSE(p_emergency_status->physical_selection_active);
}

void test_FunctionTestFinished_EmergencyStatus(void)
{
    SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < ELT_FUNCTION_TEST_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_TRUE(p_emergency_status->function_test_done_and_results_available);
}

void test_DurationTestFinished_EmergencyStatus(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < ELT_DURATION_TEST_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_TRUE(p_emergency_status->duration_test_done_and_results_available);
}

void test_BatteryFullyChargedBitInEmergencyStatus(void)
{
    SetBatteryLevel(100);
    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_TRUE(p_emergency_status->battery_charged);

    SetBatteryLevel(99);
    emergency_status = EmergencyDriverSimulator_QueryEmergencyStatus();

    TEST_ASSERT_FALSE(p_emergency_status->battery_charged);

    SetBatteryLevel(100);
    emergency_status = EmergencyDriverSimulator_QueryEmergencyStatus();

    TEST_ASSERT_TRUE(p_emergency_status->battery_charged);
}

void test_FunctionTestNotPostponedWhenBatteryDischarged(void)
{
    SetModeOfOperation(NORMAL_MODE);
    SetBatteryLevel(0);

    EmergencyDriverSimulator_StartFunctionTest();

    TEST_ASSERT_EQUAL(FUNCTION_TEST_IN_PROGRESS, GetModeOfOperation());
}

void test_DurationTestPostponedWhenBatteryNotCharged(void)
{
    SetModeOfOperation(NORMAL_MODE);
    SetBatteryLevel(99);

    EmergencyDriverSimulator_StartDurationTest();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;

    TEST_ASSERT_FALSE(p_emergency_status->function_test_pending);
    TEST_ASSERT_TRUE(p_emergency_status->duration_test_pending);
}

void test_FunctionTestStartedWhenPostponedAndBatteryCharged(void)
{
    SetModeOfOperation(NORMAL_MODE);
    SetBatteryLevel(99);

    EmergencyDriverSimulator_StartFunctionTest();

    SetBatteryLevel(100);

    TEST_ASSERT_EQUAL(FUNCTION_TEST_IN_PROGRESS, GetModeOfOperation());
}

void test_DurationTestStartedWhenPostponedAndBatteryCharged(void)
{
    SetModeOfOperation(NORMAL_MODE);
    SetBatteryLevel(99);

    EmergencyDriverSimulator_StartDurationTest();

    SetBatteryLevel(100);

    TEST_ASSERT_EQUAL(DURATION_TEST_IN_PROGRESS, GetModeOfOperation());
}

void test_StoreEmergencyLevel(void)
{
    EmergencyDriverSimulator_Dtr(50);
    EmergencyDriverSimulator_StoreDtrAsEmergencyLevel();

    TEST_ASSERT_EQUAL(50, EmergencyLevel);
}

void test_QueryBatteryCharge(void)
{
    uint8_t battery_level[] = {
        0,
        1,
        2,
        50,
        99,
        100,
    };
    uint8_t expected_battery_charge[] = {
        0,
        0,
        2,
        124,
        248,
        254,
    };

    for (size_t i = 0; i < ARRAY_SIZE(battery_level); i++)
    {
        SetBatteryLevel(battery_level[i]);
        uint8_t battery_charge = EmergencyDriverSimulator_QueryBatteryCharge();

        TEST_ASSERT_EQUAL(expected_battery_charge[i], battery_charge);
    }
}

void test_QueryEmergencyMinLevel(void)
{
    TEST_ASSERT_EQUAL(1, EmergencyDriverSimulator_QueryEmergencyMinLevel());
}

void test_QueryEmergencyMaxLevel(void)
{
    TEST_ASSERT_EQUAL(254, EmergencyDriverSimulator_QueryEmergencyMaxLevel());
}

void test_LampTotalOperationTime_Normal(void)
{
    uint8_t total_operation_time = EmergencyDriverSimulator_QueryLampTotalOperationTime();
    TEST_ASSERT_EQUAL(0, total_operation_time);

    for (size_t i = 0; i < LAMP_TOTAL_OPERATION_TIME_STEP_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    total_operation_time = EmergencyDriverSimulator_QueryLampTotalOperationTime();
    TEST_ASSERT_EQUAL(1, total_operation_time);
}

void test_LampTotalOperationTime_Emergency(void)
{
    SetMainsPowerOff();

    for (size_t i = 0; i < LAMP_TOTAL_OPERATION_TIME_STEP_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    uint8_t total_operation_time = EmergencyDriverSimulator_QueryLampTotalOperationTime();
    TEST_ASSERT_EQUAL(1, total_operation_time);
}

void test_LampEmergencyTime_Normal(void)
{
    uint8_t emergency_time = EmergencyDriverSimulator_QueryLampEmergencyTime();
    TEST_ASSERT_EQUAL(0, emergency_time);

    for (size_t i = 0; i < LAMP_EMERGENCY_TIME_STEP_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    emergency_time = EmergencyDriverSimulator_QueryLampEmergencyTime();
    TEST_ASSERT_EQUAL(0, emergency_time);
}

void test_LampEmergencyTime_Emergency(void)
{
    SetMainsPowerOff();

    for (size_t i = 0; i < LAMP_EMERGENCY_TIME_STEP_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    uint8_t emergency_time = EmergencyDriverSimulator_QueryLampEmergencyTime();
    TEST_ASSERT_EQUAL(1, emergency_time);
}

void test_ResetLampTime(void)
{
    SetMainsPowerOff();

    for (size_t i = 0; i < LAMP_TOTAL_OPERATION_TIME_STEP_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    EmergencyDriverSimulator_ResetLampTime();

    uint8_t emergency_time = EmergencyDriverSimulator_QueryLampEmergencyTime();
    TEST_ASSERT_EQUAL(0, emergency_time);
    uint8_t total_operation_time = EmergencyDriverSimulator_QueryLampTotalOperationTime();
    TEST_ASSERT_EQUAL(0, total_operation_time);
}

void test_DurationTestResult(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < ELT_DURATION_TEST_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    TEST_ASSERT_EQUAL(30, EmergencyDriverSimulator_QueryDurationTestResult());
}

void test_FailureStatus_NoTestPerformed(void)
{
    uint8_t failure_status = EmergencyDriverSimulator_QueryFailureStatus();
    TEST_ASSERT_EQUAL(0, failure_status);
}

void test_FailureStatus_AfterFunctionTest(void)
{
    SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < ELT_FUNCTION_TEST_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    uint8_t failure_status = EmergencyDriverSimulator_QueryFailureStatus();
    TEST_ASSERT_EQUAL(0, failure_status);
}

void test_FailureStatus_AfterDurationTest(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < ELT_DURATION_TEST_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    uint8_t failure_status = EmergencyDriverSimulator_QueryFailureStatus();
    TEST_ASSERT_EQUAL(0, failure_status);
}

void test_FailureStatus_SuccessDurationTestAfterFailedDurationTest(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < (ELT_DURATION_TEST_TIME_S / 2); i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    SetBatteryLevel(0);

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;
    TEST_ASSERT_TRUE(p_emergency_status->duration_test_done_and_results_available);

    uint8_t               failure_status   = EmergencyDriverSimulator_QueryFailureStatus();
    struct FailureStatus *p_failure_status = (struct FailureStatus *)&failure_status;
    TEST_ASSERT_TRUE(p_failure_status->battery_failure);

    TEST_ASSERT_EQUAL(15, EmergencyDriverSimulator_QueryDurationTestResult());

    SetBatteryLevel(100);
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < ELT_DURATION_TEST_TIME_S; i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    failure_status = EmergencyDriverSimulator_QueryFailureStatus();
    TEST_ASSERT_EQUAL(0, failure_status);
}

void test_DurationTestShortened(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    ShortenCurrentDurationTest();

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;
    TEST_ASSERT_TRUE(p_emergency_status->duration_test_done_and_results_available);

    uint8_t failure_status = EmergencyDriverSimulator_QueryFailureStatus();
    TEST_ASSERT_EQUAL(0, failure_status);

    TEST_ASSERT_EQUAL(30, EmergencyDriverSimulator_QueryDurationTestResult());
}

void test_DurationTestInterrupted(void)
{
    SetModeOfOperation(DURATION_TEST_IN_PROGRESS);

    for (size_t i = 0; i < (ELT_DURATION_TEST_TIME_S / 2); i++)
    {
        EmergencyDriverSimulator_Loop();
    }

    SetBatteryLevel(0);

    TEST_ASSERT_EQUAL(NORMAL_MODE, GetModeOfOperation());

    uint8_t                 emergency_status   = EmergencyDriverSimulator_QueryEmergencyStatus();
    struct EmergencyStatus *p_emergency_status = (struct EmergencyStatus *)&emergency_status;
    TEST_ASSERT_TRUE(p_emergency_status->duration_test_done_and_results_available);

    uint8_t               failure_status   = EmergencyDriverSimulator_QueryFailureStatus();
    struct FailureStatus *p_failure_status = (struct FailureStatus *)&failure_status;
    TEST_ASSERT_TRUE(p_failure_status->battery_failure);

    TEST_ASSERT_EQUAL(15, EmergencyDriverSimulator_QueryDurationTestResult());
}

static void SetMainsPowerOff(void)
{
    IsMainsPowerOn = false;
    EmergencyDriverSimulator_Loop();
}

static void SetMainsPowerOn(void)
{
    IsMainsPowerOn = true;
    EmergencyDriverSimulator_Loop();
}

static void ShortenCurrentDurationTest(void)
{
    ShortenDurationTest = true;
    EmergencyDriverSimulator_Loop();
}

static void SetBatteryLevel(uint8_t battery_level_percent)
{
    BatteryLevelPercent = battery_level_percent;
    EmergencyDriverSimulator_Loop();
}

static uint16_t AdcHal_ReadChannel_StubCbk(enum AdcHalChannel adc_channel, int cmock_num_calls)
{
    UNUSED(cmock_num_calls);

    TEST_ASSERT_EQUAL(ADC_HAL_CHANNEL_POTENTIOMETER, adc_channel);

    return (ADC_HAL_ADC_MAX - (BatteryLevelPercent * (ADC_HAL_ADC_MAX - 2 * EMG_ANALOG_DEAD_RANGE_VALUE) / BATTERY_LEVEL_MAX + EMG_ANALOG_DEAD_RANGE_VALUE));
}

static bool GpioHal_PinRead_StubCbk(enum GpioHalPin gpio, int cmock_num_calls)
{
    UNUSED(cmock_num_calls);

    if (gpio == GPIO_HAL_PIN_ENCODER_SW)
    {
        return IsMainsPowerOn;
    }
    else if (gpio == GPIO_HAL_PIN_SW3)
    {
        if (ShortenDurationTest)
        {
            ShortenDurationTest = false;
            return false;
        }
        return true;
    }

    // Should not enter here
    TEST_ASSERT(false);
    return false;
}
