#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

#define MESH_PROP_PRESENT_AMBIENT_LIGHT_LEVEL_UNKNOWN_VAL 0xFFFFFF
#define MESH_PROP_PRESENT_DEVICE_INPUT_POWER_UNKNOWN_VAL 0xFFFFFF
#define MESH_PROP_PRESENT_INPUT_CURRENT_UNKNOWN_VAL 0xFFFF
#define MESH_PROP_PRESENT_INPUT_VOLTAGE_UNKNOWN_VAL 0xFFFF
#define MESH_PROP_TOTAL_DEVICE_ENERGY_USE_UNKNOWN_VAL 0xFFFFFF
#define MESH_PROP_PRECISE_TOTAL_DEVICE_ENERGY_USE_UNKNOWN_VAL 0xFFFFFFFF
#define MESH_PROP_PRECISE_TOTAL_DEVICE_ENERGY_USE_NOT_VALID_VAL 0xFFFFFFFE

union SensorValue
{
    uint32_t als;
    uint8_t  pir;
    uint32_t power;
    uint16_t current;
    uint16_t voltage;
    uint32_t energy;
    uint32_t precise_energy;
};

enum SensorProperty
{
    PRESENCE_DETECTED               = 0x004D,
    PRESENT_AMBIENT_LIGHT_LEVEL     = 0x004E,
    PRESENT_DEVICE_INPUT_POWER      = 0x0052,
    PRESENT_INPUT_CURRENT           = 0x0057,
    PRESENT_INPUT_VOLTAGE           = 0x0059,
    TOTAL_DEVICE_ENERGY_USE         = 0x006A,
    PRECISE_TOTAL_DEVICE_ENERGY_USE = 0x0072
};

#endif
