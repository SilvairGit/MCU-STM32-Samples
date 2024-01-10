#include <Log.h>
#include <Utils.h>
#include <stdint.h>

int _write(int32_t file, uint8_t *ptr, int32_t len)
{
    LOG_PRINT_STRING_LEN(ptr, len);

    UNUSED(file);
    UNUSED(ptr);

    return len;
}
