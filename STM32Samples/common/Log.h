#ifndef LOG_H
#define LOG_H

#include <stddef.h>
#include <stdio.h>

#include "Timestamp.h"
#include "Utils.h"

#define LOG_ENABLE 1

// Error  - LOG_E - this option is used to print error message
#define LOG_ERROR_ENABLE 1
// Warning- LOG_W - this option is used to print warning message
#define LOG_WARNING_ENABLE 1
// Debug  - LOG_D - this option is used to print message during development and debug
#define LOG_DEBUG_ENABLE 1

#define LOG_UART_ENABLE 1
#define LOG_RTT_ENABLE 0

#if LOG_UART_ENABLE && LOG_RTT_ENABLE
#error "Only one type of logger can be selected"
#endif

// Parameters
#define LOG_NEW_LINE_SYMBOL "\n"
#define LOG_BUFF_BYTES_IN_SINGLE_LINE 16

#if LOG_ENABLE

#if LOG_UART_ENABLE
#include "LoggerHal.h"
#define LOG_INIT() LoggerHal_Init()
#define LOG_PRINT_STRING(p_buff) LoggerHal_SendString(p_buff)
#define LOG_PRINT_STRING_LEN(p_buff, buff_len) LoggerHal_SendBuffer(p_buff, buff_len)
#define LOG_PRINT_CHAR(chr) LoggerHal_SendByte(chr)
#define LOG_FLUSH() LoggerHal_Flush()
#define LOG_PRINTF(...) LOGGER_HAL_PRINTF(__VA_ARGS__)
#endif

#if LOG_RTT_ENABLE
#error "To use RTT add a Segger RTT source code to the project"

#include <SEGGER_RTT.h>
#define LOG_INIT()
#define LOG_PRINT_STRING(p_buff) SEGGER_RTT_WriteString(0, p_buff)
#define LOG_PRINT_STRING_LEN(p_buff, buff_len) SEGGER_RTT_Write(0, p_buff, buff_len)
#define LOG_PRINT_CHAR(chr) SEGGER_RTT_PutChar(0, chr)
#define LOG_FLUSH() \
    do              \
    {               \
    } while (SEGGER_RTT_GetBytesInBuffer(0) != 0)
#define LOG_PRINTF(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#endif

#else

// Disable debug console
#define LOG_INIT()
#define LOG_PRINT_STRING(p_buff)
#define LOG_PRINT_STRING_LEN(p_buff, buff_len)
#define LOG_PRINT_CHAR(chr)
#define LOG_FLUSH()
#define LOG_PRINTF(...)
#endif

static inline void LOG_UNUSED(const char *p_format, ...)
{
    UNUSED(p_format);
}

// Enable debug console
#define LOG_GENERIC(type, ...)                                                 \
    do                                                                         \
    {                                                                          \
        LOG_PRINTF("[%06u][%c] ", (unsigned int)Timestamp_GetCurrent(), type); \
        LOG_PRINTF(__VA_ARGS__);                                               \
        LOG_PRINTF("%s", LOG_NEW_LINE_SYMBOL);                                 \
    } while (0)

#define LOG_HEX_GENERIC(type, info, p_buff, len)                                       \
    do                                                                                 \
    {                                                                                  \
        LOG_PRINTF("[%06u][%c] %s", (unsigned int)Timestamp_GetCurrent(), type, info); \
        LOG_PRINTF("%s", LOG_NEW_LINE_SYMBOL);                                         \
        Log_BuffToHex(p_buff, len);                                                    \
    } while (0)


#if LOG_ENABLE && LOG_ERROR_ENABLE && !defined(CMAKE_UNIT_TEST)
#define LOG_E(...) LOG_GENERIC('E', __VA_ARGS__)
#define LOG_HEX_E(info, p_buff, buff_len) LOG_HEX_GENERIC('E', info, p_buff, buff_len)
#else
#define LOG_E(...) LOG_UNUSED(__VA_ARGS__)
#define LOG_HEX_E(...) LOG_UNUSED(__VA_ARGS__)
#endif

#if LOG_ENABLE && LOG_WARNING_ENABLE && !defined(CMAKE_UNIT_TEST)
#define LOG_W(...) LOG_GENERIC('W', __VA_ARGS__)
#define LOG_HEX_W(info, p_buff, buff_len) LOG_HEX_GENERIC('W', info, p_buff, buff_len)
#else
#define LOG_W(...) LOG_UNUSED(__VA_ARGS__)
#define LOG_HEX_W(...) LOG_UNUSED(__VA_ARGS__)
#endif

#if LOG_ENABLE && LOG_DEBUG_ENABLE && !defined(CMAKE_UNIT_TEST)
#define LOG_D(...) LOG_GENERIC('D', __VA_ARGS__)
#define LOG_HEX_D(info, p_buff, buff_len) LOG_HEX_GENERIC('D', info, p_buff, buff_len)
#else
#define LOG_D(...) LOG_UNUSED(__VA_ARGS__)
#define LOG_HEX_D(...) LOG_UNUSED(__VA_ARGS__)
#endif

void Log_BuffToHex(uint8_t *p_buff, size_t buff_len);

#endif
