#include "ModelManager.h"

#include <stddef.h>
#include <string.h>

#include "Assert.h"
#include "Log.h"
#include "UartProtocol.h"

#define MODEL_MANAGER_MAX_NUMBER_OF_REGISTERED_MODELS 16

static struct ModelManagerRegistrationRow *ModelsList[MODEL_MANAGER_MAX_NUMBER_OF_REGISTERED_MODELS];
static uint8_t                             ModelListCnt = 0;

static bool ModelManager_IsModelAvailable(uint16_t expected_model_id, uint16_t *p_available_model_list, uint8_t available_model_list_len);

void ModelManager_RegisterModel(struct ModelManagerRegistrationRow *p_model)
{
    ASSERT((p_model != NULL) && (ModelListCnt < MODEL_MANAGER_MAX_NUMBER_OF_REGISTERED_MODELS) && (p_model->p_instance_index != NULL));

    LOG_D("New model registered at position: %u with id: 0x%04X", (unsigned int)ModelListCnt, p_model->model_id);

    ModelsList[ModelListCnt] = p_model;
    ModelListCnt++;
}

uint8_t ModelManager_GetCreateInstanceIndexPayloadLen(void)
{
    uint8_t size = 0;

    size_t i;
    for (i = 0; i < ModelListCnt; i++)
    {
        size += ModelsList[i]->model_parameter_len + sizeof(ModelsList[i]->model_id);
    }

    return size;
}

uint8_t ModelManager_CreateInstanceIndexPayload(uint8_t *p_payload, uint16_t *p_avaliable_model_list, uint8_t avaliable_model_list_len)
{
    ASSERT((p_payload != NULL) && (p_avaliable_model_list != NULL));

    uint8_t index = 0;

    size_t i;
    for (i = 0; i < ModelListCnt; i++)
    {
        if (!ModelManager_IsModelAvailable(ModelsList[i]->model_id, p_avaliable_model_list, avaliable_model_list_len))
        {
            LOG_W("Model id: 0x%04X not available in InitDeviceEventList", ModelsList[i]->model_id);
            continue;
        }

        memcpy(p_payload + index, &ModelsList[i]->model_id, sizeof(ModelsList[i]->model_id));
        index += sizeof(ModelsList[i]->model_id);

        memcpy(p_payload + index, ModelsList[i]->p_model_parameter, ModelsList[i]->model_parameter_len);
        index += ModelsList[i]->model_parameter_len;
    }

    return index;
}

void ModelManager_SetInstanceIndex(uint16_t model_id, uint8_t instance_index)
{
    size_t i;
    for (i = 0; i < ModelListCnt; i++)
    {
        if ((ModelsList[i]->p_instance_index != NULL) && (*ModelsList[i]->p_instance_index != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN))
        {
            continue;
        }

        if ((ModelsList[i]->model_id == model_id) && (ModelsList[i]->p_instance_index != NULL))
        {
            LOG_D("Instance index of: %u set for model id: 0x%04X at position: %u", instance_index, model_id, i);

            *ModelsList[i]->p_instance_index = instance_index;
            return;
        }
    }

    LOG_W("Model id: 0x%04X to set instance index: %u not found", model_id, instance_index);
}

void ModelManager_ResetAllInstanceIndexes(void)
{
    size_t i;
    for (i = 0; i < ModelListCnt; i++)
    {
        if (ModelsList[i]->p_instance_index != NULL)
        {
            *ModelsList[i]->p_instance_index = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN;
        }
    }
}

bool ModelManager_IsAllModelsRegistered(void)
{
    bool ret_value = true;

    size_t i;
    for (i = 0; i < ModelListCnt; i++)
    {
        if ((ModelsList[i]->p_instance_index != NULL) && (*ModelsList[i]->p_instance_index != UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN))
        {
            continue;
        }

        ret_value = false;
        LOG_E("Model id: 0x%04X not registered", ModelsList[i]->model_id);
    }

    return ret_value;
}

static bool ModelManager_IsModelAvailable(uint16_t expected_model_id, uint16_t *p_available_model_list, uint8_t available_model_list_len)
{
    ASSERT(p_available_model_list != NULL);

    size_t i;
    for (i = 0; i < available_model_list_len; i++)
    {
        if (expected_model_id == p_available_model_list[i])
        {
            return true;
        }
    }
    return false;
}
