#include "MeshSensorGenerator.h"

#include "AdcHal.h"
#include "Config.h"
#include "EnergySensorSimulator.h"
#include "GpioHal.h"
#include "Log.h"
#include "Luminaire.h"
#include "Mesh.h"
#include "ModelManager.h"
#include "SimpleScheduler.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Utils.h"

#define MESH_TOLERANCE(_error) ((uint16_t)((4095 * _error) / 100))
#define PIR_POSITIVE_TOLERANCE MESH_TOLERANCE(0)
#define PIR_NEGATIVE_TOLERANCE MESH_TOLERANCE(0)
#define PIR_SAMPLING_FUNCTION 0x01
#define PIR_MEASUREMENT_PERIOD 0x00
#define PIR_UPDATE_INTERVAL 0x2F    // Warning! This variable must be align with SENSOR_UPDATE_INTV_PIR
#define ALS_POSITIVE_TOLERANCE MESH_TOLERANCE(0)
#define ALS_NEGATIVE_TOLERANCE MESH_TOLERANCE(0)
#define ALS_SAMPLING_FUNCTION 0x01
#define ALS_MEASUREMENT_PERIOD 0x00
#define ALS_UPDATE_INTERVAL 0x2F    // Warning! This variable must be align with SENSOR_UPDATE_INTV_ALS
#define VOLTAGE_SENSOR_POSITIVE_TOLERANCE MESH_TOLERANCE(0.5)
#define VOLTAGE_SENSOR_NEGATIVE_TOLERANCE MESH_TOLERANCE(0.5)
#define VOLTAGE_SENSOR_SAMPLING_FUNCTION 0x03
#define VOLTAGE_SENSOR_MEASUREMENT_PERIOD 0x00
#define VOLTAGE_SENSOR_UPDATE_INTERVAL 0x40    // Warning! This variable must be align with SENSOR_UPDATE_INTV_VOLT_POWER
#define CURRENT_SENSOR_POSITIVE_TOLERANCE MESH_TOLERANCE(0.5)
#define CURRENT_SENSOR_NEGATIVE_TOLERANCE MESH_TOLERANCE(0.5)
#define CURRENT_SENSOR_SAMPLING_FUNCTION 0x03
#define CURRENT_SENSOR_MEASUREMENT_PERIOD 0x00
#define CURRENT_SENSOR_UPDATE_INTERVAL 0x40    // Warning! This variable must be align with SENSOR_UPDATE_INTV_CURR_ENERGY
#define POWER_SENSOR_POSITIVE_TOLERANCE MESH_TOLERANCE(1)
#define POWER_SENSOR_NEGATIVE_TOLERANCE MESH_TOLERANCE(1)
#define POWER_SENSOR_SAMPLING_FUNCTION 0x03
#define POWER_SENSOR_MEASUREMENT_PERIOD 0x00
#define POWER_SENSOR_UPDATE_INTERVAL 0x40
#define ENERGY_SENSOR_POSITIVE_TOLERANCE MESH_TOLERANCE(1)
#define ENERGY_SENSOR_NEGATIVE_TOLERANCE MESH_TOLERANCE(1)
#define ENERGY_SENSOR_SAMPLING_FUNCTION 0x03
#define ENERGY_SENSOR_MEASUREMENT_PERIOD 0x00
#define ENERGY_SENSOR_UPDATE_INTERVAL 0x40


#define SENSOR_INPUT_TASK_PERIOD_MS 1

#define ALS_CONVERSION_COEFFICIENT 14UL    /**< light sensor coefficient [centilux / millivolt] */
#define ALS_MAX_MODEL_VALUE (0xFFFFFF - 1) /**<  Maximal allowed value of ALS reading passed to model */
#define PIR_INERTIA_MS 4000                /**< PIR inertia in milliseconds */
#define ALS_REPORT_THRESHOLD 500           /**< sensor threshold in centilux */

#define SENSOR_UPDATE_INTV_PIR_MS 200 /**< sensor update in milliseconds for PIR Sensor */
STATIC_ASSERT(PIR_UPDATE_INTERVAL == 0x2F, SENSOR_UPDATE_INTV_PIR_MS_and_PIR_UPDATE_INTERVAL_must_be_aligned);

#define SENSOR_UPDATE_INTV_ALS_MS 200 /**< sensor update in milliseconds for ALS Sensor */
STATIC_ASSERT(ALS_UPDATE_INTERVAL == 0x2F, SENSOR_UPDATE_INTV_ALS_MS_and_ALS_UPDATE_INTERVAL_must_be_aligned);

