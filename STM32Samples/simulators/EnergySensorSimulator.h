#ifndef ENERGY_SENSOR_SIMULATOR_H
#define ENERGY_SENSOR_SIMULATOR_H

#include <stdbool.h>
#include <stdint.h>

void     EnergySensorSimulator_Init(void);
bool     EnergySensorSimulator_IsInitialized(void);
uint32_t EnergySensorSimulator_GetVoltage_mV(void);
uint32_t EnergySensorSimulator_GetCurrent_mA(void);
uint32_t EnergySensorSimulator_GetPower_mW(void);
uint32_t EnergySensorSimulator_GetEnergy_Wh(void);

#endif
