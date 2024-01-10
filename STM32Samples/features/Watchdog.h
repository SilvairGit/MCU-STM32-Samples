#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdbool.h>

void Watchdog_Init(void);

bool Watchdog_IsInitialized(void);

#endif
