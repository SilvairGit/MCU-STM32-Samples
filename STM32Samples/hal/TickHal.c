#include "TickHal.h"

#include "Assert.h"
#include "Platform.h"
#include "PriorityConfig.h"
#include "SystemHal.h"

static bool              IsInitialized = false;
static volatile uint32_t TickCounter   = 0;

void SysTick_Handler(void)
{
    TickCounter++;
}

void TickHal_Init(void)
{
    ASSERT(!IsInitialized);

    // SysTick_IRQn interrupt configuration
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), SYSTICK_IRQ_PRIORITY, 0));

    LL_Init1msTick(SystemHal_GetCoreClock());
    LL_SYSTICK_EnableIT();

    // Enable TRC
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Enable  clock cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Reset the clock cycle counter value
    DWT->CYCCNT = 0;

    IsInitialized = true;
}

bool TickHal_IsInitialized(void)
{
    return IsInitialized;
}

uint32_t TickHal_GetTimestampMs(void)
{
    return TickCounter;
}

uint32_t TickHal_GetClockTick(void)
{
    return DWT->CYCCNT;
}
