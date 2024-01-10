#include <stdint.h>

#include "Attention.h"
#include "Config.h"
#include "EmgLTest.h"
#include "LCD.h"
#include "Log.h"
#include "Luminaire.h"
#include "MCU_DFU.h"
#include "MCU_Health.h"
#include "MeshSensorGenerator.h"
#include "PingPong.h"
#include "Provisioning.h"
#include "SensorReceiver.h"
#include "SimpleScheduler.h"
#include "Switch.h"
#include "SystemHal.h"
#include "TimeSource.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Watchdog.h"

int main(void)
{
    SystemHal_Init();
    Timestamp_Init();
    LOG_INIT();
    Watchdog_Init();
    SystemHal_PrintBuildInfo();
    SystemHal_PrintResetCause();
    UartProtocol_Init();

    Mesh_Init();
    PingPong_Init();
    Attention_Init();
    LCD_Setup();
    MCU_Health_Setup();
    MCU_DFU_Setup();
    TimeSource_Init();

    if (ENABLE_LC)
    {
        Luminaire_Init(LUMINAIRE_INIT_MODE_LIGHT_LC);
    }

    if (ENABLE_CTL)
    {
        Luminaire_Init(LUMINAIRE_INIT_MODE_LIGHT_CTL);
    }

    if (ENABLE_PIRALS)
    {
        MeshSensorGenerator_Setup();
    }

    if (ENABLE_EMG_L_TEST)
    {
        EmgLTest_Init();
    }

    if (ENABLE_CLIENT)
    {
        SensorReceiver_Setup();
        Switch_Setup();
    }

    Provisioning_Init();

    UartProtocol_Send(UART_FRAME_CMD_SOFTWARE_RESET_REQUEST, NULL, 0);

    SimpleScheduler_Run();

    return 0;
}
