#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#ifdef CMAKE_UNIT_TEST
#define RAM_FUNCTION
#else
#define RAM_FUNCTION __attribute__((section(".RamFunction")))
#endif

#define NORETURN __attribute__((noreturn))
#define ALWAYS_INLINE __attribute__((always_inline))
#define PACKED __attribute__((packed))
#define ALIGN(x) __attribute__((aligned(x)))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UNUSED(x) (void)(x)

#define LOW_BYTE(w) ((uint8_t)((w)&0xff))
#define HIGH_BYTE(w) ((uint8_t)((w) >> 8))

#define SWAP_2_BYTES(x) ((((x)&0x00FF) << 8) | (((x)&0xFF00) >> 8))
#define SWAP_3_BYTES(x) ((((x)&0x0000FF) << 16) | ((x)&0x00FF00) | (((x)&0xFF0000) >> 16))
#define SWAP_4_BYTES(x) ((((x)&0x000000FF) << 24) | (((x)&0x0000FF00) << 8) | (((x)&0x00FF0000) >> 8) | (((x)&0xFF000000) >> 24))

#define STATIC_ASSERT(cond, msg) typedef char STATIC_ASSERT_MSG(msg)[(cond) ? 1 : -1]
#define STATIC_ASSERT_MSG(msg) STATIC_ASSERT_MSG_(msg)
#define STATIC_ASSERT_MSG_(msg) static_assertion_##msg

ALWAYS_INLINE static inline uint32_t __get_PC(void)
{
#ifdef CMAKE_UNIT_TEST
    return 0x89ABCDEF;
#else
    uint32_t result;

    __asm volatile("MOV %0, PC" : "=r"(result));
    return (result);
#endif
}

uint8_t NibbleToHex(uint8_t nibble);

uint8_t Bcd2bin(uint8_t val);

uint8_t Bin2bcd(uint8_t val);

#endif
