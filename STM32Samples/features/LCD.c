#include "LCD.h"

#include <stdint.h>

#include "Config.h"
#include "Utils.h"

#if ENABLE_CLIENT

#include <stdlib.h>
#include <string.h>

#include "Config.h"
#include "LcdDrv.h"
#include "Log.h"
#include "SensorReceiver.h"
#include "SimpleScheduler.h"
#include "TAILocalTimeConverter.h"
#include "TimeReceiver.h"
#include "Timestamp.h"
#include "UartProtocol.h"

#define LCD_TASK_PERIOD_MS 10

#define LCD_SCREEN_SWITCH_INTV_MS 5000 /**< Defines LCD screen switch interval. */
#define LCD_ROWS_NUMBER 4              /**< Defines number of LCD rows. */
#define LCD_COLUMNS_NUMBER 20          /**< Defines number of LCD columns. */

#define LCD_PIR_VALUE_EXP_MS 60000     /**< PIR Sensor value expiration time in milliseconds. */
#define LCD_ALS_VALUE_EXP_MS 60000     /**< ALS Sensor value expiration time in milliseconds. */
#define LCD_POWER_VALUE_EXP_MS 60000   /**< Power Sensor value expiration time in milliseconds. */
#define LCD_CURRENT_VALUE_EXP_MS 60000 /**< Current Sensor value expiration time in milliseconds. */
#define LCD_VOLTAGE_VALUE_EXP_MS 60000 /**< Voltage Sensor value expiration time in milliseconds. */
#define LCD_ENERGY_VALUE_EXP_MS 60000  /**< Energy Sensor value expiration time in milliseconds. */

#define LCD_DATE_AND_TIME_UPDATE_PERIOD_MS 1000 /**< Date and Time displayed time update in milliseconds. */

enum ScreenType
{
    SCREEN_TYPE_FIRST,
    SCREEN_TYPE_MODEM_STATE_PIR_ALS = SCREEN_TYPE_FIRST,
    SCREEN_TYPE_ENERGY_SENSORS,
    SCREEN_TYPE_FW_VERSION,
    SCREEN_TYPE_DFU,
    SCREEN_TYPE_DATE_AND_TIME,
    SCREEN_TYPE_LAST = SCREEN_TYPE_DATE_AND_TIME,
};

static bool            LCD_DfuInProgress                          = false;
static enum ModemState LCD_ModemState                             = MODEM_STATE_UNKNOWN;
static char            LCD_ModemFwVersion[LCD_COLUMNS_NUMBER + 1] = "Unknown";
static enum ScreenType LCD_CurrentScreen                          = SCREEN_TYPE_FIRST;
static unsigned long   LCD_CurrentScreenTimestamp                 = 0;
static bool            LCD_NeedsUpdate                            = false;

enum LCD_SensorValueState
{
    SENSOR_VALUE_UNKNOWN,
    SENSOR_VALUE_ACTUAL,
    SENSOR_VALUE_EXPIRED,
};

struct LCD_Sensor
{
    enum SensorProperty       property;
    union SensorValue         value;
    unsigned long             value_timestamp;
    unsigned long             value_expiration_time;
    enum LCD_SensorValueState value_state;
};

