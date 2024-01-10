#include "EmergencyDriverSimulator.h"

#include <string.h>

#include "AdcHal.h"
#include "Assert.h"
#include "GpioHal.h"
#include "MeshGenericBattery.h"
#include "SimpleScheduler.h"
#include "Utils.h"

#define EMG_BATTERY_LEVEL_LOW_PERCENT 30
#define EMG_BATTERY_LEVEL_CRITICAL_LOW_PERCENT 10
#define EMG_BATTERY_NOT_DETECTED_THRESHOLD_PERCENT 0
#define EMG_ANALOG_DEAD_RANGE_VALUE 205

#define DURATION_TEST_RESULT_STEP_S 120
#define LAMP_EMERGENCY_TIME_STEP_S (60 * 60)
#define LAMP_TOTAL_OPERATION_TIME_STEP_S (60 * 60 * 4)
#define PROLONG_TIME_STEP_S 30
#define INHIBIT_TIMER_EXPIRATION_TIME_S (15 * 60)
#define BATTERY_CHARGE_LEVEL_MAX (UINT8_MAX - 1)
#define FUNCTION_TEST_INTERVAL_DEFAULT_VAL 7
#define DURATION_TEST_INTERVAL_DEFAULT_VAL 52

#define EMERGENCY_MIN_LEVEL 1
#define EMERGENCY_MAX_LEVEL 254

#define ELT_DURATION_TEST_TIME_S (60 * 60)
#define ELT_FUNCTION_TEST_TIME_S 5

#define EMERGENCY_DRIVER_SIMULATOR_TASK_PERIOD_MS 1000


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

enum ModeOfOperation
{
    FUNCTION_TEST_IN_PROGRESS,
    DURATION_TEST_IN_PROGRESS,
    NORMAL_MODE,
    INHIBIT_MODE,
    REST_MODE,
    EMERGENCY_MODE,
    EXTENDED_EMERGENCY_MODE,
};


static bool                   IsInitialized = false;
static uint8_t                Dtr;
static uint8_t                EmergencyLevel;
static uint32_t               ProlongTimeSeconds;
static uint32_t               ProlongTimeCounterSeconds;
static uint32_t               InhibitModeTimeCounterSeconds;
static struct EmergencyStatus EmergencyStatus;
static struct FailureStatus   FailureStatus;
static struct EmergencyMode   EmergencyMode;
static uint32_t               LampEmergencyTimeSeconds;
static uint32_t               LampTotalOperationTimeSeconds;
static uint32_t               DurationTestResultSeconds;
static uint32_t               FunctionTestResultSeconds;
static uint8_t                FunctionTestIntervalDays;
static uint8_t                DurationTestIntervalDays;


static uint8_t              BatteryLevelSimulator(void);
static bool                 IsMainsPowerOnSimulator(void);
static bool                 ShortenDurationTestSimulator(void);
static void                 EmergencyDriverSimulator_Loop(void);
static void                 LoopFunctionTestInProgress(void);
static void                 LoopDurationTestInProgress(void);
static void                 LoopNormalMode(void);
static void                 LoopInhibitMode(void);
static void                 LoopRestMode(void);
static void                 LoopEmergencyMode(void);
static void                 LoopExtendedEmergencyMode(void);
static void                 UpdateBatteryFullyChargedInfo(void);
static void                 UpdateLampTotalOperationTime(void);
static void                 SetModeOfOperation(enum ModeOfOperation mode_of_operation);
static enum ModeOfOperation GetModeOfOperation(void);


void EmergencyDriverSimulator_Init(void)
{
    if (!AdcHal_IsInitialized())
    {
        AdcHal_Init();
    }

    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    GpioHal_PinMode(GPIO_HAL_PIN_ENCODER_SW, GPIO_HAL_MODE_INPUT_PULLUP);
    GpioHal_PinMode(GPIO_HAL_PIN_SW3, GPIO_HAL_MODE_INPUT_PULLUP);

    SetModeOfOperation(NORMAL_MODE);
    LampEmergencyTimeSeconds      = 0;
    LampTotalOperationTimeSeconds = 0;

    SimpleScheduler_TaskAdd(EMERGENCY_DRIVER_SIMULATOR_TASK_PERIOD_MS,
                            EmergencyDriverSimulator_Loop,
                            SIMPLE_SCHEDULER_TASK_ID_EMERGENCY_DRIVER_SIMULATOR,
                            false);

    // Auto testing feature is not implemented in this simulator. There is only
    // an interface to set/query test interval for presentation purposes, but
    // tests will not run, even when test interval has different value, than 0.
    FunctionTestIntervalDays = FUNCTION_TEST_INTERVAL_DEFAULT_VAL;
    DurationTestIntervalDays = DURATION_TEST_INTERVAL_DEFAULT_VAL;

    IsInitialized = true;
}

