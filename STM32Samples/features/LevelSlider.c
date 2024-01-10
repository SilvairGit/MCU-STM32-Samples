#include "LevelSlider.h"

#include "Config.h"
#include "Utils.h"

#if ENABLE_CLIENT

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#include "AdcHal.h"
#include "Log.h"
#include "Mesh.h"
#include "Timestamp.h"
#include "UartProtocol.h"

#define GENERIC_LEVEL_NUMBER_OF_REPEATS 0    /**< Defines default number of repeats while sending mesh message request */
#define GENERIC_LEVEL_TRANSITION_TIME_MS 100 /**< Defines default transition time */
#define GENERIC_LEVEL_DELAY_TIME_MS 0        /**< Defines default delay time */
#define GENERIC_LEVEL_INTVL_MS 50u           /**< Defines the shortest interval between two LightL messages. */
#define GENERIC_LEVEL_MIN INT16_MIN          /**< Defines lower range of light lightness. */
#define GENERIC_LEVEL_MAX INT16_MAX          /**< Defines upper range of light lightness. */
#define GENERIC_LEVEL_MIN_CHANGE 0x200u      /**< Defines the lowest reported changed. */

#define POT_DEADBAND 205 /**< Potentiometer deadband. About 5% of full range */


static int16_t GenericLevelGet(void);
static bool    HasGenLevelChanged(int gen_level);
static void    PrintGenericLevelTemperature(int gen_level);
static int32_t Map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);

static uint8_t *pInstanceIdx = NULL;
static uint8_t *pLevelTid    = NULL;

void LevelSlider_Setup(uint8_t *p_instance_idx, uint8_t *p_tid)
{
    if (!AdcHal_IsInitialized())
    {
        AdcHal_Init();
    }

    pInstanceIdx = p_instance_idx;
    pLevelTid    = p_tid;
}

void LevelSlider_Loop(void)
{
    static int long last_message_time = UINT32_MAX;

    if (Timestamp_GetTimeElapsed(last_message_time, Timestamp_GetCurrent()) >= GENERIC_LEVEL_INTVL_MS)
    {
        int16_t gen_level = GenericLevelGet();
        if (HasGenLevelChanged(gen_level))
        {
            LOG_D("Temperature changed");
            PrintGenericLevelTemperature(gen_level);
            Mesh_IncrementTid(pLevelTid);
            Mesh_SendGenericLevelSet(*pInstanceIdx,
                                     gen_level,
                                     GENERIC_LEVEL_TRANSITION_TIME_MS,
                                     GENERIC_LEVEL_DELAY_TIME_MS,
                                     GENERIC_LEVEL_NUMBER_OF_REPEATS,
                                     *pLevelTid);

            last_message_time = Timestamp_GetCurrent();
        }
    }
}

static int16_t GenericLevelGet(void)
{
    uint16_t analog_measurement = AdcHal_ReadChannel(ADC_HAL_CHANNEL_POTENTIOMETER);

    if (analog_measurement < ADC_HAL_ADC_MIN + POT_DEADBAND)
    {
        analog_measurement = ADC_HAL_ADC_MIN + POT_DEADBAND;
    }
    if (analog_measurement > ADC_HAL_ADC_MAX - POT_DEADBAND)
    {
        analog_measurement = ADC_HAL_ADC_MAX - POT_DEADBAND;
    }

    return Map(analog_measurement, ADC_HAL_ADC_MIN + POT_DEADBAND, ADC_HAL_ADC_MAX - POT_DEADBAND, GENERIC_LEVEL_MIN, GENERIC_LEVEL_MAX);
}

static bool HasGenLevelChanged(int gen_level)
{
    static int  prev_gen_level          = GENERIC_LEVEL_MIN;
    static bool is_prev_gen_level_valid = false;

    if (!is_prev_gen_level_valid)
    {
        prev_gen_level          = gen_level;
        is_prev_gen_level_valid = true;
        return false;
    }
    else if ((abs(gen_level - prev_gen_level) > GENERIC_LEVEL_MIN_CHANGE) || ((gen_level == 0) && (prev_gen_level != 0)))
    {
        prev_gen_level = gen_level;
        return true;
    }
    else
    {
        return false;
    }
}

static void PrintGenericLevelTemperature(int gen_level)
{
    unsigned percentage = ((gen_level - GENERIC_LEVEL_MIN) * 100) / (GENERIC_LEVEL_MAX - GENERIC_LEVEL_MIN);
    LOG_D("Current Generic Level of Temperature is %d (%d)", gen_level, percentage);
}

static int32_t Map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
#else

void LevelSlider_Setup(uint8_t *p_instance_idx, uint8_t *p_tid)
{
    UNUSED(p_instance_idx);
    UNUSED(p_tid);
}

void LevelSlider_Loop(void)
{
}

#endif
