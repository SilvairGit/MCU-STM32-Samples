#ifndef UART_PROTOCOL_TYPES_H
#define UART_PROTOCOL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "Utils.h"

#define UART_PROTOCOL_FRAME_HEADER_LEN 2

#define UART_PROTOCOL_ATTENTION_EVENT_OFF 0
#define UART_PROTOCOL_ATTENTION_EVENT_ON 1

#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_GET 0x824B
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_SET 0x824C
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_SET_UNACKNOWLEDGED 0x824D
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_STATUS 0x824E
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_GET 0x825D
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_SET 0x825E
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_SET_UNACKNOWLEDGED 0x825F
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_STATUS 0x8260
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_GET 0x8261
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_SET 0x8264
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_SET_UNACKNOWLEDGED 0x8265
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_CTL_TEMPERATURE_STATUS 0x8266
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ONOFF_GET 0x8201
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ONOFF_SET 0x8202
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ONOFF_SET_UNACKNOWLEDGED 0x8203
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ONOFF_STATUS 0x8204
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_LEVEL_GET 0x8205
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_LEVEL_SET 0x8206
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_LEVEL_SET_UNACKNOWLEDGED 0x8207
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_LEVEL_STATUS 0x8208
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_DELTA_SET 0x8209
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_DELTA_SET_UNACKNOWLEDGED 0x820A
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ON_POWER_UP_GET 0x8211
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ON_POWER_UP_SET 0x8213
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ON_POWER_UP_SET_UNACKNOWLEDGED 0x8214
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_GENERIC_ON_POWER_UP_STATUS 0x8212
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_LINEAR_GET 0x824F
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_LINEAR_SET 0x8250
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_LINEAR_SET_UNACKNOWLEDGED 0x8251
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_LINEAR_STATUS 0x8252
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_LAST_GET 0x8253
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_LAST_STATUS 0x8254
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_DEFAULT_GET 0x8255
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_DEFAULT_SET 0x8259
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_DEFAULT_SET_UNACKNOWLEDGED 0x825A
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_DEFAULT_STATUS 0x8256
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_RANGE_GET 0x8257
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_RANGE_SET 0x825B
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_RANGE_SET_UNACKNOWLEDGED 0x825C
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_L_RANGE_STATUS 0x8258
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_LC_MODE_GET 0x8291
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_LC_MODE_SET 0x8292
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_LC_MODE_SET_UNACKNOWLEDGED 0x8293
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_LC_MODE_STATUS 0x8294
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_SENSOR_STATUS 0x0052
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_EL 0xEA3601
#define UART_PROTOCOL_MESH_MESSAGE_OPCODE_LIGHT_EL_TEST 0xE93601

struct PACKED UartProtocolFramePingRequest
{
    uint8_t len;
    uint8_t cmd;
    uint8_t p_data[];
};

struct PACKED UartProtocolFramePongResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t p_data[];
};

struct PACKED UartProtocolFrameInitDeviceEvent
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t p_model_id[];
};

struct PACKED UartProtocolFrameCreateInstancesRequest
{
    uint8_t len;
    uint8_t cmd;
    uint8_t p_data[];
};

struct PACKED UartProtocolFrameCreateInstancesResponse
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t p_model_id[];
};

struct PACKED UartProtocolFrameInitNodeEvent
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t p_model_id[];
};

struct PACKED UartProtocolFrameMeshMessageRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint8_t  sub_index;
    uint16_t mesh_opcode;
    uint8_t  p_data[];
};

struct PACKED UartProtocolFrameStartNodeRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameStartNodeResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameFactoryResetRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameFactoryResetResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameFactoryResetEvent
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameMeshMessageResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
    uint8_t sub_index;
};

struct PACKED UartProtocolFrameCurrentStateRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameCurrentStateResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t state_id;
};

struct PACKED UartProtocolFrameError
{
    uint8_t len;
    uint8_t cmd;
    uint8_t error_id;
    uint8_t p_error_parameter[];
};

