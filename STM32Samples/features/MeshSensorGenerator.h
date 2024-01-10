#ifndef MESH_SENSOR_GENERATOR_H
#define MESH_SENSOR_GENERATOR_H


#include <stdint.h>

#include "Config.h"
#include "Sensor.h"

#if ENABLE_PIRALS == 1
#define PIR_REGISTRATION_ORDER 1 /**< Defines sensor servers registration order */
#define ALS_REGISTRATION_ORDER 2 /**< Defines sensor servers registration order */
#else
#define PIR_REGISTRATION_ORDER 0 /**< Defines sensor servers registration order */
#define ALS_REGISTRATION_ORDER 0 /**< Defines sensor servers registration order */
#endif

#define CURR_ENERGY_REGISTRATION_ORDER (ALS_REGISTRATION_ORDER + 1)        /**< Defines sensor servers registration order */
#define VOLT_POWER_REGISTRATION_ORDER (CURR_ENERGY_REGISTRATION_ORDER + 1) /**< Defines sensor servers registration order */

/*
 *  Setup Mesh Sensor Generator Hardware
 */
void MeshSensorGenerator_Setup(void);

#endif    // MESH_SENSOR_GENERATOR_H
