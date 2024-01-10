#include <string.h>

#include "MockAssert.h"
#include "ModelManager.c"
#include "UartProtocol.h"
#include "Utils.h"
#include "unity.h"

static uint8_t InstanceIndex1 = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
static uint8_t InstanceIndex2 = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
static uint8_t InstanceIndex3 = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;

static uint8_t ModelParameter1[] = {0x01, 0x02, 0x03, 0x04};
static uint8_t ModelParameter2[] = {0x05, 0x06};
static uint8_t ModelParameter3[] = {0x0A, 0x0B, 0x0C};

struct ModelManagerRegistrationRow ModelConfig1 = {
    .p_model_parameter   = ModelParameter1,
    .model_id            = 0x1234,
    .model_parameter_len = ARRAY_SIZE(ModelParameter1),
    .p_instance_index    = &InstanceIndex1,
};

struct ModelManagerRegistrationRow ModelConfig2 = {
    .model_id            = 0x5678,
    .p_model_parameter   = ModelParameter2,
    .model_parameter_len = ARRAY_SIZE(ModelParameter2),
    .p_instance_index    = &InstanceIndex2,
};

struct ModelManagerRegistrationRow ModelConfig3 = {
    .model_id            = 0xABCD,
    .p_model_parameter   = ModelParameter3,
    .model_parameter_len = ARRAY_SIZE(ModelParameter3),
    .p_instance_index    = &InstanceIndex3,
};

struct ModelManagerRegistrationRow ModelConfig4 = {
    .model_id            = 0xEFFE,
    .p_model_parameter   = NULL,
    .model_parameter_len = 0,
    .p_instance_index    = &InstanceIndex3,
};

void setUp(void)
{
    ModelListCnt = 0;

    InstanceIndex1 = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
    InstanceIndex2 = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
    InstanceIndex3 = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
}

void test_RegisterModel(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    TEST_ASSERT_EQUAL(ModelListCnt, 3);

    TEST_ASSERT_EQUAL(ModelsList[0], &ModelConfig1);
    TEST_ASSERT_EQUAL(ModelsList[1], &ModelConfig2);
    TEST_ASSERT_EQUAL(ModelsList[2], &ModelConfig3);
}

void test_RegisterModelWithNullInstanceIndex(void)
{
    struct ModelManagerRegistrationRow ModelConfig = {
        .model_id            = 0x1234,
        .p_model_parameter   = NULL,
        .model_parameter_len = 0,
        .p_instance_index    = NULL,
    };

    Assert_Callback_ExpectAnyArgs();

    ModelManager_RegisterModel(&ModelConfig);
}

void test_GetCreateInstanceIndexPayloadLen(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    TEST_ASSERT_EQUAL(ModelManager_GetCreateInstanceIndexPayloadLen(), 2 * 3 + 4 + 2 + 3);
}

void test_GetCreateInstanceIndexPayloadLenNoModels(void)
{
    TEST_ASSERT_EQUAL(ModelManager_GetCreateInstanceIndexPayloadLen(), 0);
}

void test_CreateInstanceIndexPayload(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    uint8_t len = ModelManager_GetCreateInstanceIndexPayloadLen();
    uint8_t buff[len];

    uint16_t avaliable_model_list[] = {0x1234, 0x5678, 0xABCD, 0xEFFE};

    uint8_t out_len = ModelManager_CreateInstanceIndexPayload(buff, avaliable_model_list, ARRAY_SIZE(avaliable_model_list));

    uint8_t expected_buffer[2 * 3 + 4 + 2 + 3] = {0x34, 0x12, 0x01, 0x02, 0x03, 0x04, 0x78, 0x56, 0x05, 0x06, 0xCD, 0xAB, 0x0A, 0x0B, 0x0C};

    TEST_ASSERT_EQUAL(len, out_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_buffer, buff, sizeof(expected_buffer));
}

void test_CreateInstanceIndexPayloadModelNotAvaliable(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    uint8_t len = ModelManager_GetCreateInstanceIndexPayloadLen();
    uint8_t buff[len];

    uint16_t avaliable_model_list[] = {0x1234, 0xABCD, 0xEFFE};

    uint8_t out_len = ModelManager_CreateInstanceIndexPayload(buff, avaliable_model_list, ARRAY_SIZE(avaliable_model_list));

    uint8_t expected_buffer[2 * 2 + 4 + 3] = {0x34, 0x12, 0x01, 0x02, 0x03, 0x04, 0xCD, 0xAB, 0x0A, 0x0B, 0x0C};

    TEST_ASSERT_EQUAL(out_len, 2 * 2 + 4 + 3);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_buffer, buff, sizeof(expected_buffer));
}

