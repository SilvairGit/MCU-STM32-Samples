#include "EncoderHal.h"

#include "Assert.h"
#include "Log.h"
#include "Platform.h"

#define ENCODER_HAL_OFFSET_VALUE ENCODER_HAL_MAX_VALUE

static bool IsInitialized = false;

static void EncoderHal_InitGpio(void);

static void EncoderHal_InitTim3(void);

void EncoderHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("EncoderHal initialization");

    EncoderHal_InitGpio();
    EncoderHal_InitTim3();

    IsInitialized = true;
}

bool EncoderHal_IsInitialized(void)
{
    return IsInitialized;
}

int32_t EncoderHal_GetPosition(void)
{
    return LL_TIM_GetCounter(TIM3) - ENCODER_HAL_OFFSET_VALUE;
}

void EncoderHal_SetPosition(int32_t position)
{
    ASSERT((position <= ENCODER_HAL_MAX_VALUE) && (position >= ENCODER_HAL_MIN_VALUE));

    LL_TIM_SetCounter(TIM3, ENCODER_HAL_OFFSET_VALUE + position);
}

static void EncoderHal_InitGpio(void)
{
    // GPIO clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // GPIO initialization
    // PB4 -> TIM3_CH1
    // PB5 -> TIM3_CH2
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin                 = LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    gpio_init.Mode                = LL_GPIO_MODE_INPUT;
    gpio_init.Speed               = LL_GPIO_SPEED_FREQ_LOW;
    gpio_init.Pull                = LL_GPIO_PULL_UP;

    LL_GPIO_Init(GPIOB, &gpio_init);

    LL_GPIO_AF_RemapPartial_TIM3();
}

static void EncoderHal_InitTim3(void)
{
    // TIM3 clock enable
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    LL_TIM_InitTypeDef tim_init = {0};
    tim_init.Prescaler          = 0;
    tim_init.CounterMode        = LL_TIM_COUNTERMODE_UP;
    tim_init.Autoreload         = ENCODER_HAL_OFFSET_VALUE * 2;
    tim_init.ClockDivision      = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM3, &tim_init);

    LL_TIM_ENCODER_InitTypeDef encoder_init = {0};
    encoder_init.EncoderMode                = LL_TIM_ENCODERMODE_X2_TI1;
    encoder_init.IC1Polarity                = LL_TIM_IC_POLARITY_FALLING;
    encoder_init.IC1ActiveInput             = LL_TIM_ACTIVEINPUT_DIRECTTI;
    encoder_init.IC1Prescaler               = LL_TIM_ICPSC_DIV1;
    encoder_init.IC1Filter                  = LL_TIM_IC_FILTER_FDIV1;
    encoder_init.IC2Polarity                = LL_TIM_IC_POLARITY_RISING;
    encoder_init.IC2ActiveInput             = LL_TIM_ACTIVEINPUT_DIRECTTI;
    encoder_init.IC2Prescaler               = LL_TIM_ICPSC_DIV1;
    encoder_init.IC2Filter                  = LL_TIM_IC_FILTER_FDIV1;
    LL_TIM_ENCODER_Init(TIM3, &encoder_init);

    LL_TIM_SetCounter(TIM3, ENCODER_HAL_OFFSET_VALUE);

    LL_TIM_EnableCounter(TIM3);
}
