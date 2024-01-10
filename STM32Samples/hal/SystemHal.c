#include "SystemHal.h"

#include "Assert.h"
#include "Config.h"
#include "Log.h"
#include "Platform.h"

static bool IsInitialized = false;

static void SystemHal_ClockConfig(void);

void SystemHal_Init(void)
{
    ASSERT(!IsInitialized);

    // Reset of all peripherals
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    // System interrupt priority initialization
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // JTAG-DP Disabled and SW-DP Enabled
    LL_GPIO_AF_Remap_SWJ_NOJTAG();

    SystemHal_ClockConfig();

    IsInitialized = true;
}

bool SystemHal_IsInitialized(void)
{
    return IsInitialized;
}

uint32_t SystemHal_GetCoreClock(void)
{
    return SystemCoreClock;
}

void SystemHal_PrintResetCause(void)
{
    if (LL_RCC_IsActiveFlag_PINRST())
    {
        LOG_D("Reset cause: PIN reset flag");
    }

    if (LL_RCC_IsActiveFlag_PORRST())
    {
        LOG_D("Reset cause: POR/PDR reset flag");
    }

    if (LL_RCC_IsActiveFlag_SFTRST())
    {
        LOG_D("Reset cause: Software Reset flag");
    }

    if (LL_RCC_IsActiveFlag_LPWRRST())
    {
        LOG_W("Reset cause: Low Power Reset flag");
    }

    if (LL_RCC_IsActiveFlag_IWDGRST())
    {
        LOG_E("Reset cause: Independent Watchdog reset flag");
    }

    if (LL_RCC_IsActiveFlag_WWDGRST())
    {
        LOG_E("Reset cause: Window Watchdog reset flag");
    }

    LL_RCC_ClearResetFlags();
}

void SystemHal_PrintBuildInfo(void)
{
    LOG_D("Build date: %s", __DATE__);
    LOG_D("Build time: %s", __TIME__);

    uint8_t *p_fw_version = (uint8_t *)BUILD_NUMBER;
    LOG_D("FW version: %s", p_fw_version);

    LOG_D("STM32 UUID0: 0x%08lX 0x%08lX 0x%08lX", LL_GetUID_Word0(), LL_GetUID_Word1(), LL_GetUID_Word2());
}

static void SystemHal_ClockConfig(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
    {
    }
    LL_RCC_HSE_Enable();

    // Wait till HSE is ready
    while (LL_RCC_HSE_IsReady() != 1)
    {
    }
    LL_RCC_LSI_Enable();

    // Wait till LSI is ready
    while (LL_RCC_LSI_IsReady() != 1)
    {
    }
    LL_PWR_EnableBkUpAccess();
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE)
    {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
    }
    LL_RCC_LSE_Enable();

    // Wait till LSE is ready
    while (LL_RCC_LSE_IsReady() != 1)
    {
    }
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE)
    {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    }
    LL_RCC_EnableRTC();
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_2, LL_RCC_PLL_MUL_9);
    LL_RCC_PLL_Enable();

    // Wait till PLL is ready
    while (LL_RCC_PLL_IsReady() != 1)
    {
    }
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

    // Wait till System clock is ready
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    {
    }

    LL_SetSystemCoreClock(SYSTEM_HAL_CLOCK_HZ);
    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSRC_PCLK2_DIV_6);
}
