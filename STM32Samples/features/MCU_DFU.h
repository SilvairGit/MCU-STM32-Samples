#ifndef MCU_DFU_H
#define MCU_DFU_H

#include <stdbool.h>

// Setup DFU
void MCU_DFU_Setup(void);

// Get DFU state
bool MCU_DFU_IsInProgress(void);

#endif
