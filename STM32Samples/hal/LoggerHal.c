#include "LoggerHal.h"

#include <string.h>

#include "Assert.h"
#include "Atomic.h"
#include "Platform.h"
#include "PriorityConfig.h"
#include "RingBuffer.h"

#define LOGGER_HAL_BAUDRATE 500000

#define LOGGER_HAL_RX_BUFFER_LEN 512
#define LOGGER_HAL_TX_BUFFER_LEN 4096

static bool IsInitialized = false;

static volatile uint8_t DmaTxBuffer[LOGGER_HAL_TX_BUFFER_LEN];
static volatile uint8_t DmaRxBuffer[LOGGER_HAL_RX_BUFFER_LEN];

static volatile struct RingBuffer TxDmaBuffer;

static volatile uint16_t CurrentTxTransferLen = 0;
static volatile bool     isFlushInProgress    = false;

static void LoggerHal_InitGpio(void);
static void LoggerHal_InitDma1Ch2Ch3(void);
static void LoggerHal_InitNvic(void);
static void LoggerHal_InitUart3(void);

static void LoggerHal_DmaStartNextTxTransfer(void);

void LoggerHal_Init(void)
{
    ASSERT(!IsInitialized);

    RingBuffer_Init(&TxDmaBuffer, DmaTxBuffer, LOGGER_HAL_TX_BUFFER_LEN);

    LoggerHal_InitGpio();
    LoggerHal_InitDma1Ch2Ch3();
    LoggerHal_InitUart3();
    LoggerHal_InitNvic();

    IsInitialized = true;
}

bool LoggerHal_IsInitialized(void)
{
    return IsInitialized;
}

void LoggerHal_SendString(uint8_t *p_string)
{
    ASSERT(p_string != NULL);

    LoggerHal_SendBuffer(p_string, strlen((char *)p_string));
}

void LoggerHal_SendBuffer(uint8_t *p_buff, size_t buff_len)
{
    ASSERT(p_buff != NULL);

    if (RingBuffer_DataLen(&TxDmaBuffer) + buff_len > LOGGER_HAL_TX_BUFFER_LEN)
    {
        return;
    }

    Atomic_CriticalEnter();

    if (!RingBuffer_QueueBytes(&TxDmaBuffer, p_buff, buff_len))
    {
        ASSERT(false);
    }

    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_2) == 0)
    {
        LoggerHal_DmaStartNextTxTransfer();
    }

    Atomic_CriticalExit();
}

void LoggerHal_SendByte(uint8_t byte)
{
    LoggerHal_SendBuffer(&byte, 1);
}

bool LoggerHal_ReadByte(uint8_t *p_byte)
{
    ASSERT(p_byte != NULL);

    static uint32_t dma_read_ptr = 0;

    if (LOGGER_HAL_RX_BUFFER_LEN - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3) != dma_read_ptr)
    {
        *p_byte = DmaRxBuffer[dma_read_ptr];
        dma_read_ptr++;
        if (dma_read_ptr == LOGGER_HAL_RX_BUFFER_LEN)
        {
            dma_read_ptr = 0;
        }
        return true;
    }

    return false;
}

void LoggerHal_Flush(void)
{
    // To be sure that during the flush all DMA transfer is completed and
    // any new transaction will not be started, disable DMA IRQ
    NVIC_DisableIRQ(DMA1_Channel2_IRQn);

    isFlushInProgress = true;

    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_2))
    {
        while (!LL_DMA_IsActiveFlag_TC2(DMA1))
        {
            // Wait until DMA transfer complete
        }
        LL_DMA_ClearFlag_TC2(DMA1);

        RingBuffer_IncrementRdIndex(&TxDmaBuffer, CurrentTxTransferLen);
        CurrentTxTransferLen = 0;

        // Disable DMA Channel TX
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
    }

    uint8_t tx_byte;
    bool    status;
    while (true)
    {
        Atomic_CriticalEnter();
        status = RingBuffer_DequeueByte(&TxDmaBuffer, &tx_byte);
        Atomic_CriticalExit();

        if (!status)
        {
            break;
        }

        // Wait for TXE flag to be raised
        while (!LL_USART_IsActiveFlag_TXE(USART3))
        {
        }

        // Write character in Transmit Data register
        LL_USART_TransmitData8(USART3, tx_byte);
    }

    isFlushInProgress = false;

    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

