#include "AdcHal.h"

#include "Assert.h"
#include "Log.h"
#include "Platform.h"
#include "PriorityConfig.h"
#include "SystemHal.h"
#include "Timestamp.h"

#define ADC_NUMBER_OF_SCAN_CHANNELS 5

// Single channel frequency sampling is a ADC_HAL_SAMPLING_FREQUENCY_HZ / ADC_NUMBER_OF_SCAN_CHANNELS
#define ADC_HAL_SAMPLING_FREQUENCY_HZ (200 * ADC_NUMBER_OF_SCAN_CHANNELS)
#define ADC_HAL_TIMER_PRESCALER (SYSTEM_HAL_CLOCK_HZ / ADC_HAL_SAMPLING_FREQUENCY_HZ / UINT16_MAX)
#define ADC_HAL_TIMER_PERIOD (SYSTEM_HAL_CLOCK_HZ / (ADC_HAL_TIMER_PRESCALER + 1) / ADC_HAL_SAMPLING_FREQUENCY_HZ)

#define ADC_HAL_ENABLE_DELAY_MS 2
#define ADC_HAL_CALIBRATION_TIMEOUT_MS 2
#define ADC_HAL_INITIAL_MEASUREMENT_TIME_MS 2

// Device datasheet data 5.3.19 Temperature sensor characteristics
#define TEMPSENSOR_TYP_AVGSLOPE_UV 4300
#define TEMPSENSOR_TYP_CALX_MV 1430
#define TEMPSENSOR_CALX_TEMP_C 25
#define TEMPSENSOR_100MC_SCALE_FACTOR 100

#define VREFINT_VOLTAGE_MV 1200

static bool              IsInitialized = false;
static volatile uint16_t AdcResults[ADC_NUMBER_OF_SCAN_CHANNELS];

static void AdcHal_InitGpio(void);
static void AdcHal_InitDma1Ch1(void);
static void AdcHal_InitAdc1(void);
static void AdcHal_InitTim4(void);
static void AdcHal_WaitForInitializationDone(void);

void AdcHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("AdcHal initialization");

    AdcHal_InitGpio();
    AdcHal_InitDma1Ch1();
    AdcHal_InitTim4();
    AdcHal_InitAdc1();
    AdcHal_WaitForInitializationDone();

    IsInitialized = true;
}

bool AdcHal_IsInitialized(void)
{
    return IsInitialized;
}

uint16_t AdcHal_ReadChannel(enum AdcHalChannel adc_channel)
{
    ASSERT(adc_channel < ADC_HAL_CHANNEL_LENGTH_MARKER);

    return AdcResults[adc_channel];
}

uint16_t AdcHal_ReadChannelMv(enum AdcHalChannel adc_channel, uint16_t gain_coefficient)
{
    ASSERT(adc_channel < ADC_HAL_CHANNEL_LENGTH_MARKER);

    if (adc_channel == ADC_HAL_CHANNEL_UP_VCC)
    {
        // Avoid dividing by zero
        return (VREFINT_VOLTAGE_MV * ADC_HAL_ADC_MAX) / (AdcResults[ADC_HAL_CHANNEL_UP_VCC] + 1);
    }

    return (AdcResults[adc_channel] * ADC_HAL_REF_V_MV * gain_coefficient) / ADC_HAL_ADC_MAX;
}

int32_t AdcHal_ReadTemperature100mc(void)
{
    return __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(TEMPSENSOR_TYP_AVGSLOPE_UV,
                                                TEMPSENSOR_TYP_CALX_MV * TEMPSENSOR_100MC_SCALE_FACTOR,
                                                TEMPSENSOR_CALX_TEMP_C * TEMPSENSOR_100MC_SCALE_FACTOR,
                                                ADC_HAL_REF_V_MV * TEMPSENSOR_100MC_SCALE_FACTOR,
                                                AdcResults[ADC_HAL_CHANNEL_TEMPERATURE_SENSOR],
                                                LL_ADC_RESOLUTION_12B);
}

static void AdcHal_InitGpio(void)
{
    // GPIO clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    // GPIO initialization
    // PA4 -> ADC1_IN4
    // PA6 -> ADC1_IN6
    // PA7 -> ADC1_IN7
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin                 = LL_GPIO_PIN_4 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
    gpio_init.Mode                = LL_GPIO_MODE_ANALOG;
    gpio_init.Speed               = LL_GPIO_SPEED_FREQ_LOW;
    LL_GPIO_Init(GPIOA, &gpio_init);
}

