#include "PCF8523Drv.h"

#include <stddef.h>

#include "Assert.h"
#include "I2cHal.h"
#include "Log.h"
#include "Utils.h"

#define PCF8523_DRV_ADDRESS 0x68

#define PCF8523_DRV_CONTROL_1 0x00
#define PCF8523_DRV_CONTROL_2 0x01
#define PCF8523_DRV_CONTROL_3 0x02
#define PCF8523_DRV_SECONDS 0x03
#define PCF8523_DRV_MINUTES 0x04
#define PCF8523_DRV_HOURS 0x05
#define PCF8523_DRV_DAYS 0x06
#define PCF8523_DRV_WEEKDAYS 0x07
#define PCF8523_DRV_MONTHS 0x08
#define PCF8523_DRV_YEARS 0x09
#define PCF8523_DRV_MINUTE_ALARM 0x0A
#define PCF8523_DRV_HOUR_ALARM 0x0B
#define PCF8523_DRV_DAY_ALARM 0x0C
#define PCF8523_DRV_WEEKDAY_ALARM 0x0D
#define PCF8523_DRV_OFFSET 0x0E
#define PCF8523_DRV_TMR_CLKOUT_CTRL 0x0F
#define PCF8523_DRV_TMR_A_FREQ_CTRL 0x10
#define PCF8523_DRV_TMR_A_REG 0x11
#define PCF8523_DRV_TMR_B_FREQ_CTRL 0x12
#define PCF8523_DRV_TMR_B_REG 0x13

#define PCF8523_DRV_CONTROL_1_CAP_SEL_BIT 7
#define PCF8523_DRV_CONTROL_1_T_BIT 6
#define PCF8523_DRV_CONTROL_1_STOP_BIT 5
#define PCF8523_DRV_CONTROL_1_SR_BIT 4
#define PCF8523_DRV_CONTROL_1_1224_BIT 3
#define PCF8523_DRV_CONTROL_1_SIE_BIT 2
#define PCF8523_DRV_CONTROL_1_AIE_BIT 1
#define PCF8523_DRV_CONTROL_1CIE_BIT 0

#define PCF8523_DRV_CONTROL_2_WTAF_BIT 7
#define PCF8523_DRV_CONTROL_2_CTAF_BIT 6
#define PCF8523_DRV_CONTROL_2_CTBF_BIT 5
#define PCF8523_DRV_CONTROL_2_SF_BIT 4
#define PCF8523_DRV_CONTROL_2_AF_BIT 3
#define PCF8523_DRV_CONTROL_2_WTAIE_BIT 2
#define PCF8523_DRV_CONTROL_2_CTAIE_BIT 1
#define PCF8523_DRV_CONTROL_2_CTBIE_BIT 0

#define PCF8523_DRV_CONTROL_3_PM2_BIT 7
#define PCF8523_DRV_CONTROL_3_PM1_BIT 6
#define PCF8523_DRV_CONTROL_3_PM0_BIT 5
#define PCF8523_DRV_CONTROL_3_BSF_BIT 3
#define PCF8523_DRV_CONTROL_3_BLF_BIT 2
#define PCF8523_DRV_CONTROL_3_BSIE_BIT 1
#define PCF8523_DRV_CONTROL_3_BLIE_BIT 0

#define PCF8523_DRV_SECONDS_OS_BIT 7
#define PCF8523_DRV_SECONDS_10_BIT 6
#define PCF8523_DRV_SECONDS_10_LENGTH 3
#define PCF8523_DRV_SECONDS_1_BIT 3
#define PCF8523_DRV_SECONDS_1_LENGTH 4

#define PCF8523_DRV_MINUTES_10_BIT 6
#define PCF8523_DRV_MINUTES_10_LENGTH 3
#define PCF8523_DRV_MINUTES_1_BIT 3
#define PCF8523_DRV_MINUTES_1_LENGTH 4

#define PCF8523_DRV_HOURS_MODE_BIT 3    // 0 = 24-hour mode, 1 = 12-hour mode
#define PCF8523_DRV_HOURS_AMPM_BIT 5    // 2nd HOURS_10 bit if in 24-hour mode
#define PCF8523_DRV_HOURS_10_BIT 4
#define PCF8523_DRV_HOURS_1_BIT 3
#define PCF8523_DRV_HOURS_1_LENGTH 4

#define PCF8523_DRV_WEEKDAYS_BIT 2
#define PCF8523_DRV_WEEKDAYS_LENGTH 3

