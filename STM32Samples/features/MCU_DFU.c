#include "MCU_DFU.h"

#include <stdint.h>
#include <string.h>

#include "Assert.h"
#include "Checksum.h"
#include "Config.h"
#include "FlashHal.h"
#include "GpioHal.h"
#include "LCD.h"
#include "Log.h"
#include "Timestamp.h"
#include "UartProtocol.h"
#include "Utils.h"
#include "WatchdogHal.h"

#define SHA256_SIZE 32u
#define MAX_PAGE_SIZE 1024UL
#define MAX_APP_DATA_LEN 32

#define DFU_INVALID_CODE 0x00
#define DFU_SUCCESS 0x01
#define DFU_OPCODE_NOT_SUPPORTED 0x02
#define DFU_INVALID_PARAMETER 0x03
#define DFU_INSUFFICIENT_RESOURCES 0x04
#define DFU_INVALID_OBJECT 0x05
#define DFU_UNSUPPORTED_TYPE 0x07
#define DFU_OPERATION_NOT_PERMITTED 0x08
#define DFU_OPERATION_FAILED 0x0A
#define DFU_FIRMWARE_ALREADY_UP_TO_DATE 0x80
#define DFU_FIRMWARE_SUCCESSFULLY_UPDATED 0xFF

#define DFU_STATUS_IN_PROGRESS 0x00
#define DFU_STATUS_NOT_IN_PROGRESS 0x01

#define DFU_PAGE_CREATE_PAYLOAD_SIZE 4

#define DFU_CRC32_INIT_VAL 0xFFFFFFFFu


/**< CRC configuration */
#define CRC_POLYNOMIAL 0x8005u
#define CRC_INIT_VAL 0xFFFFu

/**< Defines string that forces update */
#define DFU_VALIDATION_IGNORE_STRING "ignore"

static void ProcessDfuInitRequest(uint8_t *p_payload, uint8_t len);
static void ProcessDfuStatusRequest(uint8_t *p_payload, uint8_t len);
static void ProcessDfuPageCreateRequest(uint8_t *p_payload, uint8_t len);
static void ProcessDfuWriteDataEvent(uint8_t *p_payload, uint8_t len);
static void ProcessDfuPageStoreRequest(uint8_t *p_payload, uint8_t len);
static void ProcessDfuStateCheckResponse(uint8_t *p_payload, uint8_t len);
static void ProcessDfuCancelResponse(uint8_t *p_payload, uint8_t len);

static uint8_t  ValidateAppData(uint8_t *p_app_data, uint8_t app_data_len);
static void     ClearStates(void);
static uint32_t CalcCRC(void);

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame);

static const enum UartFrameCmd UartFrameCommandList[] = {
    UART_FRAME_CMD_DFU_INIT_REQ,
    UART_FRAME_CMD_DFU_STATUS_REQ,
    UART_FRAME_CMD_DFU_PAGE_CREATE_REQ,
    UART_FRAME_CMD_DFU_WRITE_DATA_EVENT,
    UART_FRAME_CMD_DFU_PAGE_STORE_REQ,
    UART_FRAME_CMD_DFU_STATE_CHECK_RESP,
    UART_FRAME_CMD_DFU_CANCEL_RESP,
};

static struct UartProtocolHandlerConfig MessageHandlerConfig = {
    .p_uart_message_handler       = UartMessageHandler,
    .p_mesh_message_handler       = NULL,
    .p_uart_frame_command_list    = UartFrameCommandList,
    .p_mesh_message_opcode_list   = NULL,
    .uart_frame_command_list_len  = ARRAY_SIZE(UartFrameCommandList),
    .mesh_message_opcode_list_len = 0,
    .instance_index               = UART_PROTOCOL_INSTANCE_INDEX_UNKNOWN,
};

static uint8_t DfuInProgress             = 0;
static size_t  FirmwareSize              = 0;
static size_t  FirmwareOffset            = 0;
static uint8_t Sha256[SHA256_SIZE]       = {0};
static uint8_t PageBuffer[MAX_PAGE_SIZE] = {0};
static size_t  PageOffset                = 0;
static size_t  PageSize                  = 0;

void MCU_DFU_Setup(void)
{
    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    if (!FlashHal_IsInitialized())
    {
        FlashHal_Init();
    }

    ClearStates();

    LOG_D("DFU space start addr: 0x%08X", (unsigned int)FlashHal_GetSpaceAddress());
    LOG_D("DFU available bytes:  %d", FlashHal_GetSpaceSize());

    UartProtocol_RegisterMessageHandler(&MessageHandlerConfig);
}

