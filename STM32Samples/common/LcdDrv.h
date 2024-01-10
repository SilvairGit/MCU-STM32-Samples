#ifndef LCD_DRV_H
#define LCD_DRV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void LcdDrv_Init(void);

bool LcdDrv_IsInitialized(void);

void LcdDrv_SetCursor(uint8_t col, uint8_t row);

void LcdDrv_Clear(void);

void LcdDrv_PrintStr(const char *p_str, size_t str_len);

#endif