struct PACKED UartProtocolFrameModemFirmwareVersionRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameModemFirmwareVersionResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t p_fw_version[];
};

struct PACKED UartProtocolFrameSensorUpdateRequest
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
    uint8_t p_data[];
};

struct PACKED UartProtocolFrameAttentionEvent
{
    uint8_t len;
    uint8_t cmd;
    uint8_t attention;
};

struct PACKED UartProtocolFrameSoftResetRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameSoftResetResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameSensorUpdateResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameDeviceUuidRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameDeviceUUIDResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t p_uuid[16];
};

struct PACKED UartProtocolFrameSetFaultRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t company_id;
    uint8_t  fault_id;
    uint8_t  instance_index;
};

struct PACKED UartProtocolFrameSetFaultResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameClearFaultRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t company_id;
    uint8_t  fault_id;
    uint8_t  instance_index;
};

struct PACKED UartProtocolFrameClearFaultResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameStartTestRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t company_id;
    uint8_t  test_id;
    uint8_t  instance_index;
};

struct PACKED UartProtocolFrameStartTestResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameTestFinishedRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint16_t company_id;
    uint8_t  test_id;
    uint8_t  instance_index;
};

struct PACKED UartProtocolFrameTestFinishedResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameFirmwareVersionSetRequest
{
    uint8_t len;
    uint8_t cmd;
    uint8_t p_data[];
};

struct PACKED UartProtocolFrameFirmwareVersionSetResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameBatteryStatusSetRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint8_t  battery_level;
    uint32_t time_to_discharge : 24;
    uint32_t time_to_charge : 24;
    uint8_t  flags;
};

struct PACKED UartProtocolFrameBatteryStatusSetResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameMeshMessageRequest1
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
    uint8_t sub_index;
    uint8_t p_data[];
};

struct PACKED UartProtocolFrameMeshMessageRequest1Opcode1B
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
    uint8_t sub_index;
    uint8_t mesh_opcode;
    uint8_t p_data[];
};

struct PACKED UartProtocolFrameMeshMessageRequest1Opcode2B
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint8_t  sub_index;
    uint16_t mesh_opcode_be;
    uint8_t  p_data[];
};

struct PACKED UartProtocolFrameMeshMessageRequest1Opcode3B
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint8_t  sub_index;
    uint32_t mesh_opcode_be : 24;
    uint8_t  p_data[];
};

struct PACKED UartProtocolFrameTimeSourceSetRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint16_t year;
    uint8_t  month;
    uint8_t  day_of_month;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint16_t miniseconds;
};

struct PACKED UartProtocolFrameTimeSourceSetResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
};

struct PACKED UartProtocolFrameTimeSourceGetRequest
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
};

struct PACKED UartProtocolFrameTimeSourceGetResponse
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint16_t year;
    uint8_t  month;
    uint8_t  day_of_month;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint16_t miniseconds;
};

struct PACKED UartProtocolFrameTimeGetRequest
{
    uint8_t len;
    uint8_t cmd;
    uint8_t instance_index;
};

struct PACKED UartProtocolFrameTimeGetResponse
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  instance_index;
    uint64_t tai_seconds : 40;
    uint8_t  subseconds;
    uint16_t tai_utc_delta;
    uint8_t  time_zone_offset;
};

struct PACKED UartProtocolFrameDfuInitRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint32_t fw_size;
    uint8_t  sha256[32];
    uint8_t  app_data_len;
    uint8_t  p_app_data[];
};

struct PACKED UartProtocolFrameDfuInitResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t status;
};

struct PACKED UartProtocolFrameDfuStatusRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameDfuStatusResponse
{
    uint8_t  len;
    uint8_t  cmd;
    uint8_t  status;
    uint32_t supp_page_size;
    uint32_t firmware_offset;
    uint32_t firmware_crc;
};

struct PACKED UartProtocolFrameDfuPageCreateRequest
{
    uint8_t  len;
    uint8_t  cmd;
    uint32_t req_page_size;
};