#define SENSOR_UPDATE_INTV_CURR_ENERGY_MS 1000 /**< sensor update in milliseconds for Current and Energy Sensor */
STATIC_ASSERT(CURRENT_SENSOR_UPDATE_INTERVAL == 0x40, CURRENT_SENSOR_UPDATE_INTERVAL_and_SENSOR_UPDATE_INTV_CURR_ENERGY_MS_must_be_aligned);

#define SENSOR_UPDATE_INTV_VOLT_POWER_MS 1000 /**< sensor update in milliseconds for Voltage and Power Sensor */
STATIC_ASSERT(VOLTAGE_SENSOR_UPDATE_INTERVAL == 0x40, SENSOR_UPDATE_INTV_VOLT_POWER_MS_and_SENSOR_UPDATE_INTV_VOLT_POWER_MS_must_be_aligned);

static bool              IsEnabled                = false;
static volatile uint32_t PirTimestamp             = 0;
static uint8_t           SensorInputPirIdx        = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
static uint8_t           SensorInputAlsIdx        = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
static uint8_t           SensorInputCurrEnergyIdx = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
static uint8_t           SensorInputVoltPowIdx    = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

static const struct ModelParametersSensorServer1Sensor pir_registration_parameters = {.multisensor = 0x01,    //Number of sensors

                                                                                      .sensor = {{
                                                                                          .property_id = MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENCE_DETECTED,
                                                                                          .positive_tolerance = PIR_POSITIVE_TOLERANCE,
                                                                                          .negative_tolerance = PIR_NEGATIVE_TOLERANCE,
                                                                                          .sampling_function  = PIR_SAMPLING_FUNCTION,
                                                                                          .mesurement_period  = PIR_MEASUREMENT_PERIOD,
                                                                                          .update_interval    = PIR_UPDATE_INTERVAL,
                                                                                      }}};

static const struct ModelParametersSensorServer1Sensor als_registration_parameters =
    {.multisensor = 0x01,    //Number of sensors

     .sensor = {{
         .property_id        = MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_AMBIENT_LIGHT_LEVEL,
         .positive_tolerance = ALS_POSITIVE_TOLERANCE,
         .negative_tolerance = ALS_NEGATIVE_TOLERANCE,
         .sampling_function  = ALS_SAMPLING_FUNCTION,
         .mesurement_period  = ALS_MEASUREMENT_PERIOD,
         .update_interval    = ALS_UPDATE_INTERVAL,
     }}};

static const struct ModelParametersSensorServer2Sensor current_energy_registration_parameters =
    {.multisensor = 0x02,    //Number of sensors

     .sensor = {{
                    .property_id        = MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_INPUT_CURRENT,
                    .positive_tolerance = CURRENT_SENSOR_POSITIVE_TOLERANCE,
                    .negative_tolerance = CURRENT_SENSOR_NEGATIVE_TOLERANCE,
                    .sampling_function  = CURRENT_SENSOR_SAMPLING_FUNCTION,
                    .mesurement_period  = CURRENT_SENSOR_MEASUREMENT_PERIOD,
                    .update_interval    = CURRENT_SENSOR_UPDATE_INTERVAL,
                },
                {
                    .property_id        = MODEL_MANAGER_SENSOR_SERVER_PROP_ID_TOTAL_DEVICE_ENERGY_USE,
                    .positive_tolerance = ENERGY_SENSOR_POSITIVE_TOLERANCE,
                    .negative_tolerance = ENERGY_SENSOR_NEGATIVE_TOLERANCE,
                    .sampling_function  = ENERGY_SENSOR_SAMPLING_FUNCTION,
                    .mesurement_period  = ENERGY_SENSOR_MEASUREMENT_PERIOD,
                    .update_interval    = ENERGY_SENSOR_UPDATE_INTERVAL,
                }}};

static const struct ModelParametersSensorServer2Sensor voltage_power_registration_parameters =
    {.multisensor = 0x02,    //Number of sensors

     .sensor = {{
                    .property_id        = MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_INPUT_VOLTAGE,
                    .positive_tolerance = VOLTAGE_SENSOR_POSITIVE_TOLERANCE,
                    .negative_tolerance = VOLTAGE_SENSOR_NEGATIVE_TOLERANCE,
                    .sampling_function  = VOLTAGE_SENSOR_SAMPLING_FUNCTION,
                    .mesurement_period  = VOLTAGE_SENSOR_MEASUREMENT_PERIOD,
                    .update_interval    = VOLTAGE_SENSOR_UPDATE_INTERVAL,
                },
                {
                    .property_id        = MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_DEVICE_INPUT_POWER,
                    .positive_tolerance = POWER_SENSOR_POSITIVE_TOLERANCE,
                    .negative_tolerance = POWER_SENSOR_NEGATIVE_TOLERANCE,
                    .sampling_function  = POWER_SENSOR_SAMPLING_FUNCTION,
                    .mesurement_period  = POWER_SENSOR_MEASUREMENT_PERIOD,
                    .update_interval    = POWER_SENSOR_UPDATE_INTERVAL,
                }}};

