#include "LcdDrv.h"

#include <stddef.h>

#include "Assert.h"
#include "I2cHal.h"
#include "Log.h"
#include "Timestamp.h"
#include "Utils.h"

// PCF8574AT I2C address
#define LCD_DRV_PCF8574AT_ADDRESS 0x3F

// Pin mask
#define LCD_DRV_PIN_EN_MASK (1 << 2)
#define LCD_DRV_PIN_RW_MASK (1 << 1)
#define LCD_DRV_PIN_RS_MASK (1 << 0)
#define LCD_DRV_PIN_D4_MASK (1 << 4)
#define LCD_DRV_PIN_D5_MASK (1 << 5)
#define LCD_DRV_PIN_D6_MASK (1 << 6)
#define LCD_DRV_PIN_D7_MASK (1 << 7)
#define LCD_DRV_PIN_BAKLIGHT_MASK (1 << 3)

// LCD size
#define LCD_DRV_ROWS 4
#define LCD_DRV_COLUMNS 20

// LCD Commands
#define LCD_DRV_CLEARDISPLAY 0x01
#define LCD_DRV_RETURNHOME 0x02
#define LCD_DRV_ENTRYMODESET 0x04
#define LCD_DRV_DISPLAYCONTROL 0x08
#define LCD_DRV_CURSORSHIFT 0x10
#define LCD_DRV_FUNCTIONSET 0x20
#define LCD_DRV_SETCGRAMADDR 0x40
#define LCD_DRV_SETDDRAMADDR 0x80

// Flags for display entry mode
#define LCD_DRV_ENTRYRIGHT 0x00
#define LCD_DRV_ENTRYLEFT 0x02
#define LCD_DRV_ENTRYSHIFTINCREMENT 0x01
#define LCD_DRV_ENTRYSHIFTDECREMENT 0x00

// Flags for display on/off and cursor control
#define LCD_DRV_DISPLAYON 0x04
#define LCD_DRV_DISPLAYOFF 0x00
#define LCD_DRV_CURSORON 0x02
#define LCD_DRV_CURSOROFF 0x00
#define LCD_DRV_BLINKON 0x01
#define LCD_DRV_BLINKOFF 0x00

// Flags for display/cursor shift
#define LCD_DRV_DISPLAYMOVE 0x08
#define LCD_DRV_CURSORMOVE 0x00
#define LCD_DRV_MOVERIGHT 0x04
#define LCD_DRV_MOVELEFT 0x00

// Flags for function set
#define LCD_DRV_8BITMODE 0x10
#define LCD_DRV_4BITMODE 0x00
#define LCD_DRV_2LINE 0x08
#define LCD_DRV_1LINE 0x00
#define LCD_DRV_5x10DOTS 0x04
#define LCD_DRV_5x8DOTS 0x00

// COMMAND and DATA LCD Rs
#define LCD_DRV_COMMAND 0
#define LCD_DRV_DATA 1
#define LCD_DRV_FOUR_BITS 2

static uint8_t IsBacklightOnMask = 0;
static bool    IsInitialized     = false;

static void LcdDrv_SetBacklight(bool on);

static void LcdDrv_LcdInit(void);

static void LcdDrv_SetOutputPortValue(uint8_t port_value);

static void LcdDrv_LcdSend(uint8_t value, uint8_t mode);

static void LcdDrv_Write4bits(uint8_t value, uint8_t mode);

static void LcdDrv_PulseEnable(uint8_t data);

void LcdDrv_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("LcdDrv initialization");

    if (!I2cHal_IsInitialized())
    {
        I2cHal_Init();
    }

    if (!I2cHal_IsAvaliable(LCD_DRV_PCF8574AT_ADDRESS))
    {
        return;
    }

    LcdDrv_SetBacklight(true);
    LcdDrv_LcdInit();

    LcdDrv_SetCursor(0, 0);

    IsInitialized = true;
}

bool LcdDrv_IsInitialized(void)
{
    return IsInitialized;
}

void LcdDrv_SetCursor(uint8_t col, uint8_t row)
{
    if (!IsInitialized)
    {
        return;
    }

    const uint8_t row_offsets_16x5_lcd[] = {0x00, 0x40, 0x14, 0x54};

    if ((row >= LCD_DRV_ROWS) || (col >= LCD_DRV_COLUMNS))
    {
        return;
    }

    LcdDrv_LcdSend(LCD_DRV_SETDDRAMADDR | (col + row_offsets_16x5_lcd[row]), LCD_DRV_COMMAND);
}