struct PACKED UartProtocolFrameDfuPageCreateResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t status;
};

struct PACKED UartProtocolFrameDfuWriteDataEvent
{
    uint8_t len;
    uint8_t cmd;
    uint8_t image_data_len;
    uint8_t p_image_data[];
};

struct PACKED UartProtocolFrameDfuPageStoreRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameDfuPageStoreResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t status;
};

struct PACKED UartProtocolFrameDfuStateCheckRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameDfuStateCheckResponse
{
    uint8_t len;
    uint8_t cmd;
    uint8_t state;
};

struct PACKED UartProtocolFrameDfuCancelRequest
{
    uint8_t len;
    uint8_t cmd;
};

struct PACKED UartProtocolFrameDfuCancelResponse
{
    uint8_t len;
    uint8_t cmd;
};

struct UartProtocolFrameMeshMessageFrame
{
    uint8_t  instance_index;
    uint8_t  sub_index;
    uint32_t mesh_opcode;
    uint8_t *p_mesh_msg_payload;
    uint8_t  mesh_msg_len;
};

// This static asserts not count the variable part of the UART frame
STATIC_ASSERT(sizeof(struct UartProtocolFramePingRequest) == 2, Wrong_size_of_the_struct_UartProtocolFramePingRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFramePongResponse) == 2, Wrong_size_of_the_struct_UartProtocolFramePongResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameInitDeviceEvent) == 2, Wrong_size_of_the_struct_UartProtocolFrameInitDeviceEvent);
STATIC_ASSERT(sizeof(struct UartProtocolFrameCreateInstancesRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameCreateInstancesRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameCreateInstancesResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameCreateInstancesResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameInitNodeEvent) == 2, Wrong_size_of_the_struct_UartProtocolFrameInitNodeEvent);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageRequest) == 6, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameStartNodeRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameStartNodeRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameStartNodeResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameStartNodeResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameFactoryResetRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameFactoryResetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameFactoryResetResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameFactoryResetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameFactoryResetEvent) == 2, Wrong_size_of_the_struct_UartProtocolFrameFactoryResetEvent);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageResponse) == 4, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameCurrentStateRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameCurrentStateRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameCurrentStateResponse) == 3, Wrong_size_of_the_struct_UartProtocolFrameCurrentStateResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameError) == 3, Wrong_size_of_the_struct_UartProtocolFrameError);
STATIC_ASSERT(sizeof(struct UartProtocolFrameModemFirmwareVersionRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameModemFirmwareVersionRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameModemFirmwareVersionResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameModemFirmwareVersionResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameSensorUpdateRequest) == 3, Wrong_size_of_the_struct_UartProtocolFrameSensorUpdateRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameAttentionEvent) == 3, Wrong_size_of_the_struct_UartProtocolFrameAttentionEvent);
STATIC_ASSERT(sizeof(struct UartProtocolFrameSoftResetRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameSoftResetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameSoftResetResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameSoftResetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameSensorUpdateResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameSensorUpdateResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDeviceUuidRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameDeviceUuidRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDeviceUUIDResponse) == 18, Wrong_size_of_the_struct_UartProtocolFrameDeviceUUIDResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameSetFaultRequest) == 6, Wrong_size_of_the_struct_UartProtocolFrameSetFaultRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameSetFaultResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameSetFaultResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameClearFaultRequest) == 6, Wrong_size_of_the_struct_UartProtocolFrameClearFaultRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameClearFaultResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameClearFaultResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameStartTestRequest) == 6, Wrong_size_of_the_struct_UartProtocolFrameStartTestRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameStartTestResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameStartTestResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTestFinishedRequest) == 6, Wrong_size_of_the_struct_UartProtocolFrameTestFinishedRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTestFinishedResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameTestFinishedResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameFirmwareVersionSetRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameFirmwareVersionSetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameFirmwareVersionSetResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameFirmwareVersionSetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameBatteryStatusSetRequest) == 11, Wrong_size_of_the_struct_UartProtocolFrameBatteryStatusSetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameBatteryStatusSetResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameBatteryStatusSetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageRequest1) == 4, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageRequest1);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode1B) == 5, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageRequest1Opcode1B);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode2B) == 6, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageRequest1Opcode2B);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageRequest1Opcode3B) == 7, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageRequest1Opcode3B);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTimeSourceSetRequest) == 12, Wrong_size_of_the_struct_UartProtocolFrameTimeSourceSetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTimeSourceSetResponse) == 3, Wrong_size_of_the_struct_UartProtocolFrameTimeSourceSetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTimeSourceGetRequest) == 3, Wrong_size_of_the_struct_UartProtocolFrameTimeSourceGetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTimeSourceGetResponse) == 12, Wrong_size_of_the_struct_UartProtocolFrameTimeSourceGetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTimeGetRequest) == 3, Wrong_size_of_the_struct_UartProtocolFrameTimeGetRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameTimeGetResponse) == 12, Wrong_size_of_the_struct_UartProtocolFrameTimeGetResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuInitRequest) == 39, Wrong_size_of_the_struct_UartProtocolFrameDfuInitRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuInitResponse) == 3, Wrong_size_of_the_struct_UartProtocolFrameDfuInitResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuStatusRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameDfuStatusRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuStatusResponse) == 15, Wrong_size_of_the_struct_UartProtocolFrameDfuStatusResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuPageCreateRequest) == 6, Wrong_size_of_the_struct_UartProtocolFrameDfuPageCreateRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuPageCreateResponse) == 3, Wrong_size_of_the_struct_UartProtocolFrameDfuPageCreateResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuWriteDataEvent) == 3, Wrong_size_of_the_struct_UartProtocolFrameDfuWriteDataEvent);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuPageStoreRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameDfuPageStoreRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuPageStoreResponse) == 3, Wrong_size_of_the_struct_UartProtocolFrameDfuPageStoreResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuStateCheckRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameDfuStateCheckRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuStateCheckResponse) == 3, Wrong_size_of_the_struct_UartProtocolFrameDfuStateCheckResponse);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuCancelRequest) == 2, Wrong_size_of_the_struct_UartProtocolFrameDfuCancelRequest);
STATIC_ASSERT(sizeof(struct UartProtocolFrameDfuCancelResponse) == 2, Wrong_size_of_the_struct_UartProtocolFrameDfuCancelResponse);

// Mesh message frames
struct PACKED UartProtocolFrameMeshMessageLightLStatusV1
{
    uint16_t present_lightness;
};

struct PACKED UartProtocolFrameMeshMessageLightLStatusV2
{
    uint16_t present_lightness;
    uint16_t target_lightness;
    uint8_t  remaining_time;
};

struct PACKED UartProtocolFrameMeshMessageLightCtlTempStatusV1
{
    uint16_t present_ctl_temperature;
    uint16_t present_ctl_delta_uv;
};

struct PACKED UartProtocolFrameMeshMessageLightCtlTempStatusV2
{
    uint16_t present_ctl_temperature;
    uint16_t present_ctl_delta_uv;
    uint16_t target_ctl_temperature;
    uint16_t target_ctl_delat_uv;
    uint8_t  remaining_time;
};

STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageLightLStatusV1) == 2, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageLightLStatusV1);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageLightLStatusV2) == 5, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageLightLStatusV2);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV1) == 4, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageLightCtlTempStatusV1);
STATIC_ASSERT(sizeof(struct UartProtocolFrameMeshMessageLightCtlTempStatusV2) == 9, Wrong_size_of_the_struct_UartProtocolFrameMeshMessageLightCtlTempStatusV2);

#endif