bool EmergencyDriverSimulator_IsInitialized(void)
{
    return IsInitialized;
}

void EmergencyDriverSimulator_Dtr(uint8_t dtr)
{
    Dtr = dtr;
}

void EmergencyDriverSimulator_Rest(void)
{
    if (GetModeOfOperation() == EMERGENCY_MODE)
    {
        SetModeOfOperation(REST_MODE);
    }
}

void EmergencyDriverSimulator_Inhibit(void)
{
    if (GetModeOfOperation() == NORMAL_MODE)
    {
        SetModeOfOperation(INHIBIT_MODE);
        InhibitModeTimeCounterSeconds = 0;
    }
    else if (GetModeOfOperation() == INHIBIT_MODE)
    {
        InhibitModeTimeCounterSeconds = 0;
    }
}

void EmergencyDriverSimulator_ReLightResetInhibit(void)
{
    if (GetModeOfOperation() == REST_MODE)
    {
        SetModeOfOperation(EMERGENCY_MODE);
    }
    else if (GetModeOfOperation() == INHIBIT_MODE)
    {
        SetModeOfOperation(NORMAL_MODE);
    }
}

void EmergencyDriverSimulator_StartFunctionTest(void)
{
    if (GetModeOfOperation() == NORMAL_MODE)
    {
        SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);
    }
    else
    {
        EmergencyStatus.function_test_pending = true;
    }
}

void EmergencyDriverSimulator_StartDurationTest(void)
{
    if ((GetModeOfOperation() == NORMAL_MODE) && (BatteryLevelSimulator() == BATTERY_LEVEL_MAX))
    {
        SetModeOfOperation(DURATION_TEST_IN_PROGRESS);
    }
    else
    {
        EmergencyStatus.duration_test_pending = true;
    }
}

void EmergencyDriverSimulator_StopTest(void)
{
    if (GetModeOfOperation() == DURATION_TEST_IN_PROGRESS)
    {
        SetModeOfOperation(NORMAL_MODE);
    }
    else if (GetModeOfOperation() == FUNCTION_TEST_IN_PROGRESS)
    {
        SetModeOfOperation(NORMAL_MODE);
    }

    EmergencyStatus.function_test_pending = false;
    EmergencyStatus.duration_test_pending = false;
}

void EmergencyDriverSimulator_ResetLampTime(void)
{
    LampEmergencyTimeSeconds      = 0;
    LampTotalOperationTimeSeconds = 0;
}

void EmergencyDriverSimulator_StoreDtrAsEmergencyLevel(void)
{
    EmergencyLevel = Dtr;
}

void EmergencyDriverSimulator_StoreFunctionTestInterval(void)
{
    FunctionTestIntervalDays = Dtr;
}

void EmergencyDriverSimulator_StoreDurationTestInterval(void)
{
    DurationTestIntervalDays = Dtr;
}

void EmergencyDriverSimulator_StoreProlongTime(void)
{
    ProlongTimeSeconds = Dtr * PROLONG_TIME_STEP_S;
}

uint8_t EmergencyDriverSimulator_QueryBatteryCharge(void)
{
    return ((BatteryLevelSimulator() * BATTERY_CHARGE_LEVEL_MAX / 100));
}

uint8_t EmergencyDriverSimulator_QueryDurationTestResult(void)
{
    if (DurationTestResultSeconds <= DURATION_TEST_RESULT_STEP_S * UINT8_MAX)
    {
        return DurationTestResultSeconds / DURATION_TEST_RESULT_STEP_S;
    }

    return UINT8_MAX;
}

uint8_t EmergencyDriverSimulator_QueryLampEmergencyTime(void)
{
    if (LampEmergencyTimeSeconds <= LAMP_EMERGENCY_TIME_STEP_S * UINT8_MAX)
    {
        return LampEmergencyTimeSeconds / LAMP_EMERGENCY_TIME_STEP_S;
    }

    return UINT8_MAX;
}

uint8_t EmergencyDriverSimulator_QueryLampTotalOperationTime(void)
{
    if (LampTotalOperationTimeSeconds <= LAMP_TOTAL_OPERATION_TIME_STEP_S * UINT8_MAX)
    {
        return LampTotalOperationTimeSeconds / LAMP_TOTAL_OPERATION_TIME_STEP_S;
    }

    return UINT8_MAX;
}

uint8_t EmergencyDriverSimulator_QueryEmergencyMinLevel(void)
{
    return EMERGENCY_MIN_LEVEL;
}

