#include "PwmHal.h"

#include "Assert.h"
#include "Log.h"
#include "Platform.h"

#define PWM_HAL_1V_OFFSET_MV 1000
#define PWM_HAL_MAX_VALUE_MV 10000

static bool IsInitialized = false;

static void     PwmHal_InitGpio(void);
static void     PwmHal_InitTimer1Timer2(void);
static uint16_t PwmHal_ConvertDutyCycleTo_1_10V(uint16_t duty_cycle);

void PwmHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("PwmHal initialization");

    PwmHal_InitGpio();
    PwmHal_InitTimer1Timer2();

    IsInitialized = true;
}

bool PwmHal_IsInitialized(void)
{
    return IsInitialized;
}

void PwmHal_SetPwmDuty(enum PwmHalChannel channel, uint16_t duty_cycle)
{
    ASSERT((channel < PWM_HAL_CHANNEL_LENGTH_MARKER) && (duty_cycle <= PWM_HAL_OUTPUT_DUTY_CYCLE_MAX));

    switch (channel)
    {
        case PWM_HAL_CHANNEL_WARM:
            LL_TIM_OC_SetCompareCH1(TIM1, duty_cycle);
            break;

        case PWM_HAL_CHANNEL_COLD:
            LL_TIM_OC_SetCompareCH2(TIM1, duty_cycle);
            break;

        case PWM_HAL_CHANNEL_WARM_1_10V:
            LL_TIM_OC_SetCompareCH1(TIM2, PwmHal_ConvertDutyCycleTo_1_10V(duty_cycle));
            break;

        case PWM_HAL_CHANNEL_COLD_1_10V:
            LL_TIM_OC_SetCompareCH3(TIM1, PwmHal_ConvertDutyCycleTo_1_10V(duty_cycle));
            break;

        default:
            break;
    }
}

static void PwmHal_InitGpio(void)
{
    // GPIO clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // GPIO initialization
    // PA8  -> TIM1_CH1
    // PA9  -> TIM1_CH2
    // PA0  -> TIM2_CH1
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin                 = LL_GPIO_PIN_0 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    gpio_init.Mode                = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed               = LL_GPIO_SPEED_FREQ_LOW;
    gpio_init.OutputType          = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &gpio_init);

    // PB15 -> TIM1_CH3N
    gpio_init.Pin = LL_GPIO_PIN_15;
    LL_GPIO_Init(GPIOB, &gpio_init);
}

static void PwmHal_InitTimer1Timer2(void)
{
    // Timer clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

    // Timer 1 & 2 initialization
    LL_TIM_InitTypeDef tim_init = {0};
    tim_init.Prescaler          = PWM_HAL_TIMER_PRESCALER;
    tim_init.CounterMode        = LL_TIM_COUNTERMODE_UP;
    tim_init.Autoreload         = PWM_HAL_TIMER_PERIOD;
    tim_init.ClockDivision      = LL_TIM_CLOCKDIVISION_DIV1;
    tim_init.RepetitionCounter  = 0;

    LL_TIM_Init(TIM1, &tim_init);
    LL_TIM_Init(TIM2, &tim_init);

    LL_TIM_OC_InitTypeDef oc_init_struct = {0};
    oc_init_struct.OCMode                = LL_TIM_OCMODE_PWM1;
    oc_init_struct.OCState               = LL_TIM_OCSTATE_ENABLE;
    oc_init_struct.OCNState              = LL_TIM_OCSTATE_DISABLE;
    oc_init_struct.CompareValue          = 0;
    oc_init_struct.OCPolarity            = LL_TIM_OCPOLARITY_HIGH;
    oc_init_struct.OCNPolarity           = LL_TIM_OCPOLARITY_HIGH;
    oc_init_struct.OCIdleState           = LL_TIM_OCIDLESTATE_LOW;
    oc_init_struct.OCNIdleState          = LL_TIM_OCIDLESTATE_LOW;

    LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &oc_init_struct);
    LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH2, &oc_init_struct);
    LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH1, &oc_init_struct);

    oc_init_struct.OCState  = LL_TIM_OCSTATE_DISABLE;
    oc_init_struct.OCNState = LL_TIM_OCSTATE_ENABLE;
    LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH3, &oc_init_struct);

    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1);
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH2);
    LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH3N);
    LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH1);

    LL_TIM_OC_SetCompareCH1(TIM1, 0);
    LL_TIM_OC_SetCompareCH2(TIM1, 0);
    LL_TIM_OC_SetCompareCH3(TIM1, 0);
    LL_TIM_OC_SetCompareCH1(TIM2, 0);

    LL_TIM_EnableCounter(TIM1);
    LL_TIM_EnableCounter(TIM2);

    LL_TIM_EnableAllOutputs(TIM1);
    LL_TIM_EnableAllOutputs(TIM2);
}

static uint16_t PwmHal_ConvertDutyCycleTo_1_10V(uint16_t duty_cycle)
{
    if (duty_cycle == 0)
    {
        return 0;
    }

    // Calculate offset
    uint16_t offset_duty_cycle = (PWM_HAL_1V_OFFSET_MV * PWM_HAL_OUTPUT_DUTY_CYCLE_MAX) / PWM_HAL_MAX_VALUE_MV;

    // Scale to range
    offset_duty_cycle += (duty_cycle * (PWM_HAL_MAX_VALUE_MV - PWM_HAL_1V_OFFSET_MV)) / PWM_HAL_MAX_VALUE_MV;

    return offset_duty_cycle;
}
