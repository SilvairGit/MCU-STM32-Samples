#ifndef ENCODER_HAL_H
#define ENCODER_HAL_H

#include <stdbool.h>
#include <stdint.h>

#define ENCODER_HAL_MAX_VALUE INT16_MAX
#define ENCODER_HAL_MIN_VALUE (-INT16_MAX)

void EncoderHal_Init(void);

bool EncoderHal_IsInitialized(void);

int32_t EncoderHal_GetPosition(void);

void EncoderHal_SetPosition(int32_t position);

#endif