bool MCU_DFU_IsInProgress(void)
{
    return (bool)DfuInProgress;
}

static void ProcessDfuInitRequest(uint8_t *p_payload, uint8_t len)
{
    ClearStates();

    size_t index = 0;

    FirmwareSize = ((uint32_t)p_payload[index++]);
    FirmwareSize |= ((uint32_t)p_payload[index++] << 8);
    FirmwareSize |= ((uint32_t)p_payload[index++] << 16);
    FirmwareSize |= ((uint32_t)p_payload[index++] << 24);

    size_t i;
    for (i = 0; i < SHA256_SIZE; i++)
    {
        Sha256[SHA256_SIZE - i - 1] = p_payload[index++];
    }

    uint8_t  app_data_len = p_payload[index++];
    uint8_t *p_app_data   = p_payload + index;

    uint8_t init_status = ValidateAppData(p_app_data, app_data_len);
    if ((init_status != DFU_SUCCESS) || (index + app_data_len != len))
    {
        UartProtocol_Send(UART_FRAME_CMD_DFU_INIT_RESP, &init_status, sizeof(init_status));

        ClearStates();

        LOG_W("DFU Rejected");

        return;
    }

    size_t available = FlashHal_GetSpaceSize();
    if (available >= FirmwareSize)
    {
        FlashHal_EraseSpace();

        uint8_t init_status[] = {DFU_SUCCESS};
        UartProtocol_Send(UART_FRAME_CMD_DFU_INIT_RESP, init_status, sizeof(init_status));

        DfuInProgress = 1;
        LCD_UpdateDfuState(DfuInProgress);
    }
    else
    {
        uint8_t init_status[] = {DFU_INSUFFICIENT_RESOURCES};
        UartProtocol_Send(UART_FRAME_CMD_DFU_INIT_RESP, init_status, sizeof(init_status));

        ClearStates();
    }

    LOG_D("DFU Init:");
    LOG_D("Size: %d", FirmwareSize);
    LOG_D("Available:%d", available);
    LOG_HEX_D("SHA256:", Sha256, SHA256_SIZE);

    UNUSED(len);
}

static void ProcessDfuStatusRequest(uint8_t *p_payload, uint8_t len)
{
    uint32_t offset = FirmwareOffset + PageOffset;
    uint32_t crc    = CalcCRC();

    uint8_t response[] = {
        DFU_SUCCESS,

        (uint8_t)MAX_PAGE_SIZE,
        (uint8_t)(MAX_PAGE_SIZE >> 8),
        (uint8_t)(MAX_PAGE_SIZE >> 16),
        (uint8_t)(MAX_PAGE_SIZE >> 24),

        (uint8_t)offset,
        (uint8_t)(offset >> 8),
        (uint8_t)(offset >> 16),
        (uint8_t)(offset >> 24),

        (uint8_t)crc,
        (uint8_t)(crc >> 8),
        (uint8_t)(crc >> 16),
        (uint8_t)(crc >> 24),
    };

    if (!DfuInProgress)
    {
        response[0] = DFU_OPERATION_NOT_PERMITTED;
    }

    UartProtocol_Send(UART_FRAME_CMD_DFU_STATUS_RESP, response, sizeof(response));

    LOG_D("DFU Status:");
    LOG_D("Max page: %08X", (unsigned int)MAX_PAGE_SIZE);
    LOG_D("offset: %08X", (unsigned int)offset);
    LOG_D("crc: %08X", (unsigned int)crc);

    UNUSED(p_payload);
    UNUSED(len);
}

static void ProcessDfuPageCreateRequest(uint8_t *p_payload, uint8_t len)
{
    if (!DfuInProgress)
    {
        uint8_t response[] = {DFU_OPERATION_NOT_PERMITTED};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_CREATE_RESP, response, sizeof(response));
        UartProtocol_Send(UART_FRAME_CMD_DFU_CANCEL_REQ, NULL, 0);
        LOG_W("DFU Page, dfu not in progress");
        return;
    }

    if (len != DFU_PAGE_CREATE_PAYLOAD_SIZE)
    {
        uint8_t response[] = {DFU_INVALID_OBJECT};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_CREATE_RESP, response, sizeof(response));
        return;
    }

    size_t   index = 0;
    uint32_t req_page_size;
    req_page_size = ((uint32_t)p_payload[index++]);
    req_page_size |= ((uint32_t)p_payload[index++] << 8);
    req_page_size |= ((uint32_t)p_payload[index++] << 16);
    req_page_size |= ((uint32_t)p_payload[index++] << 24);

    if ((req_page_size == 0) || (FirmwareOffset + req_page_size > FlashHal_GetSpaceSize()) || (FirmwareOffset + req_page_size > FirmwareSize))
    {
        uint8_t response[] = {DFU_INVALID_OBJECT};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_CREATE_RESP, response, sizeof(response));
        return;
    }

    if (req_page_size <= MAX_PAGE_SIZE)
    {
        PageOffset = 0;
        PageSize   = req_page_size;

        uint8_t response[] = {DFU_SUCCESS};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_CREATE_RESP, response, sizeof(response));
        LOG_D("DFU Page Created:");
        LOG_D("Size: %08X", (unsigned int)req_page_size);
    }
    else
    {
        uint8_t response[] = {DFU_INSUFFICIENT_RESOURCES};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_CREATE_RESP, response, sizeof(response));
        LOG_W("DFU Page Invalid Size:");
        LOG_W("Size: %08X", (unsigned int)req_page_size);
    }
}

