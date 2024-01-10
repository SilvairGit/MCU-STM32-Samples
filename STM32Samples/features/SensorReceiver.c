#include "SensorReceiver.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Assert.h"
#include "Config.h"
#include "LCD.h"
#include "Log.h"
#include "Mesh.h"
#include "ModelManager.h"
#include "Timestamp.h"
#include "UartProtocol.h"

#define SS_FORMAT_MASK 0x01
#define SS_SHORT_LEN_MASK 0x1E
#define SS_SHORT_LEN_OFFSET 1
#define SS_SHORT_PROP_ID_LOW_MASK 0xE0
#define SS_SHORT_PROP_ID_LOW_OFFSET 5
#define SS_SHORT_PROP_ID_HIGH_OFFSET 3
#define SS_LONG_LEN_MASK 0xFE
#define SS_LONG_LEN_OFFSET 1

static void ProcessSensorStatus(uint8_t *p_payload, size_t len);
static void ProcessSensorProperty(uint16_t property_id, uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessPresenceDetected(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessPresentAmbientLightLevel(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessDeviceInputPower(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessPresentInputCurrent(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessPresentInputVoltage(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessTotalDeviceEnergyUse(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void ProcessPreciseTotalDeviceEnergyUse(uint8_t *p_payload, size_t len, uint16_t src_addr);
static void MeshMessageHandler(struct UartProtocolFrameMeshMessageFrame *p_frame);
static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame);

static const uint32_t MeshMessageOpcodeList[] = {MESH_MESSAGE_SENSOR_STATUS};

static const enum UartFrameCmd UartFrameCommandList[] = {UART_FRAME_CMD_FACTORY_RESET_EVENT};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = UartMessageHandler,
    .p_mesh_message_handler       = MeshMessageHandler,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = MeshMessageOpcodeList,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = ARRAY_SIZE(MeshMessageOpcodeList),
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

struct ModelManagerRegistrationRow ModelConfigSensorClient = {
    .model_id            = MODEL_MANAGER_ID_SENSOR_CLIENT,
    .p_model_parameter   = NULL,
    .model_parameter_len = 0,
    .p_instance_index    = &MessageHandlerConfig.instance_index,
};

void SensorReceiver_Setup(void)
{
    LOG_D("Sensor Receiver initialization");

    ModelManager_RegisterModel(&ModelConfigSensorClient);

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);
}

static void ProcessSensorStatus(uint8_t *p_payload, size_t len)
{
    if (len < 2)
    {
        LOG_W("Received empty Sensor Status message");
        return;
    }

    uint16_t src_addr = ((uint16_t)p_payload[len - 2]);
    src_addr |= ((uint16_t)p_payload[len - 1] << 8);

    if (len <= 2)
    {
        LOG_W("Received empty Sensor Status message from: %d", src_addr);
        return;
    }

    len -= 2;
    size_t index = 0;
    while (index < len)
    {
        LOG_D("ProcessSensorStatus index: %d", index);
        if (p_payload[index] & SS_FORMAT_MASK)
        {
            /* Length field in Sensor Status message is 0-based */
            size_t   message_len = ((p_payload[index++] & SS_LONG_LEN_MASK) >> SS_LONG_LEN_OFFSET) + 1;
            uint16_t property_id = ((uint16_t)p_payload[index++]);
            property_id |= ((uint16_t)p_payload[index++] << 8);

            ProcessSensorProperty(property_id, p_payload + index, message_len, src_addr);
            index += message_len;
        }
        else
        {
            /* Length field in Sensor Status message is 0-based */
            size_t   message_len = ((p_payload[index] & SS_SHORT_LEN_MASK) >> SS_SHORT_LEN_OFFSET) + 1;
            uint16_t property_id = (p_payload[index++] & SS_SHORT_PROP_ID_LOW_MASK) >> SS_SHORT_PROP_ID_LOW_OFFSET;
            property_id |= ((uint16_t)p_payload[index++]) << SS_SHORT_PROP_ID_HIGH_OFFSET;

            ProcessSensorProperty(property_id, p_payload + index, message_len, src_addr);
            index += message_len;
        }
    }
}

static void ProcessSensorProperty(uint16_t property_id, uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    switch (property_id)
    {
        case PRESENCE_DETECTED:
        {
            ProcessPresenceDetected(p_payload, len, src_addr);
            break;
        }
        case PRESENT_AMBIENT_LIGHT_LEVEL:
        {
            ProcessPresentAmbientLightLevel(p_payload, len, src_addr);
            break;
        }
        case PRESENT_DEVICE_INPUT_POWER:
        {
            ProcessDeviceInputPower(p_payload, len, src_addr);
            break;
        }
        case PRESENT_INPUT_CURRENT:
        {
            ProcessPresentInputCurrent(p_payload, len, src_addr);
            break;
        }
        case PRESENT_INPUT_VOLTAGE:
        {
            ProcessPresentInputVoltage(p_payload, len, src_addr);
            break;
        }
        case TOTAL_DEVICE_ENERGY_USE:
        {
            ProcessTotalDeviceEnergyUse(p_payload, len, src_addr);
            break;
        }
        case PRECISE_TOTAL_DEVICE_ENERGY_USE:
        {
            ProcessPreciseTotalDeviceEnergyUse(p_payload, len, src_addr);
            break;
        }
        default:
        {
            LOG_W("Invalid property id");
        }
    }
}

static void ProcessPresenceDetected(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 1)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value = {.pir = p_payload[0]};

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], PRESENCE DETECTED with value of: %u",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)sensor_value.pir);
    LCD_UpdateSensorValue(PRESENCE_DETECTED, sensor_value);
}

static void ProcessPresentAmbientLightLevel(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 3)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value = {.als = ((uint32_t)p_payload[0]) | ((uint32_t)p_payload[1] << 8) | ((uint32_t)p_payload[2] << 16)};

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], PRESENT AMBIENT LIGHT LEVEL with value of: "
          "%u.%02u",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)(sensor_value.als / 100),
          (unsigned int)(sensor_value.als % 100));
    LCD_UpdateSensorValue(PRESENT_AMBIENT_LIGHT_LEVEL, sensor_value);
}

