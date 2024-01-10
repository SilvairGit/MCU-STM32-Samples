#include "EnergySensorSimulator.h"

#include "Assert.h"
#include "Log.h"
#include "Luminaire.h"
#include "SimpleScheduler.h"

#define ENERGY_SENSOR_SIMULATOR_TASK_PERIOD_MS 1000
#define ENERGY_SENSOR_SIMULATOR_VOLTAGE_V 230
#define ENERGY_SENSOR_SIMULATOR_IDLE_POWER_W 4
#define ENERGY_SENSOR_SIMULATOR_MAX_POWER_W 40
#define ENERGY_SENSOR_SIMULATOR_SECONDS_IN_HOUR 3600

static bool     IsInitialized         = false;
static uint64_t AccumulatedEnergy_uWh = 0;

static void EnergySensorSimulator_Loop(void);

void EnergySensorSimulator_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("EnergySensorSimulator initialization");

    SimpleScheduler_TaskAdd(ENERGY_SENSOR_SIMULATOR_TASK_PERIOD_MS, EnergySensorSimulator_Loop, SIMPLE_SCHEDULER_TASK_ID_ENERGY_SENSOR_SIMULATOR, true);

    IsInitialized = true;
}

bool EnergySensorSimulator_IsInitialized(void)
{
    return IsInitialized;
}

uint32_t EnergySensorSimulator_GetVoltage_mV(void)
{
    return (ENERGY_SENSOR_SIMULATOR_VOLTAGE_V * 1000);
}

uint32_t EnergySensorSimulator_GetCurrent_mA(void)
{
    uint32_t power_mw   = EnergySensorSimulator_GetPower_mW();
    uint32_t voltage_mv = EnergySensorSimulator_GetVoltage_mV();

    return (uint32_t)((power_mw * 1000) / voltage_mv);
}

uint32_t EnergySensorSimulator_GetPower_mW(void)
{
    uint16_t duty_cycle = Luminaire_GetDutyCycle();
    float    power      = ((float)duty_cycle / UINT16_MAX);

    return (uint32_t)((power * (ENERGY_SENSOR_SIMULATOR_MAX_POWER_W - ENERGY_SENSOR_SIMULATOR_IDLE_POWER_W) + ENERGY_SENSOR_SIMULATOR_IDLE_POWER_W) * 1000);
}

uint32_t EnergySensorSimulator_GetEnergy_Wh(void)
{
    return (uint32_t)(AccumulatedEnergy_uWh / 1000000);
}

static void EnergySensorSimulator_Loop(void)
{
    uint32_t power_mw = EnergySensorSimulator_GetPower_mW();
    AccumulatedEnergy_uWh += (uint64_t)((power_mw * 1000) / ENERGY_SENSOR_SIMULATOR_SECONDS_IN_HOUR);
}