void LcdDrv_Clear(void)
{
    if (!IsInitialized)
    {
        return;
    }

    LcdDrv_LcdSend(LCD_DRV_CLEARDISPLAY, LCD_DRV_COMMAND);

    // This command is time consuming so the delay is required
    // Ugly, blocking implementation
    Timestamp_DelayMs(3);
}

void LcdDrv_PrintStr(const char *p_str, size_t str_len)
{
    ASSERT(p_str != NULL);

    if (!IsInitialized)
    {
        return;
    }

    size_t i;
    for (i = 0; i < str_len; i++)
    {
        LcdDrv_LcdSend(p_str[i], LCD_DRV_DATA);
    }
}

static void LcdDrv_SetBacklight(bool on)
{
    if (on)
    {
        IsBacklightOnMask = LCD_DRV_PIN_BAKLIGHT_MASK;
    }
    else
    {
        IsBacklightOnMask = 0;
    }

    LcdDrv_SetOutputPortValue(IsBacklightOnMask);
}

static void LcdDrv_LcdInit(void)
{
    // Display startup time minimum 50ms
    Timestamp_DelayMs(50);

    // Set 4 bit mode, Special case of "Function Set"
    LcdDrv_LcdSend(0x03, LCD_DRV_FOUR_BITS);
    // Wait min 4.1ms
    Timestamp_DelayMs(5);

    // Second try
    LcdDrv_LcdSend(0x03, LCD_DRV_FOUR_BITS);
    // Wait min 100us
    Timestamp_DelayMs(1);

    // Third try
    LcdDrv_LcdSend(0x03, LCD_DRV_FOUR_BITS);
    // Wait min of 100us
    Timestamp_DelayMs(1);

    // Set to 4-bit interface
    LcdDrv_LcdSend(0x02, LCD_DRV_FOUR_BITS);
    // Wait min of 100us
    Timestamp_DelayMs(1);

    // Set lines, font size, etc.
    LcdDrv_LcdSend(LCD_DRV_FUNCTIONSET | LCD_DRV_4BITMODE | LCD_DRV_2LINE | LCD_DRV_5x8DOTS, LCD_DRV_COMMAND);
    // Wait 1ms
    Timestamp_DelayMs(1);

    // Turn the display on with no cursor or blinking default
    LcdDrv_LcdSend(LCD_DRV_DISPLAYCONTROL | LCD_DRV_DISPLAYON | LCD_DRV_CURSOROFF | LCD_DRV_BLINKOFF, LCD_DRV_COMMAND);

    // Clear the LCD
    LcdDrv_Clear();

    // Initialize to default text direction (for romance languages), set the entry mode
    LcdDrv_LcdSend(LCD_DRV_ENTRYMODESET | LCD_DRV_ENTRYLEFT | LCD_DRV_ENTRYSHIFTDECREMENT, LCD_DRV_COMMAND);
}

static void LcdDrv_SetOutputPortValue(uint8_t port_value)
{
    // The PCF8574AT has specific I2C interface where the register address is a data register
    struct I2cTransaction transaction = {
        .rw           = I2C_TRANSACTION_WRITE,
        .num_of_bytes = 0,
        .i2c_address  = LCD_DRV_PCF8574AT_ADDRESS,
        .reg_address  = port_value,
        .p_rw_buffer  = NULL,
        .cb           = NULL,
    };

    I2cHal_ProcessTransaction(&transaction);
}

static void LcdDrv_LcdSend(uint8_t value, uint8_t mode)
{
    if (mode == LCD_DRV_FOUR_BITS)
    {
        LcdDrv_Write4bits((value & 0x0F), LCD_DRV_COMMAND);
    }
    else
    {
        LcdDrv_Write4bits((value >> 4), mode);
        Timestamp_DelayMs(1);
        LcdDrv_Write4bits((value & 0x0F), mode);
    }
}

static void LcdDrv_Write4bits(uint8_t value, uint8_t mode)
{
    uint8_t pin_value = value << 4;

    if (mode == LCD_DRV_DATA)
    {
        mode = LCD_DRV_PIN_RS_MASK;
    }

    pin_value |= mode | IsBacklightOnMask;
    LcdDrv_PulseEnable(pin_value);
}

static void LcdDrv_PulseEnable(uint8_t data)
{
    LcdDrv_SetOutputPortValue(data | LCD_DRV_PIN_EN_MASK);
    Timestamp_DelayMs(1);
    LcdDrv_SetOutputPortValue(data & ~LCD_DRV_PIN_EN_MASK);
}