#define PCF8523_DRV_DAYS_10_BIT 5
#define PCF8523_DRV_DAYS_10_LENGTH 2
#define PCF8523_DRV_DAYS_1_BIT 3
#define PCF8523_DRV_DAYS_1_LENGTH 4

#define PCF8523_DRV_MONTH_10_BIT 4
#define PCF8523_DRV_MONTH_1_BIT 3
#define PCF8523_DRV_MONTH_1_LENGTH 4

#define PCF8523_DRV_YEAR_10H_BIT 7
#define PCF8523_DRV_YEAR_10H_LENGTH 4
#define PCF8523_DRV_YEAR_1H_BIT 3
#define PCF8523_DRV_YEAR_1H_LENGTH 4

#define PCF8523_DRV_TMR_CLKOUT_CTRL_TAM_BIT 7
#define PCF8523_DRV_TMR_CLKOUT_CTRL_TBM_BIT 6
#define PCF8523_DRV_TMR_CLKOUT_CTRL_COF2_BIT 5
#define PCF8523_DRV_TMR_CLKOUT_CTRL_COF1_BIT 4
#define PCF8523_DRV_TMR_CLKOUT_CTRL_COF0_BIT 3
#define PCF8523_DRV_TMR_CLKOUT_CTRL_TAC1_BIT 2
#define PCF8523_DRV_TMR_CLKOUT_CTRL_TAC0_BIT 1
#define PCF8523_DRV_TMR_CLKOUT_CTRL_TBC_BIT 0

#define PCF8523_DRV_CONTROL_3_VALID_BIT_MASK 0xEF
#define PCF8523_DRV_CONTROL_3_RESET_DEFAULT_VALUE 0xE0

static bool IsInitialized = false;

static bool RtcReadReg(uint8_t address, uint8_t *p_data);
static bool RtcWriteReg(uint8_t address, uint8_t data);
static bool RtcReadRegBuff(uint8_t address, uint8_t *p_buf, uint8_t size);
static bool RtcWriteRegBuff(uint8_t address, uint8_t *p_buf, uint8_t size);
static void RtcStart(void);
static void ConfigureIntEverySecond(void);
static void ConfigureBatterySwitchOver(void);
static void ConfigureInternalCapacitors(void);

bool Pcf8523Drv_Init(void)
{
    if (IsInitialized)
    {
        return true;
    }

    LOG_D("Pcf8523Drv initialization");

    if (!I2cHal_IsInitialized())
    {
        I2cHal_Init();
    }

    if (!I2cHal_IsAvaliable(PCF8523_DRV_ADDRESS))
    {
        LOG_W("PCF8523 not present on I2C bus");
        return false;
    }

    uint8_t data   = 0;
    bool    status = RtcReadReg(PCF8523_DRV_TMR_B_FREQ_CTRL, &data);
    if ((data == __UINT8_MAX__) || !status)
    {
        LOG_W("RTC is not connected");

        return false;
    }

    RtcStart();

    ConfigureIntEverySecond();
    ConfigureBatterySwitchOver();
    ConfigureInternalCapacitors();
    IsInitialized = true;

    return true;
}

bool Pcf8523Drv_IsInitialized(void)
{
    return IsInitialized;
}

bool Pcf8523Drv_IsAvailable(void)
{
    return I2cHal_IsAvaliable(PCF8523_DRV_ADDRESS);
}

static bool RtcReadReg(uint8_t address, uint8_t *p_data)
{
    return RtcReadRegBuff(address, p_data, 1);
}

static bool RtcWriteReg(uint8_t address, uint8_t data)
{
    return RtcWriteRegBuff(address, &data, 1);
}

static bool RtcReadRegBuff(uint8_t address, uint8_t *p_buf, uint8_t size)
{
    struct I2cTransaction trans =
        {.rw = I2C_TRANSACTION_READ, .num_of_bytes = size, .i2c_address = PCF8523_DRV_ADDRESS, .reg_address = address, .p_rw_buffer = p_buf, .cb = NULL};

    return I2cHal_ProcessTransaction(&trans);
}

static bool RtcWriteRegBuff(uint8_t address, uint8_t *p_buf, uint8_t size)
{
    struct I2cTransaction trans =
        {.rw = I2C_TRANSACTION_WRITE, .num_of_bytes = size, .i2c_address = PCF8523_DRV_ADDRESS, .reg_address = address, .p_rw_buffer = p_buf, .cb = NULL};

    return I2cHal_ProcessTransaction(&trans);
}

