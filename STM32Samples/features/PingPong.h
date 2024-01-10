#ifndef PING_PONG_H
#define PING_PONG_H

#include <stdbool.h>

void PingPong_Init(void);

bool PingPong_IsInitialized(void);

void PingPong_Enable(bool is_enable);

#endif
