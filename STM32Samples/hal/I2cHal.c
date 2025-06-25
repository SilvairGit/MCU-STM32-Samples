#include "I2cHal.h"

#include "Assert.h"
#include "Log.h"
#include "Platform.h"
#include "PriorityConfig.h"
#include "Timestamp.h"

#define I2C_HAL_CLOCK_SPEED_HZ 400000
#define I2C_HAL_REQUEST_WRITE 0x00
#define I2C_HAL_REQUEST_READ 0x01

#define I2C_HAL_WAIT_FOR_ADDRESS_TIEMOUT_MS 10

static bool IsInitialized = false;

static volatile struct I2cTransaction *ActiveTransaction        = NULL;
static volatile size_t                 ActiveTransactionDataCnt = 0;
static volatile bool                   IsTransactionInProgress  = false;
static volatile bool                   IsAddressSent            = false;

static void I2cHal_InitGpioAndErrata(void);
static void I2cHal_InitI2c1(void);
static void I2cHal_InitNvic(void);
static void I2cHal_StartTransaction(struct I2cTransaction *p_transaction);


void I2cHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("I2cHal initialization");

    I2cHal_InitGpioAndErrata();
    I2cHal_InitI2c1();
    I2cHal_InitNvic();

    IsInitialized = true;
}

bool I2cHal_IsInitialized(void)
{
    return IsInitialized;
}

bool I2cHal_ProcessTransaction(struct I2cTransaction *p_transaction)
{
    if (IsTransactionInProgress)
    {
        LOG_W("I2C transaction dropped");
        return false;
    }

    I2cHal_StartTransaction(p_transaction);

    while (IsTransactionInProgress)
    {
        // Do nothing
    }
    return true;
}

bool I2cHal_IsAvaliable(uint8_t address)
{
    static struct I2cTransaction transaction = {
        .rw           = I2C_TRANSACTION_WRITE,
        .num_of_bytes = 0,
        .reg_address  = 0,
        .p_rw_buffer  = NULL,
        .cb           = NULL,
    };

    transaction.i2c_address = address;

    I2cHal_StartTransaction(&transaction);

    uint32_t transaction_time_start = Timestamp_GetCurrent();
    while (IsTransactionInProgress)
    {
        if (Timestamp_GetTimeElapsed(transaction_time_start, Timestamp_GetCurrent()) > I2C_HAL_WAIT_FOR_ADDRESS_TIEMOUT_MS)
        {
            IsTransactionInProgress = false;
            return false;
        }
    }

    return true;
}

static void I2cHal_InitGpioAndErrata(void)
{
    // GPIO clock enable
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    // I2C1 clock enable
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    // STM32F101x8/B, STM32F102x8/B and STM32F103x8/B medium-density device errata
    // 2.8.7 I2C analog filter may provide wrong value, locking BUSY flag and preventing master mode entry

    // 1.  Disable the I2C peripheral by clearing the PE bit in I2Cx_CR1 register
    LL_I2C_Disable(I2C1);

    // GPIO Configuration
    // PB8 -> I2C1_SCL
    // PB9 -> I2C1_SDA

    // 2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    LL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin                 = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    gpio_init.Mode                = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed               = LL_GPIO_SPEED_FREQ_HIGH;
    gpio_init.OutputType          = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_init.Pull                = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOB, &gpio_init);

    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);

    // 3. Check SCL and SDA High level in GPIOx_IDR.
    if (LL_GPIO_IsOutputPinSet(GPIOB, LL_GPIO_PIN_8) == 0)
    {
        ASSERT(false);
    }
    if (LL_GPIO_IsOutputPinSet(GPIOB, LL_GPIO_PIN_9) == 0)
    {
        ASSERT(false);
    }

    // 4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_9);

    // 5. Check SDA Low level in GPIOx_IDR.
    if (LL_GPIO_IsOutputPinSet(GPIOB, LL_GPIO_PIN_9))
    {
        ASSERT(false);
    }
    // 6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR)
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_8);

    // 7. Check SCL Low level in GPIOx_IDR.
    if (LL_GPIO_IsOutputPinSet(GPIOB, LL_GPIO_PIN_8))
    {
        ASSERT(false);
    }

    // 8. Configure the SCL I/O as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8);

    // 9. Check SCL High level in GPIOx_IDR.
    if (LL_GPIO_IsOutputPinSet(GPIOB, LL_GPIO_PIN_8) == 0)
    {
        ASSERT(false);
    }

    // 10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);

    // 11. Check SDA High level in GPIOx_IDR.
    if (LL_GPIO_IsOutputPinSet(GPIOB, LL_GPIO_PIN_9) == 0)
    {
        ASSERT(false);
    }

    // 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    LL_GPIO_Init(GPIOB, &gpio_init);

    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);

    LL_GPIO_AF_EnableRemap_I2C1();

    // 13. Set SWRST bit in I2Cx_CR1 register.
    LL_I2C_EnableReset(I2C1);

    // 14. Clear SWRST bit in I2Cx_CR1 register.
    LL_I2C_DisableReset(I2C1);

    // 15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register
    // Done in I2cHal_InitI2c1
}