static struct LCD_Sensor LCD_PirSensor = {
    .property = PRESENCE_DETECTED,
    .value =
        {
            .pir = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_PIR_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static struct LCD_Sensor LCD_AlsSensor = {
    .property = PRESENT_AMBIENT_LIGHT_LEVEL,
    .value =
        {
            .als = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_ALS_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static struct LCD_Sensor LCD_PowerSensor = {
    .property = PRESENT_DEVICE_INPUT_POWER,
    .value =
        {
            .power = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_POWER_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static struct LCD_Sensor LCD_CurrentSensor = {
    .property = PRESENT_INPUT_CURRENT,
    .value =
        {
            .current = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_CURRENT_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static struct LCD_Sensor LCD_VoltageSensor = {
    .property = PRESENT_INPUT_VOLTAGE,
    .value =
        {
            .voltage = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_VOLTAGE_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static struct LCD_Sensor LCD_EnergySensor = {
    .property = TOTAL_DEVICE_ENERGY_USE,
    .value =
        {
            .energy = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_ENERGY_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static struct LCD_Sensor LCD_PreciseEnergySensor = {
    .property = PRECISE_TOTAL_DEVICE_ENERGY_USE,
    .value =
        {
            .precise_energy = 0,
        },
    .value_timestamp       = 0,
    .value_expiration_time = LCD_ENERGY_VALUE_EXP_MS,
    .value_state           = SENSOR_VALUE_UNKNOWN,
};

static void LCD_Loop(void);
static void DisplayLine(size_t line, const char *text);
static void DisplayScreen(uint8_t screenNum);
static void DisplayModemState(uint8_t lineNumber, enum ModemState modemState);
static void ScreenIterate(void);
static void CheckSensorValuesExpiration(void);
static void CheckTimeDisplayNeedUpdate(void);

void LCD_Setup(void)
{
    if (!LcdDrv_IsInitialized())
    {
        LcdDrv_Init();
    }

    DisplayScreen(LCD_CurrentScreen);

    SimpleScheduler_TaskAdd(LCD_TASK_PERIOD_MS, LCD_Loop, SIMPLE_SCHEDULER_TASK_ID_LCD, true);
}

void LCD_UpdateModemState(enum ModemState modemState)
{
    if (LCD_ModemState != modemState && LCD_CurrentScreen == SCREEN_TYPE_MODEM_STATE_PIR_ALS)
        LCD_NeedsUpdate = true;

    LCD_ModemState = modemState;
}

void LCD_UpdateModemFwVersion(char *fwVersion, uint8_t fwVerLen)
{
    if (fwVerLen > LCD_COLUMNS_NUMBER)
        fwVerLen = LCD_COLUMNS_NUMBER;

    if (strncmp(LCD_ModemFwVersion, fwVersion, fwVerLen) && LCD_CurrentScreen == SCREEN_TYPE_FW_VERSION)
        LCD_NeedsUpdate = true;
    strncpy(LCD_ModemFwVersion, fwVersion, fwVerLen);
    LCD_ModemFwVersion[fwVerLen] = '\0';
}

void LCD_UpdateSensorValue(enum SensorProperty sensorProperty, union SensorValue sensorValue)
{
    switch (sensorProperty)
    {
        case PRESENCE_DETECTED:
        {
            if (LCD_PirSensor.value.pir != sensorValue.pir && LCD_CurrentScreen == SCREEN_TYPE_MODEM_STATE_PIR_ALS)
                LCD_NeedsUpdate = true;

            LCD_PirSensor.value.pir       = sensorValue.pir;
            LCD_PirSensor.value_state     = SENSOR_VALUE_ACTUAL;
            LCD_PirSensor.value_timestamp = Timestamp_GetCurrent();
            break;
        }
        case PRESENT_AMBIENT_LIGHT_LEVEL:
        {
            if (LCD_AlsSensor.value.als != sensorValue.als && LCD_CurrentScreen == SCREEN_TYPE_MODEM_STATE_PIR_ALS)
                LCD_NeedsUpdate = true;

            LCD_AlsSensor.value.als       = sensorValue.als;
            LCD_AlsSensor.value_state     = (sensorValue.als == MESH_PROP_PRESENT_AMBIENT_LIGHT_LEVEL_UNKNOWN_VAL) ? SENSOR_VALUE_UNKNOWN : SENSOR_VALUE_ACTUAL;
            LCD_AlsSensor.value_timestamp = Timestamp_GetCurrent();
            break;
        }
        case PRESENT_DEVICE_INPUT_POWER:
        {
            if (LCD_PowerSensor.value.power != sensorValue.power && LCD_CurrentScreen == SCREEN_TYPE_ENERGY_SENSORS)
                LCD_NeedsUpdate = true;

            LCD_PowerSensor.value.power = sensorValue.power;
            LCD_PowerSensor.value_state = (sensorValue.power == MESH_PROP_PRESENT_DEVICE_INPUT_POWER_UNKNOWN_VAL) ? SENSOR_VALUE_UNKNOWN : SENSOR_VALUE_ACTUAL;
            LCD_PowerSensor.value_timestamp = Timestamp_GetCurrent();
            break;
        }
        case PRESENT_INPUT_CURRENT:
        {
            if (LCD_CurrentSensor.value.current != sensorValue.current && LCD_CurrentScreen == SCREEN_TYPE_ENERGY_SENSORS)
                LCD_NeedsUpdate = true;

            LCD_CurrentSensor.value.current = sensorValue.current;
            LCD_CurrentSensor.value_state   = (sensorValue.current == MESH_PROP_PRESENT_INPUT_CURRENT_UNKNOWN_VAL) ? SENSOR_VALUE_UNKNOWN : SENSOR_VALUE_ACTUAL;
            LCD_CurrentSensor.value_timestamp = Timestamp_GetCurrent();
            break;
        }
        case PRESENT_INPUT_VOLTAGE:
        {
            if (LCD_VoltageSensor.value.voltage != sensorValue.voltage && LCD_CurrentScreen == SCREEN_TYPE_ENERGY_SENSORS)
                LCD_NeedsUpdate = true;

            LCD_VoltageSensor.value.voltage = sensorValue.voltage;
            LCD_VoltageSensor.value_state   = (sensorValue.voltage == MESH_PROP_PRESENT_INPUT_VOLTAGE_UNKNOWN_VAL) ? SENSOR_VALUE_UNKNOWN : SENSOR_VALUE_ACTUAL;
            LCD_VoltageSensor.value_timestamp = Timestamp_GetCurrent();
            break;
        }
        case TOTAL_DEVICE_ENERGY_USE:
        {
            if (LCD_EnergySensor.value.energy != sensorValue.energy && LCD_CurrentScreen == SCREEN_TYPE_ENERGY_SENSORS)
                LCD_NeedsUpdate = true;

            LCD_EnergySensor.value.energy = sensorValue.energy;
            LCD_EnergySensor.value_state  = (sensorValue.energy == MESH_PROP_TOTAL_DEVICE_ENERGY_USE_UNKNOWN_VAL) ? SENSOR_VALUE_UNKNOWN : SENSOR_VALUE_ACTUAL;
            LCD_EnergySensor.value_timestamp = Timestamp_GetCurrent();
            break;
        }
        case PRECISE_TOTAL_DEVICE_ENERGY_USE:
        {
            if (LCD_PreciseEnergySensor.value.precise_energy != sensorValue.precise_energy && LCD_CurrentScreen == SCREEN_TYPE_ENERGY_SENSORS)
                LCD_NeedsUpdate = true;

            LCD_PreciseEnergySensor.value.precise_energy = sensorValue.precise_energy;
            LCD_PreciseEnergySensor.value_state          = ((sensorValue.precise_energy == MESH_PROP_PRECISE_TOTAL_DEVICE_ENERGY_USE_UNKNOWN_VAL) ||
                                                   (sensorValue.precise_energy == MESH_PROP_PRECISE_TOTAL_DEVICE_ENERGY_USE_NOT_VALID_VAL))
                                                               ? SENSOR_VALUE_UNKNOWN
                                                               : SENSOR_VALUE_ACTUAL;
            LCD_PreciseEnergySensor.value_timestamp      = Timestamp_GetCurrent();
            break;
        }
        default:
            break;
    }
}

void LCD_UpdateDfuState(bool dfuInProgress)
{
    LCD_DfuInProgress = dfuInProgress;
}

void LCD_EraseSensorsValues(void)
{
    LCD_PirSensor.value_state     = SENSOR_VALUE_UNKNOWN;
    LCD_AlsSensor.value_state     = SENSOR_VALUE_UNKNOWN;
    LCD_PowerSensor.value_state   = SENSOR_VALUE_UNKNOWN;
    LCD_CurrentSensor.value_state = SENSOR_VALUE_UNKNOWN;
    LCD_VoltageSensor.value_state = SENSOR_VALUE_UNKNOWN;
    LCD_EnergySensor.value_state  = SENSOR_VALUE_UNKNOWN;

    LCD_NeedsUpdate = true;
}

static void LCD_Loop(void)
{
    bool switchScreen = (Timestamp_GetTimeElapsed(LCD_CurrentScreenTimestamp, Timestamp_GetCurrent()) >= LCD_SCREEN_SWITCH_INTV_MS);
    CheckSensorValuesExpiration();
    CheckTimeDisplayNeedUpdate();

    if (switchScreen)
        ScreenIterate();
    if (switchScreen | LCD_NeedsUpdate)
        DisplayScreen(LCD_CurrentScreen);
}

static void DisplayLine(size_t line, const char *text)
{
    if (strlen(text) > LCD_COLUMNS_NUMBER)
    {
        LOG_W("Trying to write too long string on LCD: %s", text);
        return;
    }

    LcdDrv_SetCursor(0, line);
    LcdDrv_PrintStr(text, strlen(text));
}

static void DisplayScreen(uint8_t screenNum)
{
    switch (screenNum)
    {
        case SCREEN_TYPE_DFU:
        {
            LcdDrv_Clear();

            if (LCD_DfuInProgress)
            {
                DisplayLine(0, "DFU in progress");
            }

            break;
        }

        case SCREEN_TYPE_MODEM_STATE_PIR_ALS:
        {
            char text[LCD_COLUMNS_NUMBER] = {0};

            LcdDrv_Clear();

            DisplayModemState(0, LCD_ModemState);

            strcpy(text, "ALS: ");
            if (LCD_AlsSensor.value_state != SENSOR_VALUE_UNKNOWN)
            {
                if (LCD_AlsSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), "(");
                itoa(LCD_AlsSensor.value.als / 100, text + strlen(text), 10);
                strcpy(text + strlen(text), ".00");
                itoa(LCD_AlsSensor.value.als % 100, text + strlen(text) - (LCD_AlsSensor.value.als % 100 < 10 ? 1 : 2), 10);
                strcpy(text + strlen(text), " lux");
                if (LCD_AlsSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), ")");
            }
            else
            {
                strcpy(text + strlen(text), "Unknown");
            }
            DisplayLine(2, text);

            strcpy(text, "PIR: ");
            if (LCD_PirSensor.value_state != SENSOR_VALUE_UNKNOWN)
            {
                if (LCD_PirSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), "(");
                LCD_PirSensor.value.pir ? strcpy(text + strlen(text), "True") : strcpy(text + strlen(text), "False");
                if (LCD_PirSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), ")");
            }
            else
            {
                strcpy(text + strlen(text), "Unknown");
            }
            DisplayLine(3, text);

            break;
        }

        case SCREEN_TYPE_ENERGY_SENSORS:
        {
            char text[LCD_COLUMNS_NUMBER] = {0};

            LcdDrv_Clear();

            strcpy(text, "Power:   ");
            if (LCD_PowerSensor.value_state != SENSOR_VALUE_UNKNOWN)
            {
                if (LCD_PowerSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), "(");
                itoa(LCD_PowerSensor.value.power / 10, text + strlen(text), 10);
                strcpy(text + strlen(text), ".");
                itoa(LCD_PowerSensor.value.power % 10, text + strlen(text), 10);
                strcpy(text + strlen(text), " W");
                if (LCD_PowerSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), ")");
            }
            else
            {
                strcpy(text + strlen(text), "Unknown");
            }
            DisplayLine(0, text);

            strcpy(text, "Energy:  ");
            if (LCD_EnergySensor.value_state != SENSOR_VALUE_UNKNOWN || LCD_PreciseEnergySensor.value_state != SENSOR_VALUE_UNKNOWN)
            {
                if (Timestamp_Compare(LCD_EnergySensor.value_timestamp, LCD_PreciseEnergySensor.value_timestamp))
                {
                    if (LCD_PreciseEnergySensor.value_state == SENSOR_VALUE_EXPIRED)
                        strcpy(text + strlen(text), "(");
                    itoa(LCD_PreciseEnergySensor.value.precise_energy, text + strlen(text), 10);
                    strcpy(text + strlen(text), " Wh");
                    if (LCD_PreciseEnergySensor.value_state == SENSOR_VALUE_EXPIRED)
                        strcpy(text + strlen(text), ")");
                }
                else
                {
                    if (LCD_EnergySensor.value_state == SENSOR_VALUE_EXPIRED)
                        strcpy(text + strlen(text), "(");
                    itoa(LCD_EnergySensor.value.energy, text + strlen(text), 10);
                    strcpy(text + strlen(text), " kWh");
                    if (LCD_EnergySensor.value_state == SENSOR_VALUE_EXPIRED)
                        strcpy(text + strlen(text), ")");
                }
            }
            else
            {
                strcpy(text + strlen(text), "Unknown");
            }
            DisplayLine(1, text);

            strcpy(text, "Voltage: ");
            if (LCD_VoltageSensor.value_state != SENSOR_VALUE_UNKNOWN)
            {
                if (LCD_VoltageSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), "(");
                itoa(LCD_VoltageSensor.value.voltage / 64, text + strlen(text), 10);
                strcpy(text + strlen(text), ".00");
                itoa(LCD_VoltageSensor.value.voltage % 64, text + strlen(text) - (LCD_VoltageSensor.value.voltage % 64 < 10 ? 1 : 2), 10);
                strcpy(text + strlen(text), " V");
                if (LCD_VoltageSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), ")");
            }
            else
            {
                strcpy(text + strlen(text), "Unknown");
            }
            DisplayLine(2, text);

            strcpy(text, "Current: ");
            if (LCD_CurrentSensor.value_state != SENSOR_VALUE_UNKNOWN)
            {
                if (LCD_CurrentSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), "(");
                itoa(LCD_CurrentSensor.value.current / 100, text + strlen(text), 10);
                strcpy(text + strlen(text), ".00");
                itoa(LCD_CurrentSensor.value.current % 100, text + strlen(text) - (LCD_CurrentSensor.value.current % 100 < 10 ? 1 : 2), 10);
                strcpy(text + strlen(text), " A");
                if (LCD_CurrentSensor.value_state == SENSOR_VALUE_EXPIRED)
                    strcpy(text + strlen(text), ")");
            }
            else
            {
                strcpy(text + strlen(text), "Unknown");
            }
            DisplayLine(3, text);

            break;
        }

        case SCREEN_TYPE_FW_VERSION:
        {
            LcdDrv_Clear();

            DisplayLine(0, "Modem FW version");
            DisplayLine(1, LCD_ModemFwVersion);
            DisplayLine(2, "MCU FW version");
            DisplayLine(3, BUILD_NUMBER);

            break;
        }

        case SCREEN_TYPE_DATE_AND_TIME:
        {
            LcdDrv_Clear();

            struct TimeReceiver_MeshTimeLastSync *last_time_sync = TimeReceiver_GetLastSyncTime();

            DisplayLine(0, "Date:");
            DisplayLine(2, "Time:");

            if (last_time_sync->tai_seconds == TIME_TAI_SECONDS_TIME_UNKNOWN)
            {
                DisplayLine(1, "Unknown");
                DisplayLine(3, "Unknown");
            }
            else
            {
                uint64_t actual_tai_ms = last_time_sync->tai_seconds * 1000 + TIME_SUBSECONDS_TO_MS(last_time_sync->subsecond) +
                                         Timestamp_GetTimeElapsed(last_time_sync->local_sync_timestamp_ms, Timestamp_GetCurrent());

                int16_t          time_zone_offset_minutes = TIME_ZONE_OFFSET_STATE_TO_MIN((int16_t)last_time_sync->time_zone_offset);
                int16_t          leap_seconds             = TIME_TAI_UTC_DELTA_STATE_TO_SEC((int16_t)last_time_sync->tai_utc_delta);
                struct LocalTime local_time               = TAILocalTimeConverter_TAIToLocalTime(actual_tai_ms / 1000, time_zone_offset_minutes, leap_seconds);
                char             text[LCD_COLUMNS_NUMBER];
                char             str_buff[LCD_COLUMNS_NUMBER];

                strcpy(str_buff, "00-00-0000");

                itoa(local_time.day, text, 10);
                strncpy(str_buff + 2 - strlen(text), text, strlen(text));

                itoa(local_time.month + 1, text, 10);
                strncpy(str_buff + 5 - strlen(text), text, strlen(text));

                itoa(local_time.year, text, 10);
                strncpy(str_buff + 10 - strlen(text), text, strlen(text));

                DisplayLine(1, str_buff);

                strcpy(str_buff, "00:00:00");

                itoa(local_time.hour, text, 10);
                strncpy(str_buff + 2 - strlen(text), text, strlen(text));

                itoa(local_time.minutes, text, 10);
                strncpy(str_buff + 5 - strlen(text), text, strlen(text));

                itoa(local_time.seconds, text, 10);
                strncpy(str_buff + 8 - strlen(text), text, strlen(text));
                DisplayLine(3, str_buff);
            }

            break;
        }
        default:
            break;
    }

    LCD_NeedsUpdate = false;
}

static void DisplayModemState(uint8_t lineNumber, enum ModemState modemState)
{
    char const *modem_states[] = {
        "Init Device state",
        "Device state",
        "Init Node state",
        "Node state",
        "Unknown state",
    };

    DisplayLine(lineNumber, modem_states[modemState]);
}

static void ScreenIterate(void)
{
    LCD_CurrentScreen = (enum ScreenType)(LCD_CurrentScreen + 1);
    if (LCD_CurrentScreen == SCREEN_TYPE_DFU && !LCD_DfuInProgress)
        LCD_CurrentScreen = (enum ScreenType)(LCD_CurrentScreen + 1);
    if (LCD_CurrentScreen > SCREEN_TYPE_LAST)
        LCD_CurrentScreen = SCREEN_TYPE_FIRST;

    LCD_CurrentScreenTimestamp = Timestamp_GetCurrent();
}

static void CheckSensorValuesExpiration(void)
{
    if ((Timestamp_GetTimeElapsed(LCD_PirSensor.value_timestamp, Timestamp_GetCurrent()) > LCD_PirSensor.value_expiration_time) &&
        LCD_PirSensor.value_state == SENSOR_VALUE_ACTUAL)
    {
        LCD_PirSensor.value_state = SENSOR_VALUE_EXPIRED;
        LCD_NeedsUpdate           = true;
    }

    if ((Timestamp_GetTimeElapsed(LCD_AlsSensor.value_timestamp, Timestamp_GetCurrent()) > LCD_AlsSensor.value_expiration_time) &&
        LCD_AlsSensor.value_state == SENSOR_VALUE_ACTUAL)
    {
        LCD_AlsSensor.value_state = SENSOR_VALUE_EXPIRED;
        LCD_NeedsUpdate           = true;
    }

    if ((Timestamp_GetTimeElapsed(LCD_PowerSensor.value_timestamp, Timestamp_GetCurrent()) > LCD_PowerSensor.value_expiration_time) &&
        LCD_PowerSensor.value_state == SENSOR_VALUE_ACTUAL)
    {
        LCD_PowerSensor.value_state = SENSOR_VALUE_EXPIRED;
        LCD_NeedsUpdate             = true;
    }

    if ((Timestamp_GetTimeElapsed(LCD_CurrentSensor.value_timestamp, Timestamp_GetCurrent()) > LCD_CurrentSensor.value_expiration_time) &&
        LCD_CurrentSensor.value_state == SENSOR_VALUE_ACTUAL)
    {
        LCD_CurrentSensor.value_state = SENSOR_VALUE_EXPIRED;
        LCD_NeedsUpdate               = true;
    }

    if ((Timestamp_GetTimeElapsed(LCD_VoltageSensor.value_timestamp, Timestamp_GetCurrent()) > LCD_VoltageSensor.value_expiration_time) &&
        LCD_VoltageSensor.value_state == SENSOR_VALUE_ACTUAL)
    {
        LCD_VoltageSensor.value_state = SENSOR_VALUE_EXPIRED;
        LCD_NeedsUpdate               = true;
    }

    if ((Timestamp_GetTimeElapsed(LCD_EnergySensor.value_timestamp, Timestamp_GetCurrent()) > LCD_EnergySensor.value_expiration_time) &&
        LCD_EnergySensor.value_state == SENSOR_VALUE_ACTUAL)
    {
        LCD_EnergySensor.value_state = SENSOR_VALUE_EXPIRED;
        LCD_NeedsUpdate              = true;
    }
}

static void CheckTimeDisplayNeedUpdate(void)
{
    static uint32_t time_update_timestamp = 0;
    if (Timestamp_GetTimeElapsed(time_update_timestamp, Timestamp_GetCurrent()) > LCD_DATE_AND_TIME_UPDATE_PERIOD_MS)
    {
        time_update_timestamp += LCD_DATE_AND_TIME_UPDATE_PERIOD_MS;
        if (LCD_CurrentScreen == SCREEN_TYPE_DATE_AND_TIME)
        {
            struct TimeReceiver_MeshTimeLastSync *last_time_sync = TimeReceiver_GetLastSyncTime();
            if (last_time_sync->tai_seconds != TIME_TAI_SECONDS_TIME_UNKNOWN)
            {
                LCD_NeedsUpdate = true;
            }
        }
    }
}
#else
void LCD_Setup(void)
{
}


void LCD_UpdateModemState(enum ModemState modemState)
{
    UNUSED(modemState);
}


void LCD_UpdateModemFwVersion(char *fwVersion, uint8_t fwVerLen)
{
    UNUSED(fwVersion);
    UNUSED(fwVerLen);
}


void LCD_UpdateSensorValue(enum SensorProperty sensorProperty, union SensorValue sensorValue)
{
    UNUSED(sensorProperty);
    UNUSED(sensorValue);
}

void LCD_UpdateDfuState(bool dfuInProgress)
{
    UNUSED(dfuInProgress);
}

void LCD_EraseSensorsValues(void)
{
}
#endif
