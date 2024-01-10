#include "Utils.h"

uint8_t NibbleToHex(uint8_t nibble)
{
    static const uint8_t hex_asc[] = "0123456789ABCDEF";
    nibble &= 0x0F;
    return hex_asc[nibble];
}

uint8_t Bcd2bin(uint8_t val)
{
    return val - 6 * (val >> 4);
}
uint8_t Bin2bcd(uint8_t val)
{
    return val + 6 * (val / 10);
}