static void ProcessDeviceInputPower(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 3)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value = {.power = ((uint32_t)p_payload[0]) | ((uint32_t)p_payload[1] << 8) | ((uint32_t)p_payload[2] << 16)};

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], PRESENT DEVICE INPUT POWER with value of: %u.%02u",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)(sensor_value.power / 10),
          (unsigned int)(sensor_value.power % 10));
    LCD_UpdateSensorValue(PRESENT_DEVICE_INPUT_POWER, sensor_value);
}

static void ProcessPresentInputCurrent(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 2)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value;
    sensor_value.current = ((uint16_t)p_payload[0]) | ((uint16_t)p_payload[1] << 8);

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], PRESENT INPUT CURRENT with value of: %u.%02u",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)(sensor_value.current / 100),
          (unsigned int)(sensor_value.current % 100));
    LCD_UpdateSensorValue(PRESENT_INPUT_CURRENT, sensor_value);
}

static void ProcessPresentInputVoltage(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 2)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value;
    sensor_value.voltage = ((uint16_t)p_payload[0]) | ((uint16_t)p_payload[1] << 8);

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], PRESENT INPUT VOLTAGE with value of: %u.%02u",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)(sensor_value.voltage / 64),
          (unsigned int)((sensor_value.voltage % 64) * 100 / 64));
    LCD_UpdateSensorValue(PRESENT_INPUT_VOLTAGE, sensor_value);
}

static void ProcessTotalDeviceEnergyUse(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 3)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value;
    sensor_value.energy = (((uint32_t)p_payload[0]) | ((uint32_t)p_payload[1] << 8) | ((uint32_t)p_payload[2] << 16));

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], TOTAL DEVICE ENERGY USE with value of: "
          "%u.%02u Wh",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)(sensor_value.energy / 100),
          (unsigned int)(sensor_value.energy % 100));
    LCD_UpdateSensorValue(TOTAL_DEVICE_ENERGY_USE, sensor_value);
}

static void ProcessPreciseTotalDeviceEnergyUse(uint8_t *p_payload, size_t len, uint16_t src_addr)
{
    if (len != 4)
    {
        LOG_W("Invalid Length Sensor Status message");
        return;
    }

    union SensorValue sensor_value = {.precise_energy = ((uint32_t)p_payload[0]) | ((uint32_t)p_payload[1] << 8) | ((uint32_t)p_payload[2] << 16) |
                                                        ((uint32_t)p_payload[3] << 24)};

    LOG_D("Decoded Sensor Status message from 0x%04X [%u ms], PRECISE TOTAL DEVICE ENERGY USE with value of: "
          "%u.%02u Wh",
          src_addr,
          (unsigned int)Timestamp_GetCurrent(),
          (unsigned int)(sensor_value.precise_energy / 100),
          (unsigned int)(sensor_value.precise_energy % 100));
    LCD_UpdateSensorValue(PRECISE_TOTAL_DEVICE_ENERGY_USE, sensor_value);
}

static void MeshMessageHandler(struct UartProtocolFrameMeshMessageFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->mesh_opcode)
    {
        case MESH_MESSAGE_SENSOR_STATUS:
            ProcessSensorStatus(p_frame->p_mesh_msg_payload, p_frame->mesh_msg_len);
            break;

        default:
            break;
    }
}

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_FACTORY_RESET_EVENT:
            LCD_EraseSensorsValues();
            break;

        default:
            break;
    }
}
