#ifndef DELTA_ENCODER_H
#define DELTA_ENCODER_H

#include <stdint.h>

// Setup the delta encoder
void DeltaEncoder_Setup(uint8_t *p_instance_idx, uint8_t *p_tid);

// Loop the delta encoder
void DeltaEncoder_Loop(void);

#endif /* DELTA_ENCODER_H */
