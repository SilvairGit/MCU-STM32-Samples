#ifndef UART_HAL_H
#define UART_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void UartHal_Init(void);

bool UartHal_IsInitialized(void);

void UartHal_SendString(uint8_t *p_string);

void UartHal_SendBuffer(uint8_t *p_buff, size_t buff_len);

void UartHal_SendByte(uint8_t byte);

bool UartHal_ReadByte(uint8_t *p_byte);

void UartHal_Flush(void);

#endif