uint8_t EmergencyDriverSimulator_QueryEmergencyMaxLevel(void)
{
    return EMERGENCY_MAX_LEVEL;
}

uint8_t EmergencyDriverSimulator_QueryEmergencyMode(void)
{
    return (*(uint8_t *)(&EmergencyMode));
}

uint8_t EmergencyDriverSimulator_QueryFailureStatus(void)
{
    return (*(uint8_t *)(&FailureStatus));
}

uint8_t EmergencyDriverSimulator_QueryEmergencyStatus(void)
{
    return (*(uint8_t *)(&EmergencyStatus));
}

static bool IsMainsPowerOnSimulator(void)
{
    // Pressing encoder button simulates loss of power supply
    return GpioHal_PinRead(GPIO_HAL_PIN_ENCODER_SW);
}

static bool ShortenDurationTestSimulator(void)
{
    // Pressing button finishes duration test immediately
    return !GpioHal_PinRead(GPIO_HAL_PIN_SW3);
}

static uint8_t BatteryLevelSimulator(void)
{
    // Moving potentiometer position simulates battery charge level
    uint16_t analog_value = ADC_HAL_ADC_MAX - AdcHal_ReadChannel(ADC_HAL_CHANNEL_POTENTIOMETER);

    if (analog_value > ADC_HAL_ADC_MAX - EMG_ANALOG_DEAD_RANGE_VALUE)
    {
        analog_value = ADC_HAL_ADC_MAX - EMG_ANALOG_DEAD_RANGE_VALUE;
    }

    if (analog_value < EMG_ANALOG_DEAD_RANGE_VALUE)
    {
        analog_value = EMG_ANALOG_DEAD_RANGE_VALUE;
    }

    return (analog_value - EMG_ANALOG_DEAD_RANGE_VALUE) * BATTERY_LEVEL_MAX / (ADC_HAL_ADC_MAX - 2 * EMG_ANALOG_DEAD_RANGE_VALUE);
}

static void EmergencyDriverSimulator_Loop(void)
{
    enum ModeOfOperation mode_of_operation = GetModeOfOperation();

    switch (mode_of_operation)
    {
        case FUNCTION_TEST_IN_PROGRESS:
        {
            LoopFunctionTestInProgress();
            break;
        }
        case DURATION_TEST_IN_PROGRESS:
        {
            LoopDurationTestInProgress();
            break;
        }
        case NORMAL_MODE:
        {
            LoopNormalMode();
            break;
        }
        case INHIBIT_MODE:
        {
            LoopInhibitMode();
            break;
        }
        case REST_MODE:
        {
            LoopRestMode();
            break;
        }
        case EMERGENCY_MODE:
        {
            LoopEmergencyMode();
            break;
        }
        case EXTENDED_EMERGENCY_MODE:
        {
            LoopExtendedEmergencyMode();
            break;
        }
        default:
        {
            // Should not enter here
            ASSERT(false);
        }
    }

    UpdateBatteryFullyChargedInfo();
    UpdateLampTotalOperationTime();
}

static void LoopFunctionTestInProgress(void)
{
    if (!IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(EMERGENCY_MODE);
        return;
    }

    FunctionTestResultSeconds++;

    if (FunctionTestResultSeconds == ELT_FUNCTION_TEST_TIME_S)
    {
        EmergencyStatus.function_test_done_and_results_available = true;
        SetModeOfOperation(NORMAL_MODE);
    }
}

static void LoopDurationTestInProgress(void)
{
    if (!IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(EMERGENCY_MODE);
        return;
    }

    DurationTestResultSeconds++;

    if (DurationTestResultSeconds == ELT_DURATION_TEST_TIME_S)
    {
        EmergencyStatus.duration_test_done_and_results_available = true;
        SetModeOfOperation(NORMAL_MODE);
    }

    if (ShortenDurationTestSimulator())
    {
        DurationTestResultSeconds                                = ELT_DURATION_TEST_TIME_S;
        EmergencyStatus.duration_test_done_and_results_available = true;
        SetModeOfOperation(NORMAL_MODE);
    }

    if (BatteryLevelSimulator() == 0)
    {
        EmergencyStatus.duration_test_done_and_results_available = true;
        FailureStatus.battery_failure                            = true;
        SetModeOfOperation(NORMAL_MODE);
    }
}

static void LoopNormalMode(void)
{
    if (!IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(EMERGENCY_MODE);
        return;
    }

    if (EmergencyStatus.function_test_pending && (BatteryLevelSimulator() == 100))
    {
        SetModeOfOperation(FUNCTION_TEST_IN_PROGRESS);
    }
    else if (EmergencyStatus.duration_test_pending && (BatteryLevelSimulator() == 100))
    {
        SetModeOfOperation(DURATION_TEST_IN_PROGRESS);
    }
}

