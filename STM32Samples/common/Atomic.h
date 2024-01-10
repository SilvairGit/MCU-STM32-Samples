#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

void Atomic_CriticalEnter(void);

void Atomic_CriticalExit(void);

uint32_t Atomic_CriticalGetNestingMax(void);

#endif
