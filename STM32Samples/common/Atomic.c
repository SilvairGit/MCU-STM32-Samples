#include "Atomic.h"

#include "AtomicHal.h"

static volatile uint32_t CriticalNestingCnt = 0;
static volatile uint32_t CriticalNestingMax = 0;

void Atomic_CriticalEnter(void)
{
    AtomicHal_IrqDisable();

    CriticalNestingCnt++;
    if (CriticalNestingCnt > CriticalNestingMax)
    {
        CriticalNestingMax = CriticalNestingCnt;
    }
}

void Atomic_CriticalExit(void)
{
    CriticalNestingCnt--;
    if (CriticalNestingCnt == 0)
    {
        AtomicHal_IrqEnable();
    }
}

uint32_t Atomic_CriticalGetNestingMax(void)
{
    return CriticalNestingMax;
}
