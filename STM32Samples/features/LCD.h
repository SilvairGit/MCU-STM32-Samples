#ifndef LCD_H
#define LCD_H

#include <stdbool.h>

#include "MCU_Definitions.h"
#include "Sensor.h"

// Setup LCD hardware
void LCD_Setup(void);

// Update displayed Modem state
void LCD_UpdateModemState(enum ModemState modemState);

// Update displayed Modem FW version
void LCD_UpdateModemFwVersion(char *fwVersion, uint8_t fwVerLen);

// Update displayed Sensor value
void LCD_UpdateSensorValue(enum SensorProperty sensorProperty, union SensorValue sensorValue);

// Update DFU in progress state
void LCD_UpdateDfuState(bool dfuInProgress);

// Sets Sensors values to Unknown state on LCD screen
void LCD_EraseSensorsValues(void);

#endif
