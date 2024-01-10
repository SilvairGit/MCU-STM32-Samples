#include "AtomicHal.h"

#include "Platform.h"

void AtomicHal_IrqEnable(void)
{
    __enable_irq();
}

void AtomicHal_IrqDisable(void)
{
    __disable_irq();
}
