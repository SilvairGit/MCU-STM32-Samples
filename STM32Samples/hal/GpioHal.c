#include "GpioHal.h"

#include <stddef.h>

#include "Assert.h"
#include "Log.h"
#include "PriorityConfig.h"

struct GpioHalLutRow
{
    GPIO_TypeDef *p_gpio_port;
    uint32_t      gpio_pin;
    bool          irq_support;
    uint32_t      exti_port;
    uint32_t      exit_line;
    uint32_t      exti_af_line;
    void (*irq_cb)(void);
};

static bool IsInitialized = false;

static struct GpioHalLutRow PinLookUpTable[] = {
    {GPIOB, LL_GPIO_PIN_1, false, 0, 0, 0, NULL},                                                           // PIN_LED_STATUS, 		13, PB1
    {GPIOB, LL_GPIO_PIN_0, true, LL_GPIO_AF_EXTI_PORTB, LL_EXTI_LINE_0, LL_GPIO_AF_EXTI_LINE0, NULL},       // PIN_ENCODER_SW,		14, PB0
    {GPIOA, LL_GPIO_PIN_1, true, LL_GPIO_AF_EXTI_PORTA, LL_EXTI_LINE_1, LL_GPIO_AF_EXTI_LINE1, NULL},       // PIN_RTC_INT1,		11, PA1
    {GPIOA, LL_GPIO_PIN_5, true, LL_GPIO_AF_EXTI_PORTA, LL_EXTI_LINE_5, LL_GPIO_AF_EXTI_LINE5, NULL},       // PIN_PIR,				5,  PA5
    {GPIOB, LL_GPIO_PIN_14, true, LL_GPIO_AF_EXTI_PORTB, LL_EXTI_LINE_14, LL_GPIO_AF_EXTI_LINE14, NULL},    // PIN_SW_1,			22, PB14
    {GPIOB, LL_GPIO_PIN_13, true, LL_GPIO_AF_EXTI_PORTB, LL_EXTI_LINE_13, LL_GPIO_AF_EXTI_LINE13, NULL},    // PIN_SW_2,			21, PB13
    {GPIOB, LL_GPIO_PIN_12, true, LL_GPIO_AF_EXTI_PORTB, LL_EXTI_LINE_12, LL_GPIO_AF_EXTI_LINE12, NULL},    // PIN_SW_3,			20, PB12
    {GPIOB, LL_GPIO_PIN_2, true, LL_GPIO_AF_EXTI_PORTB, LL_EXTI_LINE_2, LL_GPIO_AF_EXTI_LINE2, NULL},       // PIN_SW_4,			15, PB2
    {GPIOA, LL_GPIO_PIN_10, false, 0, 0, 0, NULL},                                                          // GPIO6, 		        X,  PA10
    {GPIOA, LL_GPIO_PIN_15, false, 0, 0, 0, NULL},                                                          // GPIO7, 		        X,  PA15
    {GPIOA, LL_GPIO_PIN_12, false, 0, 0, 0, NULL},                                                          // GPIO8, 		        X,  PA12
};

static uint8_t PinIrqEdgeLookUpTable[] = {
    LL_EXTI_TRIGGER_RISING_FALLING,
    LL_EXTI_TRIGGER_FALLING,
    LL_EXTI_TRIGGER_RISING,
};

void GpioHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("GpioHal initialization");

    // Clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // EXTI interrupt initialization
    NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), EXTI_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(EXTI0_IRQn);

    NVIC_SetPriority(EXTI1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), EXTI_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(EXTI1_IRQn);

    NVIC_SetPriority(EXTI2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), EXTI_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(EXTI2_IRQn);

    NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), EXTI_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), EXTI_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    IsInitialized = true;
}

bool GpioHal_IsInitialized(void)
{
    return IsInitialized;
}