struct ModelManagerRegistrationRow ModelConfigSensorServerPir = {
    .model_id            = MODEL_MANAGER_ID_SENSOR_SERVER,
    .p_model_parameter   = (uint8_t *)&pir_registration_parameters,
    .model_parameter_len = sizeof(pir_registration_parameters),
    .p_instance_index    = &SensorInputPirIdx,
};

struct ModelManagerRegistrationRow ModelConfigSensorServerAls = {
    .model_id            = MODEL_MANAGER_ID_SENSOR_SERVER,
    .p_model_parameter   = (uint8_t *)&als_registration_parameters,
    .model_parameter_len = sizeof(als_registration_parameters),
    .p_instance_index    = &SensorInputAlsIdx,
};

struct ModelManagerRegistrationRow ModelConfigSensorServerEnergy = {
    .model_id            = MODEL_MANAGER_ID_SENSOR_SERVER,
    .p_model_parameter   = (uint8_t *)&current_energy_registration_parameters,
    .model_parameter_len = sizeof(current_energy_registration_parameters),
    .p_instance_index    = &SensorInputCurrEnergyIdx,
};

struct ModelManagerRegistrationRow ModelConfigSensorServerVoltagePower = {
    .model_id            = MODEL_MANAGER_ID_SENSOR_SERVER,
    .p_model_parameter   = (uint8_t *)&voltage_power_registration_parameters,
    .model_parameter_len = sizeof(voltage_power_registration_parameters),
    .p_instance_index    = &SensorInputVoltPowIdx,
};

static void ProcessPIR(void);
static void ProcessALS(void);
static void ProcessCurrentEnergy(void);
static void ProcessVoltagePower(void);

static void Sensor_Loop(void);

void InterruptPIR(void)
{
    PirTimestamp = Timestamp_GetCurrent();
}

void MeshSensorGenerator_Setup(void)
{
    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    if (!AdcHal_IsInitialized())
    {
        AdcHal_Init();
    }

    GpioHal_PinMode(GPIO_HAL_PIN_PIR, GPIO_HAL_MODE_INPUT);

    GpioHal_SetPinIrq(GPIO_HAL_PIN_PIR, GPIO_HAL_IRQ_EDGE_RISING, InterruptPIR);

#if ENABLE_PIRALS
    ModelManager_RegisterModel(&ModelConfigSensorServerPir);
    ModelManager_RegisterModel(&ModelConfigSensorServerAls);
#endif
#if ENABLE_ENERGY
    EnergySensorSimulator_Init();

    ModelManager_RegisterModel(&ModelConfigSensorServerEnergy);
    ModelManager_RegisterModel(&ModelConfigSensorServerVoltagePower);
#endif

    SimpleScheduler_TaskAdd(SENSOR_INPUT_TASK_PERIOD_MS, Sensor_Loop, SIMPLE_SCHEDULER_TASK_ID_SENSOR_INPUT, false);

    IsEnabled = true;
}

static void Sensor_Loop(void)
{
    if (!IsEnabled)
        return;

    static unsigned long timestamp_pir         = 0;
    static unsigned long timestamp_als         = 0;
    static unsigned long timestamp_curr_energy = 0;
    static unsigned long timestamp_volt_power  = 0;

    if (Timestamp_GetTimeElapsed(timestamp_pir, Timestamp_GetCurrent()) >= SENSOR_UPDATE_INTV_PIR_MS)
    {
        timestamp_pir = Timestamp_GetCurrent();
        ProcessPIR();
    }
    if (Timestamp_GetTimeElapsed(timestamp_als, Timestamp_GetCurrent()) >= SENSOR_UPDATE_INTV_ALS_MS)
    {
        timestamp_als = Timestamp_GetCurrent();
        ProcessALS();
    }
    if (Timestamp_GetTimeElapsed(timestamp_curr_energy, Timestamp_GetCurrent()) >= SENSOR_UPDATE_INTV_CURR_ENERGY_MS)
    {
        timestamp_curr_energy = Timestamp_GetCurrent();
        ProcessCurrentEnergy();
    }
    if (Timestamp_GetTimeElapsed(timestamp_volt_power, Timestamp_GetCurrent()) >= SENSOR_UPDATE_INTV_VOLT_POWER_MS)
    {
        timestamp_volt_power = Timestamp_GetCurrent();
        ProcessVoltagePower();
    }
}

