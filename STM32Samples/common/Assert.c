#include "Assert.h"

#include <stdbool.h>

#include "Atomic.h"
#include "Log.h"

void Assert_Callback(uint32_t pc)
{
    // Enter critical section and never exit
    Atomic_CriticalEnter();

    while (true)
    {
        LOG_FLUSH();

        // Use a command line to find the function name. Example:
        // addr2line -e OUTPUT_FILE.ELF -j .text PC_ADDRESS
        // addr2line -e output_file.elf -j .text 0x12345678
        LOG_E("ASSERT ERROR, pc@0x%08lX", pc);

        // Delay ~2s to avoid to many messages on the console
        // Delay cannot use SysTick delay because Assert can be called from critical section or HardFault
        volatile size_t i;
        for (i = 0; i < 10000000; i++)
        {
            // Do nothing
        }
    }
}