void Pcf8523Drv_SetTime(struct Pcf8523Drv_TimeDate *p_time)
{
    if (!Pcf8523Drv_IsAvailable())
    {
        return;
    }
    ASSERT(p_time != NULL);

    uint8_t data_to_write[8];
    data_to_write[0] = Bin2bcd(p_time->seconds);
    data_to_write[1] = Bin2bcd(p_time->minute);
    data_to_write[2] = Bin2bcd(p_time->hour);
    data_to_write[3] = Bin2bcd(p_time->day);
    data_to_write[4] = Bin2bcd(0);
    data_to_write[5] = Bin2bcd(p_time->month);
    data_to_write[6] = Bin2bcd(p_time->year - 2000);
    data_to_write[7] = 0;

    (void)RtcWriteRegBuff(PCF8523_DRV_SECONDS, data_to_write, sizeof(data_to_write));
}

bool Pcf8523Drv_GetTime(struct Pcf8523Drv_TimeDate *p_time)
{
    ASSERT(p_time != NULL);

    uint8_t data_to_read[7];
    bool    status = RtcReadRegBuff(PCF8523_DRV_SECONDS, data_to_read, sizeof(data_to_read));
    if (!status)
    {
        return false;
    }

    p_time->milliseconds = 0;
    p_time->seconds      = Bcd2bin(data_to_read[0] & 0x7F);
    p_time->minute       = Bcd2bin(data_to_read[1]);
    p_time->hour         = Bcd2bin(data_to_read[2]);
    p_time->day          = Bcd2bin(data_to_read[3]);
    p_time->month        = Bcd2bin(data_to_read[5]);
    p_time->year         = Bcd2bin(data_to_read[6]) + 2000;
    return true;
}

bool Pcf8523Drv_IsResetState(void)
{
    uint8_t reg_val = 0;
    bool    status  = RtcReadReg(PCF8523_DRV_CONTROL_3, &reg_val);
    return (((reg_val & PCF8523_DRV_CONTROL_3_VALID_BIT_MASK) == PCF8523_DRV_CONTROL_3_RESET_DEFAULT_VALUE) && status);
}

static void RtcStart(void)
{
    uint8_t reg_val = 0;
    bool    status  = RtcReadReg(PCF8523_DRV_CONTROL_1, &reg_val);
    reg_val &= ~(1 << PCF8523_DRV_CONTROL_1_STOP_BIT);
    if (status)
    {
        (void)RtcWriteReg(PCF8523_DRV_CONTROL_1, reg_val);
    }
}

static void ConfigureIntEverySecond(void)
{
    uint8_t reg_val = 0;
    RtcReadReg(PCF8523_DRV_TMR_CLKOUT_CTRL, &reg_val);
    reg_val |= (1 << PCF8523_DRV_TMR_CLKOUT_CTRL_TAM_BIT) | (1 << PCF8523_DRV_TMR_CLKOUT_CTRL_COF2_BIT) | (1 << PCF8523_DRV_TMR_CLKOUT_CTRL_COF1_BIT) |
               (1 << PCF8523_DRV_TMR_CLKOUT_CTRL_COF0_BIT);
    (void)RtcWriteReg(PCF8523_DRV_TMR_CLKOUT_CTRL, reg_val);

    reg_val = 0;
    RtcReadReg(PCF8523_DRV_CONTROL_1, &reg_val);
    reg_val |= (1 << PCF8523_DRV_CONTROL_1_SIE_BIT);
    (void)RtcWriteReg(PCF8523_DRV_CONTROL_1, reg_val);
}

static void ConfigureBatterySwitchOver(void)
{
    uint8_t reg_val = 0;
    RtcReadReg(PCF8523_DRV_CONTROL_3, &reg_val);
    reg_val &= ~((1 << PCF8523_DRV_CONTROL_3_PM0_BIT) | (1 << PCF8523_DRV_CONTROL_3_PM1_BIT) | (1 << PCF8523_DRV_CONTROL_3_PM2_BIT));
    (void)RtcWriteReg(PCF8523_DRV_CONTROL_3, reg_val);
}

static void ConfigureInternalCapacitors(void)
{
    uint8_t reg_val = 0;
    RtcReadReg(PCF8523_DRV_CONTROL_1, &reg_val);
    reg_val |= (1 << PCF8523_DRV_CONTROL_1_CAP_SEL_BIT);
    (void)RtcWriteReg(PCF8523_DRV_CONTROL_1, reg_val);
}