static void LoggerHal_InitGpio(void)
{
    // GPIO clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // GPIO initialization
    // PB10 -> USART3_TX
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin                 = LL_GPIO_PIN_10;
    gpio_init.Mode                = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType          = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull                = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOB, &gpio_init);

    // PB11 -> USART3_RX
    gpio_init.Pin  = LL_GPIO_PIN_11;
    gpio_init.Mode = LL_GPIO_MODE_INPUT;
    LL_GPIO_Init(GPIOB, &gpio_init);
}

static void LoggerHal_InitDma1Ch2Ch3(void)
{
    // DMA1 clock enable
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    // USART3_RX DMA1 CH3 configuration
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_ConfigAddresses(DMA1,
                           LL_DMA_CHANNEL_3,
                           LL_USART_DMA_GetRegAddr(USART3),
                           (uint32_t)DmaRxBuffer,
                           LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_3));

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, LOGGER_HAL_RX_BUFFER_LEN);

    // Enable DMA Channel RX
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);

    // USART3_TX DMA1 CH2 configuration
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_BYTE);

    // Enable DMA1 CH2 TX complete interrupts
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);
}

static void LoggerHal_InitNvic(void)
{
    NVIC_SetPriority(DMA1_Channel2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), DMA1_CH2_UART_TX_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

static void LoggerHal_InitUart3(void)
{
    // USART3 clock enable
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

    LL_USART_InitTypeDef usart_init = {0};

    usart_init.BaudRate            = LOGGER_HAL_BAUDRATE;
    usart_init.DataWidth           = LL_USART_DATAWIDTH_8B;
    usart_init.StopBits            = LL_USART_STOPBITS_1;
    usart_init.Parity              = LL_USART_PARITY_NONE;
    usart_init.TransferDirection   = LL_USART_DIRECTION_TX_RX;
    usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_init.OverSampling        = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART3, &usart_init);

    // Enable DMA Channel RX
    LL_USART_EnableDMAReq_RX(USART3);

    // Enable USART
    LL_USART_ConfigAsyncMode(USART3);
    LL_USART_Enable(USART3);
}

static void LoggerHal_DmaStartNextTxTransfer(void)
{
    if (isFlushInProgress)
    {
        return;
    }

    uint8_t *p_tx_begin_pointer = RingBuffer_GetMaxContinuousBuffer(&TxDmaBuffer, &CurrentTxTransferLen);
    if (CurrentTxTransferLen == 0)
    {
        return;
    }

    // Clear Global IRQ flag
    LL_DMA_ClearFlag_GI2(DMA1);

    // Disable DMA Channel TX
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

    // Disable DMA TX Interrupt
    LL_USART_DisableDMAReq_TX(USART3);

    LL_DMA_ConfigAddresses(DMA1,
                           LL_DMA_CHANNEL_2,
                           (uint32_t)p_tx_begin_pointer,
                           LL_USART_DMA_GetRegAddr(USART3),
                           LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2));

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, CurrentTxTransferLen);

    // Enable DMA TX Interrupt
    LL_USART_EnableDMAReq_TX(USART3);

    // Enable DMA Channel TX
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}

void DMA1_Channel2_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC2(DMA1))
    {
        LL_DMA_ClearFlag_TC2(DMA1);

        Atomic_CriticalEnter();

        RingBuffer_IncrementRdIndex(&TxDmaBuffer, CurrentTxTransferLen);
        CurrentTxTransferLen = 0;

        // Disable DMA Channel TX
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

        if (!RingBuffer_IsEmpty(&TxDmaBuffer))
        {
            LoggerHal_DmaStartNextTxTransfer();
        }

        Atomic_CriticalExit();
    }
}