static void AdcHal_InitDma1Ch1(void)
{
    // DMA1 clock enable
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    // DMA1 CH1 configuration
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_HALFWORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_HALFWORD);

    LL_DMA_ConfigAddresses(DMA1,
                           LL_DMA_CHANNEL_1,
                           LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
                           (uint32_t)&AdcResults,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, ADC_NUMBER_OF_SCAN_CHANNELS);

    LL_DMA_ClearFlag_TC1(DMA1);

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
}

static void AdcHal_InitAdc1(void)
{
    // ADC1 clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);

    LL_ADC_InitTypeDef init_struct = {0};
    init_struct.DataAlignment      = LL_ADC_DATA_ALIGN_RIGHT;
    init_struct.SequencersScanMode = LL_ADC_SEQ_SCAN_ENABLE;
    LL_ADC_Init(ADC1, &init_struct);

    LL_ADC_CommonInitTypeDef common_init_struct = {0};
    common_init_struct.Multimode                = LL_ADC_MULTI_INDEPENDENT;
    LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &common_init_struct);

    LL_ADC_REG_InitTypeDef reg_init_struct = {0};
    reg_init_struct.TriggerSource          = LL_ADC_REG_TRIG_EXT_TIM4_CH4;
    reg_init_struct.SequencerLength        = LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS;
    reg_init_struct.SequencerDiscont       = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    reg_init_struct.ContinuousMode         = LL_ADC_REG_CONV_SINGLE;
    reg_init_struct.DMATransfer            = LL_ADC_REG_DMA_TRANSFER_UNLIMITED;
    LL_ADC_REG_Init(ADC1, &reg_init_struct);

    // Configure regular channels
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_4);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_4, LL_ADC_SAMPLINGTIME_71CYCLES_5);

    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_6);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_5, LL_ADC_SAMPLINGTIME_71CYCLES_5);

    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_3, LL_ADC_CHANNEL_7);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_6, LL_ADC_SAMPLINGTIME_71CYCLES_5);

    // Temperature sensor channel
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_4, LL_ADC_CHANNEL_TEMPSENSOR);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR, LL_ADC_SAMPLINGTIME_71CYCLES_5);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_TEMPSENSOR);

    // Internal reference channel
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_5, LL_ADC_CHANNEL_VREFINT);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_71CYCLES_5);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_VREFINT);

    LL_ADC_Enable(ADC1);

    // Delay between ADC enable and ADC start of calibration
    Timestamp_DelayMs(ADC_HAL_ENABLE_DELAY_MS);

    // Run ADC self calibration
    LL_ADC_StartCalibration(ADC1);

    // Poll for ADC effectively calibrated
    uint32_t calib_timestamp = Timestamp_GetCurrent();
    while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
    {
        if (Timestamp_GetTimeElapsed(calib_timestamp, Timestamp_GetCurrent()) > ADC_HAL_CALIBRATION_TIMEOUT_MS)
        {
            // ADC calibration timeout - critical failure
            ASSERT(false);
        }
    }

    LL_ADC_REG_StartConversionExtTrig(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
}

static void AdcHal_InitTim4(void)
{
    // TIM4 clock enable
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

    LL_TIM_InitTypeDef init_struct = {0};
    init_struct.Prescaler          = ADC_HAL_TIMER_PRESCALER;
    init_struct.CounterMode        = LL_TIM_COUNTERMODE_UP;
    init_struct.Autoreload         = ADC_HAL_TIMER_PERIOD;
    init_struct.ClockDivision      = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM4, &init_struct);

    LL_TIM_OC_InitTypeDef oc_init_struct = {0};
    oc_init_struct.OCMode                = LL_TIM_OCMODE_PWM1;
    oc_init_struct.OCState               = LL_TIM_OCSTATE_ENABLE;
    oc_init_struct.OCNState              = LL_TIM_OCSTATE_DISABLE;
    oc_init_struct.CompareValue          = 1;
    oc_init_struct.OCPolarity            = LL_TIM_OCPOLARITY_HIGH;
    oc_init_struct.OCNPolarity           = LL_TIM_OCPOLARITY_HIGH;
    oc_init_struct.OCIdleState           = LL_TIM_OCIDLESTATE_LOW;
    oc_init_struct.OCNIdleState          = LL_TIM_OCIDLESTATE_LOW;
    LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH4, &oc_init_struct);

    LL_TIM_SetTriggerOutput(TIM4, LL_TIM_TRGO_OC4REF);
    LL_TIM_EnableCounter(TIM4);
}

static void AdcHal_WaitForInitializationDone(void)
{
    while (LL_DMA_IsActiveFlag_TC1(DMA1) == 0)
    {
        // Wait for DMA transfer complete
    }
}
