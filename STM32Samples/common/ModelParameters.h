#ifndef MODEL_PARAMETERS_H
#define MODEL_PARAMETERS_H

#include <stdbool.h>
#include <stdint.h>

#include "Utils.h"

struct PACKED ModelParametersLightCtlServer
{
    uint16_t min_temperature_range;
    uint16_t max_temperature_range;
};

struct PACKED ModelParametersSensorServerRow
{
    uint16_t property_id;
    uint16_t positive_tolerance;
    uint16_t negative_tolerance;
    uint8_t  sampling_function;
    uint8_t  mesurement_period;
    uint8_t  update_interval;
};

struct PACKED ModelParametersSensorServer
{
    uint8_t                               multisensor;
    struct ModelParametersSensorServerRow p_sensor[];
};

struct PACKED ModelParametersSensorServer1Sensor
{
    uint8_t                               multisensor;
    struct ModelParametersSensorServerRow sensor[1];
};

struct PACKED ModelParametersSensorServer2Sensor
{
    uint8_t                               multisensor;
    struct ModelParametersSensorServerRow sensor[2];
};

struct PACKED ModelParametersHealthServer
{
    uint16_t number_of_company_ids;
    uint16_t p_company_id_list[];
};

struct PACKED ModelParametersHealthServer1Id
{
    uint8_t  number_of_company_ids;
    uint16_t company_id_list[1];
};

struct PACKED ModelParametersTimeServer
{
    uint8_t  flags;
    uint16_t rtc_accuracy;
};

STATIC_ASSERT(sizeof(struct ModelParametersLightCtlServer) == 4, Wrong_size_of_the_struct_ModelParametersLightCtlServer);
STATIC_ASSERT(sizeof(struct ModelParametersSensorServerRow) == 9, Wrong_size_of_the_struct_ModelParametersSensorServerRow);
STATIC_ASSERT(sizeof(struct ModelParametersSensorServer) == 1, Wrong_size_of_the_struct_ModelParametersSensorServer);
STATIC_ASSERT(sizeof(struct ModelParametersSensorServer1Sensor) == 10, Wrong_size_of_the_struct_ModelParametersSensorServer1Sensor);
STATIC_ASSERT(sizeof(struct ModelParametersSensorServer2Sensor) == 19, Wrong_size_of_the_struct_ModelParametersSensorServer1Sensor);
STATIC_ASSERT(sizeof(struct ModelParametersHealthServer) == 2, Wrong_size_of_the_struct_ModelParametersHealthServer);
STATIC_ASSERT(sizeof(struct ModelParametersHealthServer1Id) == 3, Wrong_size_of_the_struct_ModelParametersHealthServer1Id);
STATIC_ASSERT(sizeof(struct ModelParametersTimeServer) == 3, Wrong_size_of_the_struct_ModelParametersTimeServer);

#endif
