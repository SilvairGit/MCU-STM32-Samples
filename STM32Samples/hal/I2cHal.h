#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum I2cTransactionRw
{
    I2C_TRANSACTION_WRITE,
    I2C_TRANSACTION_READ
};

struct I2cTransaction
{
    enum I2cTransactionRw rw;
    size_t                num_of_bytes;
    uint8_t               i2c_address;
    uint8_t               reg_address;
    uint8_t              *p_rw_buffer;
    void (*cb)(struct I2cTransaction *p_transaction);
};

void I2cHal_Init(void);

bool I2cHal_IsInitialized(void);

bool I2cHal_ProcessTransaction(struct I2cTransaction *p_transaction);

bool I2cHal_IsAvaliable(uint8_t address);

#endif
