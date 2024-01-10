#ifndef ATTENTION_H
#define ATTENTION_H

#include <stdbool.h>

void Attention_Init(void);

bool Attention_IsInitialized(void);

void Attention_StateChange(bool is_enable);

#endif