static void I2cHal_InitI2c1(void)
{
    LL_I2C_EnableClockStretching(I2C1);

    LL_I2C_InitTypeDef i2c_init = {0};
    i2c_init.PeripheralMode     = LL_I2C_MODE_I2C;
    i2c_init.ClockSpeed         = I2C_HAL_CLOCK_SPEED_HZ;
    i2c_init.DutyCycle          = LL_I2C_DUTYCYCLE_16_9;
    i2c_init.OwnAddress1        = 0;
    i2c_init.TypeAcknowledge    = LL_I2C_ACK;
    i2c_init.OwnAddrSize        = LL_I2C_OWNADDRESS1_7BIT;
    LL_I2C_Init(I2C1, &i2c_init);

    LL_I2C_EnableIT_EVT(I2C1);
}

static void I2cHal_InitNvic(void)
{
    NVIC_SetPriority(I2C1_EV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), I2C1_IRQ_PRIORITY, 0));
    NVIC_EnableIRQ(I2C1_EV_IRQn);
}

void I2C1_EV_IRQHandler(void)
{
    // Follow RM0008
    // Figure 273. Transfer sequence diagram for master transmitter
    // Figure 274. Method 1: transfer sequence diagram for master receiver
    // Figure 275. Method 2: transfer sequence diagram for master receiver when N>2

    // Check SB flag value in ISR register
    if (LL_I2C_IsActiveFlag_SB(I2C1))
    {
        if (IsAddressSent)
        {
            // For master Read, second address is always Read
            // Send 7-Bit address with a a Read request
            LL_I2C_TransmitData8(I2C1, (ActiveTransaction->i2c_address << 1) | I2C_HAL_REQUEST_READ);
        }
        else
        {
            // For master Read and Write the first address is always Write
            // Send 7-Bit address with a a Write request
            LL_I2C_TransmitData8(I2C1, (ActiveTransaction->i2c_address << 1) | I2C_HAL_REQUEST_WRITE);
        }
        return;
    }

    // Check ADDR flag value in ISR register - if true, slave is present on the bus and confirm address
    if (LL_I2C_IsActiveFlag_ADDR(I2C1))
    {
        // Clear ADDR flag
        LL_I2C_ClearFlag_ADDR(I2C1);
        // Enable TXE interrupt
        LL_I2C_EnableIT_BUF(I2C1);

        // If the number of data to Read is 1, the NACK and STOP must be set immediately after slave address
        if (IsAddressSent && (ActiveTransaction->num_of_bytes == 1))
        {
            LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);

            // Disable TXE and wait for BFT interrupt
            LL_I2C_GenerateStopCondition(I2C1);
        }

        return;
    }

    // Check if TXE and if TXE interrupt is enable
    if (LL_I2C_IsActiveFlag_TXE(I2C1) && LL_I2C_IsEnabledIT_BUF(I2C1))
    {
        // Send register address only once
        if (IsAddressSent == false)
        {
            IsAddressSent = true;

            // Check if there is any data to Write or Read or ...
            // If there are data to read, disable TXE interrupt(wait for RXNE interrupt) and generate Restart to send start Read sequence
            if ((ActiveTransaction->num_of_bytes == 0) || (ActiveTransaction->rw == I2C_TRANSACTION_READ))
            {
                // If there is no data, disable TXE interrupt and wait for BFT interrupt
                LL_I2C_DisableIT_BUF(I2C1);

                // Send register address
                LL_I2C_TransmitData8(I2C1, ActiveTransaction->reg_address);
                return;
            }

            // Send register address in case of Write sequence
            LL_I2C_TransmitData8(I2C1, ActiveTransaction->reg_address);
            return;
        }

        // Below section is responsible for Write sequence
        if (ActiveTransaction->rw == I2C_TRANSACTION_WRITE)
        {
            if (ActiveTransactionDataCnt == ActiveTransaction->num_of_bytes - 1)
            {
                // It the last byte is transmit, disable TXE interrupt and wait for BFT interrupt
                LL_I2C_DisableIT_BUF(I2C1);

                // Send next data byte
                LL_I2C_TransmitData8(I2C1, ActiveTransaction->p_rw_buffer[ActiveTransactionDataCnt]);
            }
            else
            {
                // In other case just transmit next byte
                LL_I2C_TransmitData8(I2C1, ActiveTransaction->p_rw_buffer[ActiveTransactionDataCnt]);
            }

            ActiveTransactionDataCnt++;
        }
        return;
    }

    // Receive data
    if (LL_I2C_IsActiveFlag_RXNE(I2C1))
    {
        ActiveTransaction->p_rw_buffer[ActiveTransactionDataCnt] = LL_I2C_ReceiveData8(I2C1);
        ActiveTransactionDataCnt++;

        // Schedule NACK and STOP before last byte
        if (ActiveTransactionDataCnt == ActiveTransaction->num_of_bytes - 1)
        {
            LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);
            LL_I2C_GenerateStopCondition(I2C1);
        }
        else if (ActiveTransactionDataCnt == ActiveTransaction->num_of_bytes)
        {
            // Finish transaction after receiving the last bytes
            if (ActiveTransaction->cb != NULL)
            {
                ActiveTransaction->cb(ActiveTransaction);
            }

            IsTransactionInProgress = false;
        }
        return;
    }

    // Wait for the last data transmitted and finish transaction
    if (LL_I2C_IsActiveFlag_BTF(I2C1) && LL_I2C_IsActiveFlag_TXE(I2C1))
    {
        // If there are data to read generate Restart to start Read sequence
        if ((ActiveTransaction->rw == I2C_TRANSACTION_READ) && (ActiveTransaction->num_of_bytes > 0))
        {
            LL_I2C_GenerateStartCondition(I2C1);
            return;
        }

        // Generate stop condition
        LL_I2C_GenerateStopCondition(I2C1);

        // Wait until stop clear the BTF bit
        while (LL_I2C_IsActiveFlag_BTF(I2C1))
        {
            // Do nothing
        }

        // Finish transaction when the num_of_bytes == 0 or all data was already transfered
        if (ActiveTransaction->cb != NULL)
        {
            ActiveTransaction->cb(ActiveTransaction);
        }

        IsTransactionInProgress = false;

        return;
    }
}

static void I2cHal_StartTransaction(struct I2cTransaction *p_transaction)
{
    ASSERT((p_transaction != NULL) && (IsTransactionInProgress == false));

    IsTransactionInProgress  = true;
    ActiveTransactionDataCnt = 0;
    ActiveTransaction        = p_transaction;
    IsAddressSent            = false;

    LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
    LL_I2C_GenerateStartCondition(I2C1);
}