static void ProcessDfuWriteDataEvent(uint8_t *p_payload, uint8_t len)
{
    if (!DfuInProgress)
    {
        UartProtocol_Send(UART_FRAME_CMD_DFU_CANCEL_REQ, NULL, 0);
        LOG_W("DFU Write data, dfu not in progress");
        return;
    }

    size_t   index     = 0;
    uint8_t  image_len = p_payload[index++];
    uint8_t *p_image   = p_payload + index;

    if (PageOffset + image_len <= PageSize)
    {
        memcpy(PageBuffer + PageOffset, p_image, image_len);
        PageOffset += image_len;
    }

    UNUSED(len);
}

static void ProcessDfuPageStoreRequest(uint8_t *p_payload, uint8_t len)
{
    if (!DfuInProgress)
    {
        uint8_t response[] = {DFU_OPERATION_NOT_PERMITTED};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));
        UartProtocol_Send(UART_FRAME_CMD_DFU_CANCEL_REQ, NULL, 0);
        LOG_W("DFU Write data, dfu not in progress");
        return;
    }

    if (PageOffset == 0)
    {
        uint8_t response[] = {DFU_SUCCESS};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));
        LOG_W("DFU Page not stored");
        return;
    }

    if (PageOffset != PageSize)
    {
        uint8_t response[] = {DFU_OPERATION_NOT_PERMITTED};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));

        LOG_W("DFU Page store failed, size doesn't match");
        return;
    }

    uint32_t page_store_address = FlashHal_GetSpaceAddress() + FirmwareOffset;
    bool     ret_val            = FlashHal_SaveToFlash(page_store_address, (uint32_t *)PageBuffer, PageSize / 4);
    if (ret_val != true)
    {
        uint8_t response[] = {DFU_OPERATION_FAILED};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));

        LOG_W("DFU Page not stored, flasher fail");
        return;
    }

    FirmwareOffset += PageOffset;
    PageOffset = 0;
    PageSize   = 0;

    if (FirmwareOffset != FirmwareSize)
    {
        uint8_t response[] = {DFU_SUCCESS};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));

        uint32_t crc = Checksum_CalcCRC32((uint8_t *)((uintptr_t)FlashHal_GetSpaceAddress()), FirmwareOffset, CRC_INIT_VAL);
        LOG_D("DFU Page store success, CRC %08X", (unsigned int)crc);
        return;
    }

    uint8_t calculated_sha256[SHA256_SIZE];
    Checksum_CalcSHA256((uint8_t *)((uintptr_t)FlashHal_GetSpaceAddress()), FirmwareOffset, calculated_sha256);
    bool is_object_valid = (0 == memcmp(calculated_sha256, Sha256, SHA256_SIZE));

    if (!is_object_valid)
    {
        uint8_t response[] = {DFU_INVALID_OBJECT};
        UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));

        LOG_W("DFU Invalid object");
        ClearStates();
        return;
    }

    uint8_t response[] = {DFU_FIRMWARE_SUCCESSFULLY_UPDATED};
    UartProtocol_Send(UART_FRAME_CMD_DFU_PAGE_STORE_RESP, response, sizeof(response));

    LOG_D("DFU Firmware updated");
    LOG_FLUSH();
    UartProtocol_Flush();

    size_t FwSizeWords = FirmwareSize / sizeof(uint32_t);

    ClearStates();
    WatchdogHal_Refresh();
    FlashHal_UpdateFirmware(FwSizeWords);

    //Should not get there
    for (;;)
    {
        GpioHal_PinSet(GPIO_HAL_PIN_LED_STATUS, false);
        Timestamp_DelayMs(1000);
        GpioHal_PinSet(GPIO_HAL_PIN_LED_STATUS, true);
        Timestamp_DelayMs(1000);
    }

    UNUSED(p_payload);
    UNUSED(len);
}