static void ProcessPIR(void)
{
    if (SensorInputPirIdx != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN || Luminaire_IsStartupBehaviorInProgress())
    {
        bool pir = GpioHal_PinRead(GPIO_HAL_PIN_PIR) || (Timestamp_GetTimeElapsed(PirTimestamp, Timestamp_GetCurrent()) < PIR_INERTIA_MS);

        uint8_t pir_buf[] = {
            SensorInputPirIdx,
            LOW_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENCE_DETECTED),
            HIGH_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENCE_DETECTED),
            pir,
        };
        UartProtocol_Send(UART_FRAME_CMD_SENSOR_UPDATE_REQUEST, pir_buf, sizeof(pir_buf));
    }
}

static void ProcessALS(void)
{
    if (SensorInputAlsIdx != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN || Luminaire_IsStartupBehaviorInProgress())
    {
        uint32_t als_centilux = AdcHal_ReadChannelMv(ADC_HAL_CHANNEL_ALS, ALS_CONVERSION_COEFFICIENT);

        /*
        * Sensor Server can be configured to report on change. In one mode report is triggered by
        * percentage change from the actual value. In case of small measurement, it can generate heavy traffic.
        */
        if (als_centilux < ALS_REPORT_THRESHOLD)
        {
            als_centilux = 0;
        }

        if (als_centilux > ALS_MAX_MODEL_VALUE)
        {
            als_centilux = ALS_MAX_MODEL_VALUE;
        }

        uint8_t als_buf[] = {
            SensorInputAlsIdx,
            LOW_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_AMBIENT_LIGHT_LEVEL),
            HIGH_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_AMBIENT_LIGHT_LEVEL),
            (uint8_t)als_centilux,
            (uint8_t)(als_centilux >> 8),
            (uint8_t)(als_centilux >> 16),
        };

        UartProtocol_Send(UART_FRAME_CMD_SENSOR_UPDATE_REQUEST, als_buf, sizeof(als_buf));
    }
}

static void ProcessCurrentEnergy(void)
{
    uint16_t current = EnergySensorSimulator_GetCurrent_mA() / 10;
    uint32_t energy  = EnergySensorSimulator_GetEnergy_Wh() / 1000;

    if (SensorInputCurrEnergyIdx != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN)
    {
        uint8_t currenergy_buf[] = {
            SensorInputCurrEnergyIdx,
            LOW_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_INPUT_CURRENT),
            HIGH_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_INPUT_CURRENT),
            (uint8_t)current,
            (uint8_t)(current >> 8),
            LOW_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_TOTAL_DEVICE_ENERGY_USE),
            HIGH_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_TOTAL_DEVICE_ENERGY_USE),
            (uint8_t)energy,
            (uint8_t)(energy >> 8),
            (uint8_t)(energy >> 16),
        };
        UartProtocol_Send(UART_FRAME_CMD_SENSOR_UPDATE_REQUEST, currenergy_buf, sizeof(currenergy_buf));
    }
}

static void ProcessVoltagePower(void)
{
    uint16_t voltage = (uint16_t)((float)EnergySensorSimulator_GetVoltage_mV() * 64 / 1000);
    uint32_t power   = EnergySensorSimulator_GetPower_mW() / 100;

    if (SensorInputVoltPowIdx != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN)
    {
        uint8_t voltpow_buf[] = {
            SensorInputVoltPowIdx,
            LOW_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_INPUT_VOLTAGE),
            HIGH_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_INPUT_VOLTAGE),
            (uint8_t)voltage,
            (uint8_t)(voltage >> 8),
            LOW_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_DEVICE_INPUT_POWER),
            HIGH_BYTE(MODEL_MANAGER_SENSOR_SERVER_PROP_ID_PRESENT_DEVICE_INPUT_POWER),
            (uint8_t)power,
            (uint8_t)(power >> 8),
            (uint8_t)(power >> 16),
        };
        UartProtocol_Send(UART_FRAME_CMD_SENSOR_UPDATE_REQUEST, voltpow_buf, sizeof(voltpow_buf));
    }
}
