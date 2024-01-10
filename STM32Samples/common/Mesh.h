#ifndef MESH_H
#define MESH_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "MeshSensorGenerator.h"


/**
 * Supported Mesh Opcodes definitions
 */
#define MESH_MESSAGE_GENERIC_ONOFF_GET 0x8201
#define MESH_MESSAGE_GENERIC_ONOFF_SET 0x8202
#define MESH_MESSAGE_GENERIC_ONOFF_SET_UNACKNOWLEDGED 0x8203
#define MESH_MESSAGE_GENERIC_ONOFF_STATUS 0x8204
#define MESH_MESSAGE_GENERIC_LEVEL_GET 0x8205
#define MESH_MESSAGE_GENERIC_LEVEL_SET 0x8206
#define MESH_MESSAGE_GENERIC_LEVEL_SET_UNACKNOWLEDGED 0x8207
#define MESH_MESSAGE_GENERIC_LEVEL_STATUS 0x8208
#define MESH_MESSAGE_GENERIC_DELTA_SET 0x8209
#define MESH_MESSAGE_GENERIC_DELTA_SET_UNACKNOWLEDGED 0x820A
#define MESH_MESSAGE_LIGHT_L_GET 0x824B
#define MESH_MESSAGE_LIGHT_L_SET 0x824C
#define MESH_MESSAGE_LIGHT_L_SET_UNACKNOWLEDGED 0x824D
#define MESH_MESSAGE_LIGHT_L_STATUS 0x824E
#define MESH_MESSAGE_LIGHT_LC_MODE_GET 0x8291
#define MESH_MESSAGE_LIGHT_LC_MODE_SET 0x8292
#define MESH_MESSAGE_LIGHT_LC_MODE_SET_UNACKNOWLEDGED 0x8293
#define MESH_MESSAGE_LIGHT_LC_MODE_STATUS 0x8294
#define MESH_MESSAGE_SENSOR_STATUS 0x0052
#define MESH_MESSAGE_LEVEL_STATUS 0x8208
#define MESH_MESSAGE_LIGHT_CTL_TEMPERATURE_STATUS 0x8266
#define MESH_MESSAGE_LEVEL_GET 0x8205
#define MESH_MESSAGE_LIGHT_EL 0xEA3601
#define MESH_MESSAGE_LIGHT_EL_TEST 0xE93601

/*
 *  Structure definition
 */
struct Mesh_MeshMessageRequest1Cmd
{
    uint8_t  instance_index;
    uint8_t  instance_subindex;
    uint32_t mesh_cmd;
    uint8_t  mesh_cmd_size;
};

// Initialize Mesh module
void Mesh_Init(void);

// Check if Mesh module is initialized
bool Mesh_IsInitialized(void);

/*  Search for model ID in a message
 *
 *  @param p_payload            Pointer to payload
 *  @param len                  Payload len
 *  @param expected_model_id    Expected model ID
 *  @return                     True if found, false otherwise
 */
bool Mesh_IsModelAvailable(uint8_t *p_payload, uint8_t len, uint16_t expected_model_id);

/*  Send Generic OnOff Set Unacknowledged message with repeats.
 *
 *  @param instance_idx        Instance index.
 *  @param value               Generic OnOff target value.
 *  @param transition_time     Transition time (mesh format).
 *  @param delay_ms            Delay in miliseconds.
 *  @param num_of_repeats      Number of message repeats.
 *  @param tid                 Message TID.
 */
void Mesh_SendGenericOnOffSet(uint8_t instance_idx, bool value, uint32_t transition_time, uint32_t delay_ms, uint8_t num_of_repeats, uint8_t tid);

/*  Send Generic OnOff Set Unacknowledged message with repeats.
 *
 *  @param instance_idx          Instance index.
 *  @param value                 Generic OnOff target value.
 *  @param transition_time       Transition time (mesh format).
 *  @param delay_ms              Delay in miliseconds.
 *  @param num_of_repeats        Number of message repeats.
*   @param repeats_interval_ms   time between repeats in milliseconds
 *  @param tid                   Message TID
 */
void Mesh_SendGenericOnOffSetWithRepeatsInterval(uint8_t  instance_idx,
                                                 bool     value,
                                                 uint32_t transition_time,
                                                 uint32_t delay_ms,
                                                 uint8_t  num_of_repeats,
                                                 uint16_t repeats_interval_ms,
                                                 uint8_t  tid);

/*  Send Generic Level Set Unacknowledged message with repeats.
 *
 *  @param instance_idx        Instance index.
 *  @param value               Generic Level target value.
 *  @param transition_time     Transition time (mesh format).
 *  @param delay_ms            Delay in miliseconds.
 *  @param num_of_repeats      Number of message repeats.
 *  @param tid                 Message TID.
 */
void Mesh_SendGenericLevelSet(uint8_t instance_idx, uint16_t value, uint32_t transition_time, uint32_t delay_ms, uint8_t num_of_repeats, uint8_t tid);

/*  Send Generic Delta Set Unacknowledged message with repeats.
 *
 *  @param instance_idx        Instance index.
 *  @param value               Delta change of the value.
 *  @param transition_time     Transition time (mesh format).
 *  @param delay_ms            Delay in miliseconds.
 *  @param num_of_repeats      Number of message repeats.
 *  @param tid                 Message TID.
 */
void Mesh_SendGenericDeltaSet(uint8_t instance_idx, int32_t value, uint32_t transition_time, uint32_t delay_ms, uint8_t num_of_repeats, uint8_t tid);


/*  Send Generic Delta Set Unacknowledged message with repeats.
 *
 *  @param instance_idx          Instance index.
 *  @param value                 Delta change of the value.
 *  @param transition_time       Transition time (mesh format).
 *  @param delay_ms              Delay in miliseconds.
 *  @param num_of_repeats        Number of message repeats.
*   @param repeats_interval_ms   time between repeats in milliseconds
 *  @param tid                   Message TID.
 */
void Mesh_SendGenericDeltaSetWithRepeatsInterval(uint8_t  instance_idx,
                                                 int32_t  value,
                                                 uint32_t transition_time,
                                                 uint32_t delay_ms,
                                                 uint8_t  num_of_repeats,
                                                 uint16_t repeats_interval_ms,
                                                 uint8_t  tid);


/*  Send Generic Delta Set Unacknowledged message with repeats.
 *
 *  @param instance_idx          Instance index.
 *  @param value                 Delta change of the value.
 *  @param transition_time       Transition time (mesh format).
 *  @param delay_ms              mesh delay in miliseconds.
 *  @param dispatch_time_ms      time between call this function and send this command through UART
 *  @param tid                   Message TID
 */
void Mesh_SendGenericDeltaSetWithDispatchTime(uint8_t  instance_idx,
                                              int32_t  value,
                                              uint32_t transition_time,
                                              uint32_t delay_ms,
                                              uint16_t dispatch_time_ms,
                                              uint8_t  tid);

void Mesh_IncrementTid(uint8_t *p_tid);

#endif