void GpioHal_PinMode(enum GpioHalPin gpio, enum GpioHalGpioMode mode)
{
    ASSERT((gpio < GPIO_HAL_PIN_LENGTH_MARKER) && (mode < GPIO_HAL_MODE_LENGTH_MARKER));

    LL_GPIO_SetPinSpeed(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin, LL_GPIO_SPEED_FREQ_LOW);

    switch (mode)
    {
        case GPIO_HAL_MODE_INPUT:
            LL_GPIO_SetPinMode(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin, LL_GPIO_MODE_INPUT);
            break;

        case GPIO_HAL_MODE_OUTPUT:
            LL_GPIO_SetPinMode(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin, LL_GPIO_MODE_OUTPUT);
            break;

        case GPIO_HAL_MODE_INPUT_PULLUP:
            LL_GPIO_SetPinPull(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin, LL_GPIO_PULL_UP);
            LL_GPIO_SetPinMode(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin, LL_GPIO_MODE_INPUT);
            break;

        default:
            break;
    }
}

void GpioHal_SetPinIrq(enum GpioHalPin gpio, enum GpioHalGpioIrqEdge edge, void (*irq_cb)(void))
{
    ASSERT((gpio < GPIO_HAL_PIN_LENGTH_MARKER) && (edge < GPIO_HAL_IRQ_EDGE_LENGTH_MARKER));

    if (PinLookUpTable[gpio].irq_support == false)
    {
        return;
    }

    PinLookUpTable[gpio].irq_cb = irq_cb;

    LL_GPIO_AF_SetEXTISource(PinLookUpTable[gpio].exti_port, PinLookUpTable[gpio].exti_af_line);

    LL_EXTI_InitTypeDef exti_init = {0};
    exti_init.Line_0_31           = PinLookUpTable[gpio].exit_line;
    exti_init.LineCommand         = ENABLE;
    exti_init.Mode                = LL_EXTI_MODE_IT;
    exti_init.Trigger             = PinIrqEdgeLookUpTable[edge];
    LL_EXTI_Init(&exti_init);
}

bool GpioHal_PinRead(enum GpioHalPin gpio)
{
    ASSERT(gpio < GPIO_HAL_PIN_LENGTH_MARKER);

    return LL_GPIO_IsInputPinSet(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin);
}

void GpioHal_PinSet(enum GpioHalPin gpio, bool high)
{
    ASSERT(gpio < GPIO_HAL_PIN_LENGTH_MARKER);

    if (high)
    {
        LL_GPIO_SetOutputPin(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin);
    }
}

void GpioHal_PinToggle(enum GpioHalPin gpio)
{
    ASSERT(gpio < GPIO_HAL_PIN_LENGTH_MARKER);

    LL_GPIO_TogglePin(PinLookUpTable[gpio].p_gpio_port, PinLookUpTable[gpio].gpio_pin);
}

void EXTI0_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_ENCODER_SW].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_ENCODER_SW].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_ENCODER_SW].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_ENCODER_SW].irq_cb();
        }
    }
}

void EXTI1_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_RTC_INT1].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_RTC_INT1].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_RTC_INT1].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_RTC_INT1].irq_cb();
        }
    }
}

void EXTI2_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW4].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW4].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_SW4].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_SW4].irq_cb();
        }
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_PIR].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_PIR].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_PIR].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_PIR].irq_cb();
        }
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW1].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW1].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_SW1].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_SW1].irq_cb();
        }
    }

    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW2].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW2].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_SW2].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_SW2].irq_cb();
        }
    }

    if (LL_EXTI_IsActiveFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW3].exit_line) != RESET)
    {
        LL_EXTI_ClearFlag_0_31(PinLookUpTable[GPIO_HAL_PIN_SW3].exit_line);
        if (PinLookUpTable[GPIO_HAL_PIN_SW3].irq_cb != NULL)
        {
            PinLookUpTable[GPIO_HAL_PIN_SW3].irq_cb();
        }
    }
}
