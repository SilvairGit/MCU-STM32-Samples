#include "stm32f1xx_it.h"

#include <stdbool.h>

#include "Assert.h"
#include "stm32f1xx_conf.h"

void NMI_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void HardFault_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void MemManage_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void BusFault_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void UsageFault_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void SVC_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void DebugMon_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void PendSV_Handler(void)
{
    // Add logger error message
    ASSERT(false);
}

void RCC_IRQHandler(void)
{
}