static void LoopInhibitMode(void)
{
    if (!IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(REST_MODE);
    }
    else if (InhibitModeTimeCounterSeconds == INHIBIT_TIMER_EXPIRATION_TIME_S)
    {
        SetModeOfOperation(NORMAL_MODE);
    }
    else
    {
        InhibitModeTimeCounterSeconds++;
    }
}

static void LoopRestMode(void)
{
    if (IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(NORMAL_MODE);
    }
}

static void LoopEmergencyMode(void)
{
    if (IsMainsPowerOnSimulator() && (ProlongTimeSeconds != 0))
    {
        SetModeOfOperation(EXTENDED_EMERGENCY_MODE);
        ProlongTimeCounterSeconds = 0;
    }
    else if (IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(NORMAL_MODE);
        ProlongTimeCounterSeconds = 0;
    }
    else if (BatteryLevelSimulator() == 0)
    {
        SetModeOfOperation(REST_MODE);
    }
    else
    {
        LampEmergencyTimeSeconds++;
    }
}

static void LoopExtendedEmergencyMode(void)
{
    if (!IsMainsPowerOnSimulator())
    {
        SetModeOfOperation(EMERGENCY_MODE);
        return;
    }

    if (ProlongTimeCounterSeconds == ProlongTimeSeconds)
    {
        SetModeOfOperation(NORMAL_MODE);
    }
    else if (BatteryLevelSimulator() == 0)
    {
        SetModeOfOperation(NORMAL_MODE);
    }
    else
    {
        ProlongTimeCounterSeconds++;
    }
}

static void UpdateBatteryFullyChargedInfo(void)
{
    if (BatteryLevelSimulator() == 100)
    {
        EmergencyStatus.battery_charged = true;
    }
    else
    {
        EmergencyStatus.battery_charged = false;
    }
}

static void UpdateLampTotalOperationTime(void)
{
    LampTotalOperationTimeSeconds++;
}

static void SetModeOfOperation(enum ModeOfOperation mode_of_operation)
{
    switch (mode_of_operation)
    {
        case FUNCTION_TEST_IN_PROGRESS:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.mode_function_test_in_progress_active      = true;
            EmergencyStatus.inhibit_mode_active                      = false;
            EmergencyStatus.function_test_done_and_results_available = false;
            EmergencyStatus.function_test_pending                    = false;
            FunctionTestResultSeconds                                = 0;
            break;
        }
        case DURATION_TEST_IN_PROGRESS:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.mode_duration_test_in_progress_active      = true;
            EmergencyStatus.inhibit_mode_active                      = false;
            EmergencyStatus.duration_test_done_and_results_available = false;
            EmergencyStatus.duration_test_pending                    = false;
            DurationTestResultSeconds                                = 0;
            break;
        }
        case NORMAL_MODE:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.normal_mode_active    = true;
            EmergencyStatus.inhibit_mode_active = false;
            break;
        }
        case INHIBIT_MODE:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.normal_mode_active    = true;
            EmergencyStatus.inhibit_mode_active = true;
            break;
        }
        case REST_MODE:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.rest_mode_active = true;
            break;
        }
        case EMERGENCY_MODE:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.emergency_mode_active = true;
            EmergencyStatus.inhibit_mode_active = false;
            break;
        }
        case EXTENDED_EMERGENCY_MODE:
        {
            memset(&EmergencyMode, 0, sizeof(EmergencyMode));
            EmergencyMode.extended_emergency_mode_active = true;
            EmergencyStatus.inhibit_mode_active          = false;
            break;
        }
        default:
        {
            // Should not enter here
            ASSERT(false);
        }
    }
}

static enum ModeOfOperation GetModeOfOperation(void)
{
    if (EmergencyMode.normal_mode_active && EmergencyStatus.inhibit_mode_active)
    {
        return INHIBIT_MODE;
    }
    else if (EmergencyMode.normal_mode_active)
    {
        return NORMAL_MODE;
    }
    else if (EmergencyMode.rest_mode_active)
    {
        return REST_MODE;
    }
    else if (EmergencyMode.emergency_mode_active)
    {
        return EMERGENCY_MODE;
    }
    else if (EmergencyMode.extended_emergency_mode_active)
    {
        return EXTENDED_EMERGENCY_MODE;
    }
    else if (EmergencyMode.mode_function_test_in_progress_active)
    {
        return FUNCTION_TEST_IN_PROGRESS;
    }
    else if (EmergencyMode.mode_duration_test_in_progress_active)
    {
        return DURATION_TEST_IN_PROGRESS;
    }

    // Should not enter here
    ASSERT(false);
    return NORMAL_MODE;
}
