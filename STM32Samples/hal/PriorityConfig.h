#ifndef PRIORITY_CONFIG_H
#define PRIORITY_CONFIG_H

// List of all IRQ priority

// TickHal
#define SYSTICK_IRQ_PRIORITY 1

// UartHal
#define DMA1_CH7_UART_TX_IRQ_PRIORITY 7

// I2cHal
#define I2C1_IRQ_PRIORITY 9

// GpioHal
#define EXTI_IRQ_PRIORITY 11

// AdcHal
#define ADC1_IRQ_PRIORITY 13

// LoggerHal
#define DMA1_CH2_UART_TX_IRQ_PRIORITY 15

#endif