void test_CreateInstanceIndexPayloadNullPyloadPtr(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);
    ModelManager_RegisterModel(&ModelConfig4);

    uint8_t len = ModelManager_GetCreateInstanceIndexPayloadLen();
    uint8_t buff[len];

    uint16_t avaliable_model_list[] = {0x1234, 0x5678, 0xABCD, 0xEFFE};

    uint8_t out_len = ModelManager_CreateInstanceIndexPayload(buff, avaliable_model_list, ARRAY_SIZE(avaliable_model_list));

    uint8_t expected_buffer[2 * 4 + 4 + 2 + 3] = {0x34, 0x12, 0x01, 0x02, 0x03, 0x04, 0x78, 0x56, 0x05, 0x06, 0xCD, 0xAB, 0x0A, 0x0B, 0x0C, 0xFE, 0xEF};

    TEST_ASSERT_EQUAL(len, out_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_buffer, buff, sizeof(expected_buffer));
}

void test_CreateInstanceIndexPayloadNullAvaliableModelListPtr(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    uint8_t buff[10];

    Assert_Callback_ExpectAnyArgs();
    Assert_Callback_ExpectAnyArgs();
    Assert_Callback_ExpectAnyArgs();
    Assert_Callback_ExpectAnyArgs();

    ModelManager_CreateInstanceIndexPayload(buff, NULL, 0);
}

void test_SetInstanceIndex(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    ModelManager_SetInstanceIndex(0x5678, 7);

    TEST_ASSERT_EQUAL(InstanceIndex1, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
    TEST_ASSERT_EQUAL(InstanceIndex2, 7);
    TEST_ASSERT_EQUAL(InstanceIndex3, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
}

void test_ResetAllInstanceIndexes(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    ModelManager_SetInstanceIndex(0x5678, 7);
    ModelManager_SetInstanceIndex(0xABCD, 8);

    TEST_ASSERT_EQUAL(InstanceIndex1, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
    TEST_ASSERT_EQUAL(InstanceIndex2, 7);
    TEST_ASSERT_EQUAL(InstanceIndex3, 8);

    ModelManager_ResetAllInstanceIndexes();

    TEST_ASSERT_EQUAL(InstanceIndex1, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
    TEST_ASSERT_EQUAL(InstanceIndex2, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
    TEST_ASSERT_EQUAL(InstanceIndex3, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
}

void test_SetInstanceIndexManyTimes(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    ModelManager_SetInstanceIndex(0x5678, 7);
    ModelManager_SetInstanceIndex(0x5678, 8);

    TEST_ASSERT_EQUAL(InstanceIndex1, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
    TEST_ASSERT_EQUAL(InstanceIndex2, 7);
    TEST_ASSERT_EQUAL(InstanceIndex3, UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN);
}

void test_ModelAvailable(void)
{
    uint16_t avaliable_model_list[] = {0x1234, 0x5678, 0xABCD, 0xEFFE};

    bool ret_val = ModelManager_IsModelAvailable(0x5678, avaliable_model_list, ARRAY_SIZE(avaliable_model_list));

    TEST_ASSERT_EQUAL(ret_val, true);
}

void test_ModelNotAvailable(void)
{
    uint16_t avaliable_model_list[] = {0x1234, 0x5678, 0xABCD, 0xEFFE};

    bool ret_val = ModelManager_IsModelAvailable(0x1111, avaliable_model_list, ARRAY_SIZE(avaliable_model_list));

    TEST_ASSERT_EQUAL(ret_val, false);
}

void test_AllModelsRegistered(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    ModelManager_SetInstanceIndex(0x1234, 7);
    ModelManager_SetInstanceIndex(0x5678, 8);
    ModelManager_SetInstanceIndex(0xABCD, 9);

    bool ret_val = ModelManager_IsAllModelsRegistered();

    TEST_ASSERT_EQUAL(ret_val, true);
}

void test_NotAllModelsRegistered(void)
{
    ModelManager_RegisterModel(&ModelConfig1);
    ModelManager_RegisterModel(&ModelConfig2);
    ModelManager_RegisterModel(&ModelConfig3);

    ModelManager_SetInstanceIndex(0x1234, 7);
    ModelManager_SetInstanceIndex(0xABCD, 9);

    bool ret_val = ModelManager_IsAllModelsRegistered();

    TEST_ASSERT_EQUAL(ret_val, false);
}
