#ifndef LOGGER_HAL_H
#define LOGGER_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define LOGGER_HAL_PRINTF(...) printf(__VA_ARGS__)

void LoggerHal_Init(void);

bool LoggerHal_IsInitialized(void);

void LoggerHal_SendString(uint8_t *p_string);

void LoggerHal_SendBuffer(uint8_t *p_buff, size_t buff_len);

void LoggerHal_SendByte(uint8_t byte);

bool LoggerHal_ReadByte(uint8_t *p_byte);

void LoggerHal_Flush(void);

#endif
