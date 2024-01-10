#ifndef FLASH_HAL_H
#define FLASH_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Utils.h"

void FlashHal_Init(void);

bool FlashHal_IsInitialized(void);

size_t FlashHal_GetSpaceSize(void);

uint32_t FlashHal_GetSpaceAddress(void);

bool FlashHal_EraseSpace(void);

bool FlashHal_SaveToFlash(uint32_t address, const uint32_t *p_src, uint32_t num_of_words);

RAM_FUNCTION bool FlashHal_UpdateFirmware(uint32_t num_of_words);

#endif
