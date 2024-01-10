#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include <stdbool.h>
#include <stdint.h>

#include "Platform.h"

enum GpioHalPin
{
    GPIO_HAL_PIN_LED_STATUS,
    GPIO_HAL_PIN_ENCODER_SW,
    GPIO_HAL_PIN_RTC_INT1,
    GPIO_HAL_PIN_PIR,
    GPIO_HAL_PIN_SW1,
    GPIO_HAL_PIN_SW2,
    GPIO_HAL_PIN_SW3,
    GPIO_HAL_PIN_SW4,
    GPIO_HAL_PIN_GPIO6,
    GPIO_HAL_PIN_GPIO7,
    GPIO_HAL_PIN_GPIO8,
    GPIO_HAL_PIN_LENGTH_MARKER
};

enum GpioHalGpioMode
{
    GPIO_HAL_MODE_INPUT,
    GPIO_HAL_MODE_OUTPUT,
    GPIO_HAL_MODE_INPUT_PULLUP,
    GPIO_HAL_MODE_LENGTH_MARKER
};

enum GpioHalGpioIrqEdge
{
    GPIO_HAL_IRQ_EDGE_ANY,
    GPIO_HAL_IRQ_EDGE_FALLING,
    GPIO_HAL_IRQ_EDGE_RISING,
    GPIO_HAL_IRQ_EDGE_LENGTH_MARKER
};

void GpioHal_Init(void);

bool GpioHal_IsInitialized(void);

void GpioHal_PinMode(enum GpioHalPin gpio, enum GpioHalGpioMode mode);

void GpioHal_SetPinIrq(enum GpioHalPin gpio, enum GpioHalGpioIrqEdge edge, void (*irq_cb)(void));

bool GpioHal_PinRead(enum GpioHalPin gpio);

void GpioHal_PinSet(enum GpioHalPin gpio, bool high);

void GpioHal_PinToggle(enum GpioHalPin gpio);

#endif
