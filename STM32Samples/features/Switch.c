#include "Switch.h"

#include "Config.h"
#include "Utils.h"

#if ENABLE_CLIENT

#include <stdint.h>
#include <stdlib.h>

#include "DeltaEncoder.h"
#include "GpioHal.h"
#include "LCD.h"
#include "LevelSlider.h"
#include "Log.h"
#include "ModelManager.h"
#include "OnOffDeltaButtons.h"
#include "SimpleScheduler.h"
#include "UartProtocol.h"

#define SWITCH_TASK_PERIOD_MS 1


static uint8_t LightLcClientInstanceIdx  = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
static uint8_t LightCtlClientInstanceIdx = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

struct ModelManagerRegistrationRow ModelConfigLightLcClient = {
    .model_id            = MODEL_MANAGER_ID_LIGHT_LC_CLIENT,
    .p_model_parameter   = NULL,
    .model_parameter_len = 0,
    .p_instance_index    = &LightLcClientInstanceIdx,
};

struct ModelManagerRegistrationRow ModelConfigLightCtlClient = {
    .model_id            = MODEL_MANAGER_ID_LIGHT_CTL_CLIENT,
    .p_model_parameter   = NULL,
    .model_parameter_len = 0,
    .p_instance_index    = &LightCtlClientInstanceIdx,
};

static uint8_t LCOnOffTid  = 0;
static uint8_t CTLOnOffTid = 0;
static uint8_t LCLevelTid  = 0;
static uint8_t CTLLevelTid = 0;

static void LoopSwitch(void);

void Switch_Setup(void)
{
    LOG_D("Switch initialization");

    DeltaEncoder_Setup(&LightLcClientInstanceIdx, &LCLevelTid);
    LevelSlider_Setup(&LightCtlClientInstanceIdx, &CTLLevelTid);
    OnOffDeltaButtons_Setup(&LightLcClientInstanceIdx, &LightCtlClientInstanceIdx, &LCOnOffTid, &CTLOnOffTid, &LCLevelTid, &CTLLevelTid);

    ModelManager_RegisterModel(&ModelConfigLightLcClient);
    ModelManager_RegisterModel(&ModelConfigLightCtlClient);

    SimpleScheduler_TaskAdd(SWITCH_TASK_PERIOD_MS, LoopSwitch, SIMPLE_SCHEDULER_TASK_ID_SWITCH, false);
}

static void LoopSwitch(void)
{
    OnOffDeltaButtons_Loop();
    DeltaEncoder_Loop();
    LevelSlider_Loop();
}
#else
void Switch_Setup(void)
{
}
#endif
