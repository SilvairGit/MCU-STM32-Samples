#include "Log.h"

#include <stddef.h>
#include <stdint.h>

void Log_BuffToHex(uint8_t *p_buff, size_t buff_len)
{
    if ((p_buff == NULL) || (buff_len == 0))
    {
        LOG_PRINTF("\tNone%s", LOG_NEW_LINE_SYMBOL);
        return;
    }

    size_t char_cnt = 0;
    size_t i;
    for (i = 0; i < buff_len; i++)
    {
        if (char_cnt == 0)
        {
            LOG_PRINT_CHAR('\t');
        }

        LOG_PRINT_CHAR(NibbleToHex(p_buff[i] >> 4));
        LOG_PRINT_CHAR(NibbleToHex(p_buff[i]));
        LOG_PRINT_CHAR(' ');

        if (char_cnt == (LOG_BUFF_BYTES_IN_SINGLE_LINE - 1))
        {
            LOG_PRINTF("%s", LOG_NEW_LINE_SYMBOL);
        }

        char_cnt++;
        if (char_cnt == LOG_BUFF_BYTES_IN_SINGLE_LINE)
        {
            char_cnt = 0;
        }
    }

    if (char_cnt != 0)
    {
        LOG_PRINTF("%s", LOG_NEW_LINE_SYMBOL);
    }
}
