#ifndef ASSERT_H
#define ASSERT_H

#include <stdint.h>

#include "Utils.h"

#define ASSERT_ENABLE 1

#if ASSERT_ENABLE

#ifdef CMAKE_UNIT_TEST
void Assert_Callback(uint32_t pc);
#else
NORETURN void Assert_Callback(uint32_t pc);
#endif

#define ASSERT(expr)                     \
    do                                   \
    {                                    \
        if ((expr) == 0)                 \
        {                                \
            Assert_Callback(__get_PC()); \
        }                                \
    } while (0)

#else
#define ASSERT(expr)
#endif

#endif