static void ProcessDfuStateCheckResponse(uint8_t *p_payload, uint8_t len)
{
    size_t  index  = 0;
    uint8_t status = p_payload[index++];

    if ((status == DFU_STATUS_IN_PROGRESS) != (DfuInProgress))
    {
        UartProtocol_Send(UART_FRAME_CMD_DFU_CANCEL_REQ, NULL, 0);
        LOG_W("DFU Canceling");
    }

    UNUSED(len);
}

static void ProcessDfuCancelResponse(uint8_t *p_payload, uint8_t len)
{
    ClearStates();
    LOG_D("DFU Cancelled");

    UNUSED(p_payload);
    UNUSED(len);
}

static uint8_t ValidateAppData(uint8_t *p_app_data, uint8_t app_data_len)
{
    LOG_D("Application Data length: %d", app_data_len);
    LOG_D("Application Data: %s", p_app_data);
    LOG_HEX_D("Application Data:", p_app_data, app_data_len);

    if (app_data_len > MAX_APP_DATA_LEN)
    {
        return DFU_INVALID_OBJECT;
    }

    if (app_data_len == strlen(DFU_VALIDATION_IGNORE_STRING) && memcmp(p_app_data, DFU_VALIDATION_IGNORE_STRING, app_data_len) == 0)
    {
        /* Application Data contains special string that always validates the package */
        return DFU_SUCCESS;
    }

    /* Valid Application Data is in format: DFU_VALIDATION_STRING/BUILD_NUMBER */
    uint8_t *delimiter = (uint8_t *)memchr(p_app_data, '/', app_data_len);
    if (delimiter == NULL)
    {
        /* Application Data does not contain delimiter */
        return DFU_INVALID_OBJECT;
    }
    uint8_t *fw_type      = p_app_data;
    uint8_t  fw_type_len  = delimiter - p_app_data;
    uint8_t *fw_build     = &p_app_data[fw_type_len + 1];
    uint8_t  fw_build_len = app_data_len - fw_type_len - 1;

    if (strncmp((char *)fw_type, DFU_VALIDATION_STRING, fw_type_len) || (fw_type_len != strlen(DFU_VALIDATION_STRING)))
    {
        /* DFU package contains different type of firmware */
        return DFU_INVALID_OBJECT;
    }

    if (!strncmp((char *)fw_build, BUILD_NUMBER, fw_build_len) && (fw_build_len == strlen(BUILD_NUMBER)))
    {
        /* Application Data contains the same firmware that exists on the device */
        return DFU_FIRMWARE_ALREADY_UP_TO_DATE;
    }

    return DFU_SUCCESS;
}

static void ClearStates(void)
{
    DfuInProgress  = 0;
    FirmwareSize   = 0;
    FirmwareOffset = 0;
    PageOffset     = 0;
    PageSize       = 0;

    memset(Sha256, 0, SHA256_SIZE);
    memset(PageBuffer, 0, MAX_PAGE_SIZE);

    LCD_UpdateDfuState(DfuInProgress);
}

static uint32_t CalcCRC(void)
{
    uint32_t crc = ~DFU_CRC32_INIT_VAL;
    if (FirmwareOffset != 0)
    {
        crc = Checksum_CalcCRC32((uint8_t *)((uintptr_t)FlashHal_GetSpaceAddress()), FirmwareOffset, ~crc);
    }
    if (PageOffset != 0)
    {
        crc = Checksum_CalcCRC32(PageBuffer, PageOffset, ~crc);
    }
    return crc;
}

static void UartMessageHandler(struct UartFrameRxTxFrame *p_frame)
{
    ASSERT(p_frame != NULL);

    switch (p_frame->cmd)
    {
        case UART_FRAME_CMD_DFU_INIT_REQ:
            ProcessDfuInitRequest(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_DFU_STATUS_REQ:
            ProcessDfuStatusRequest(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_DFU_PAGE_CREATE_REQ:
            ProcessDfuPageCreateRequest(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_DFU_WRITE_DATA_EVENT:
            ProcessDfuWriteDataEvent(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_DFU_PAGE_STORE_REQ:
            ProcessDfuPageStoreRequest(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_DFU_STATE_CHECK_RESP:
            ProcessDfuStateCheckResponse(p_frame->p_payload, p_frame->len);
            break;

        case UART_FRAME_CMD_DFU_CANCEL_RESP:
            ProcessDfuCancelResponse(p_frame->p_payload, p_frame->len);
            break;

        default:
            break;
    }
}
