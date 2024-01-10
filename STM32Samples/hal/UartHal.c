#include "UartHal.h"

#include <string.h>

#include "Assert.h"
#include "Atomic.h"
#include "Log.h"
#include "Platform.h"
#include "PriorityConfig.h"
#include "RingBuffer.h"

#define UART_HAL_BAUDRATE 57600

#define UART_HAL_RX_BUFFER_LEN 512
#define UART_HAL_TX_BUFFER_LEN 1024

static bool IsInitialized = false;

static volatile uint8_t DmaTxBuffer[UART_HAL_TX_BUFFER_LEN];
static volatile uint8_t DmaRxBuffer[UART_HAL_RX_BUFFER_LEN];

static volatile struct RingBuffer TxDmaBuffer;

static volatile uint16_t CurrentTxTransferLen = 0;
static volatile bool     isFlushInProgress    = false;

static void UartHal_InitGpio(void);
static void UartHal_InitDma1Ch6Ch7(void);
static void UartHal_InitNvic(void);
static void UartHal_InitUart2(void);

static void UartHal_DmaStartNextTxTransfer(void);

void UartHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("UartHal initialization");

    RingBuffer_Init(&TxDmaBuffer, DmaTxBuffer, UART_HAL_TX_BUFFER_LEN);

    UartHal_InitGpio();
    UartHal_InitDma1Ch6Ch7();
    UartHal_InitUart2();
    UartHal_InitNvic();

    IsInitialized = true;
}

bool UartHal_IsInitialized(void)
{
    return IsInitialized;
}

void UartHal_SendString(uint8_t *p_string)
{
    ASSERT(p_string != NULL);

    UartHal_SendBuffer(p_string, strlen((char *)p_string));
}

void UartHal_SendBuffer(uint8_t *p_buff, size_t buff_len)
{
    ASSERT(p_buff != NULL);

    Atomic_CriticalEnter();

    if (!RingBuffer_QueueBytes(&TxDmaBuffer, p_buff, buff_len))
    {
        ASSERT(false);
    }

    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7) == 0)
    {
        UartHal_DmaStartNextTxTransfer();
    }

    Atomic_CriticalExit();
}

void UartHal_SendByte(uint8_t byte)
{
    UartHal_SendBuffer(&byte, 1);
}

bool UartHal_ReadByte(uint8_t *p_byte)
{
    ASSERT(p_byte != NULL);

    static uint32_t dma_read_ptr = 0;

    if (UART_HAL_RX_BUFFER_LEN - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6) != dma_read_ptr)
    {
        *p_byte = DmaRxBuffer[dma_read_ptr];
        dma_read_ptr++;
        if (dma_read_ptr == UART_HAL_RX_BUFFER_LEN)
        {
            dma_read_ptr = 0;
        }
        return true;
    }

    return false;
}

void UartHal_Flush(void)
{
    // To be sure that during the flush all DMA transfer is completed and
    // any new transaction will not be started, disable DMA IRQ
    NVIC_DisableIRQ(DMA1_Channel7_IRQn);

    isFlushInProgress = true;

    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7))
    {
        while (!LL_DMA_IsActiveFlag_TC7(DMA1))
        {
            // Wait until DMA transfer complete
        }
        LL_DMA_ClearFlag_TC7(DMA1);

        RingBuffer_IncrementRdIndex(&TxDmaBuffer, CurrentTxTransferLen);
        CurrentTxTransferLen = 0;

        // Disable DMA Channel TX
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_7);
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
        while (!LL_USART_IsActiveFlag_TXE(USART2))
        {
        }

        // Write character in Transmit Data register
        LL_USART_TransmitData8(USART2, tx_byte);
    }

    isFlushInProgress = false;

    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

static void UartHal_InitGpio(void)
{
    // GPIO clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    // GPIO initialization
    // PA2 -> USART2_TX
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin                 = LL_GPIO_PIN_2;
    gpio_init.Mode                = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType          = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull                = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOA, &gpio_init);

    // PA3 -> USART2_RX
    gpio_init.Pin  = LL_GPIO_PIN_3;
    gpio_init.Mode = LL_GPIO_MODE_INPUT;
    LL_GPIO_Init(GPIOA, &gpio_init);
}

static void UartHal_InitDma1Ch6Ch7(void)
{
    // DMA1 clock enable
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    // USART2_RX DMA1 CH6 configuration
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_MEDIUM);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_ConfigAddresses(DMA1,
                           LL_DMA_CHANNEL_6,
                           LL_USART_DMA_GetRegAddr(USART2),
                           (uint32_t)DmaRxBuffer,
                           LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6));

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, UART_HAL_RX_BUFFER_LEN);

    // Enable DMA Channel RX
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);

    // USART2_TX DMA1 CH7 configuration
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_7, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_7, LL_DMA_PRIORITY_MEDIUM);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_7, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_7, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_7, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_7, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_7, LL_DMA_MDATAALIGN_BYTE);

    // Enable DMA1 CH7 TX complete interrupts
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_7);
}

static void UartHal_InitNvic(void)
{
    NVIC_SetPriority(DMA1_Channel7_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), DMA1_CH7_UART_TX_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

static void UartHal_InitUart2(void)
{
    // USART2 clock enable
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

    LL_USART_InitTypeDef usart_init = {0};

    usart_init.BaudRate            = UART_HAL_BAUDRATE;
    usart_init.DataWidth           = LL_USART_DATAWIDTH_8B;
    usart_init.StopBits            = LL_USART_STOPBITS_1;
    usart_init.Parity              = LL_USART_PARITY_NONE;
    usart_init.TransferDirection   = LL_USART_DIRECTION_TX_RX;
    usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_init.OverSampling        = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART2, &usart_init);

    // Enable DMA Channel RX
    LL_USART_EnableDMAReq_RX(USART2);

    // Enable USART
    LL_USART_ConfigAsyncMode(USART2);
    LL_USART_Enable(USART2);
}

static void UartHal_DmaStartNextTxTransfer(void)
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
    LL_DMA_ClearFlag_GI7(DMA1);

    // Disable DMA Channel TX
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_7);

    // Disable DMA TX Interrupt
    LL_USART_DisableDMAReq_TX(USART2);

    LL_DMA_ConfigAddresses(DMA1,
                           LL_DMA_CHANNEL_7,
                           (uint32_t)p_tx_begin_pointer,
                           LL_USART_DMA_GetRegAddr(USART2),
                           LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_7));

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_7, CurrentTxTransferLen);

    // Enable DMA TX Interrupt
    LL_USART_EnableDMAReq_TX(USART2);

    // Enable DMA Channel TX
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_7);
}

void DMA1_Channel7_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC7(DMA1))
    {
        LL_DMA_ClearFlag_TC7(DMA1);

        Atomic_CriticalEnter();

        RingBuffer_IncrementRdIndex(&TxDmaBuffer, CurrentTxTransferLen);
        CurrentTxTransferLen = 0;

        // Disable DMA Channel TX
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_7);

        if (!RingBuffer_IsEmpty(&TxDmaBuffer))
        {
            UartHal_DmaStartNextTxTransfer();
        }

        Atomic_CriticalExit();
    }
}
