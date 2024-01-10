#ifndef LUMINAIRE_H
#define LUMINAIRE_H

#include <stdbool.h>
#include <stdint.h>

enum LuminaireInitMode
{
    LUMINAIRE_INIT_MODE_LIGHT_LC,
    LUMINAIRE_INIT_MODE_LIGHT_CTL
};

void Luminaire_Init(enum LuminaireInitMode init_mode);

bool Luminaire_IsInitialized(void);

void Luminaire_IndicateAttention(bool is_attention_enable, bool led_state);

void Luminaire_StartStartupBehavior(void);

void Luminaire_StopStartupBehavior(void);

uint16_t Luminaire_GetDutyCycle(void);

bool Luminaire_IsStartupBehaviorInProgress(void);

#endif
