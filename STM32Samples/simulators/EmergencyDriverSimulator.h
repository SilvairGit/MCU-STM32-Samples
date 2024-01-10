#ifndef EMERGENCY_DRIVER_SIMULATOR_H
#define EMERGENCY_DRIVER_SIMULATOR_H

#include <stdbool.h>
#include <stdint.h>

void EmergencyDriverSimulator_Init(void);
bool EmergencyDriverSimulator_IsInitialized(void);

void EmergencyDriverSimulator_Dtr(uint8_t dtr);

void    EmergencyDriverSimulator_Rest(void);
void    EmergencyDriverSimulator_Inhibit(void);
void    EmergencyDriverSimulator_ReLightResetInhibit(void);
void    EmergencyDriverSimulator_StartFunctionTest(void);
void    EmergencyDriverSimulator_StartDurationTest(void);
void    EmergencyDriverSimulator_StopTest(void);
void    EmergencyDriverSimulator_ResetLampTime(void);
void    EmergencyDriverSimulator_StoreDtrAsEmergencyLevel(void);
void    EmergencyDriverSimulator_StoreFunctionTestInterval(void);
void    EmergencyDriverSimulator_StoreDurationTestInterval(void);
void    EmergencyDriverSimulator_StoreProlongTime(void);
uint8_t EmergencyDriverSimulator_QueryBatteryCharge(void);
uint8_t EmergencyDriverSimulator_QueryDurationTestResult(void);
uint8_t EmergencyDriverSimulator_QueryLampEmergencyTime(void);
uint8_t EmergencyDriverSimulator_QueryLampTotalOperationTime(void);
uint8_t EmergencyDriverSimulator_QueryEmergencyMinLevel(void);
uint8_t EmergencyDriverSimulator_QueryEmergencyMaxLevel(void);
uint8_t EmergencyDriverSimulator_QueryEmergencyMode(void);
uint8_t EmergencyDriverSimulator_QueryFailureStatus(void);
uint8_t EmergencyDriverSimulator_QueryEmergencyStatus(void);

#endif
