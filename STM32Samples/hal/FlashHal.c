#include "FlashHal.h"

#include <stddef.h>

#include "Assert.h"
#include "AtomicHal.h"
#include "Log.h"
#include "Platform.h"
#include "Utils.h"

#define FLASH_HAL_PAGE_ERASE_TIMEOUT 0x00000FFF
#define FLASH_HAL_HALF_WORD_PROG_TIMEOUT 0x0000000F
#define FLASH_HAL_PAGE_SIZE 0x400
#define FLASH_HAL_WORD_SIZE 4
#define FLASH_HAL_BLANK_WORD 0xFFFFFFFF

#define FLASH_HAL_FLASH_START_ADDRESS ((uint32_t)(&_flash_start))
#define FLASH_HAL_FLASH_END_ADDRESS ((uint32_t)(&_flash_end))
#define FLASH_HAL_FLASH_SIZE ((uint32_t)(&_flash_size))

#define FLASH_HAL_IMAGE_START_ADDRESS ((uint32_t)(&_image_start))
#define FLASH_HAL_IMAGE_END_ADDRESS ((uint32_t)(&_image_end))
#define FLASH_HAL_IMAGE_SIZE ((uint32_t)(&_image_size))

enum FlashHalStatus
{
    FLASH_HAL_STATUS_BUSY,
    FLASH_HAL_STAUTS_ERROR_PG,
    FLASH_HAL_STATUS_ERROR_WRP,
    FLASH_HAL_STATUS_COMPLETE,
    FLASH_HAL_STATUS_TIMEOUT
};

RAM_FUNCTION static enum FlashHalStatus FlashHal_GetStatus(void);

RAM_FUNCTION static enum FlashHalStatus FlashHal_WaitForLastOperation(uint32_t timeout);

RAM_FUNCTION static enum FlashHalStatus FlashHal_ErasePage(uint32_t page_address);

RAM_FUNCTION static enum FlashHalStatus FlashHal_ProgramWord(uint32_t address, uint32_t data);

RAM_FUNCTION static void FlashHal_Unlock(void);

RAM_FUNCTION static void FlashHal_Lock(void);

RAM_FUNCTION static void FlashHal_BlockingDelay(void);

static bool FlashHal_CheckIsSpaceBlank(void);

extern uint32_t _flash_start;
extern uint32_t _flash_end;
extern uint32_t _flash_size;

extern uint32_t _image_start;
extern uint32_t _image_end;
extern uint32_t _image_size;

static bool IsInitialized = false;

void FlashHal_Init(void)
{
    ASSERT(!IsInitialized);

    LOG_D("FlashHal initialization");

    // Erase only if not blank. Erase take more than 100ms so it is better to clean Space during startup than blocking
    // code execution for more than 100ms during normal run run. It is important for DFU where erase is call
    FlashHal_EraseSpace();

    IsInitialized = true;
}

bool FlashHal_IsInitialized(void)
{
    return IsInitialized;
}

size_t FlashHal_GetSpaceSize(void)
{
    return FLASH_HAL_FLASH_END_ADDRESS - FlashHal_GetSpaceAddress();
}

uint32_t FlashHal_GetSpaceAddress(void)
{
    // Align space address to page size
    if (FLASH_HAL_IMAGE_END_ADDRESS % FLASH_HAL_PAGE_SIZE == 0)
    {
        return FLASH_HAL_IMAGE_END_ADDRESS;
    }

    return (FLASH_HAL_IMAGE_END_ADDRESS + FLASH_HAL_PAGE_SIZE - (FLASH_HAL_IMAGE_END_ADDRESS % FLASH_HAL_PAGE_SIZE));
}

bool FlashHal_EraseSpace(void)
{
    if (FlashHal_CheckIsSpaceBlank())
    {
        return true;
    }

    uint32_t space_page_addess = FlashHal_GetSpaceAddress();

    FlashHal_Unlock();

    // Erase space page by page
    while (space_page_addess < FLASH_HAL_FLASH_END_ADDRESS)
    {
        enum FlashHalStatus status = FlashHal_ErasePage(space_page_addess);
        ASSERT(status == FLASH_HAL_STATUS_COMPLETE);

        space_page_addess += FLASH_HAL_PAGE_SIZE;
    }

    FlashHal_Lock();

    return true;
}

bool FlashHal_SaveToFlash(uint32_t address, const uint32_t *p_src, uint32_t num_of_words)
{
    ASSERT((address >= FLASH_HAL_FLASH_START_ADDRESS) && (address < FLASH_HAL_FLASH_END_ADDRESS) && (p_src != NULL));

    uint32_t flash_end_addr = address + num_of_words * FLASH_HAL_WORD_SIZE;

    ASSERT((flash_end_addr >= FLASH_HAL_FLASH_START_ADDRESS) && (flash_end_addr <= FLASH_HAL_FLASH_END_ADDRESS));

    FlashHal_Unlock();

    // Program the user Flash area word by word
    while (address < flash_end_addr)
    {
        enum FlashHalStatus status = FlashHal_ProgramWord(address, *((uint32_t *)p_src));
        ASSERT(status == FLASH_HAL_STATUS_COMPLETE);

        address += FLASH_HAL_WORD_SIZE;
        p_src++;
    }

    FlashHal_Lock();

    return true;
}

bool FlashHal_UpdateFirmware(uint32_t num_of_words)
{
    uint32_t src_address = FlashHal_GetSpaceAddress();
    uint32_t dst_address = FLASH_HAL_FLASH_START_ADDRESS;

    AtomicHal_IrqDisable();

    FlashHal_Unlock();

    size_t i;
    for (i = 0; i < num_of_words; i++)
    {
        if (dst_address % FLASH_HAL_PAGE_SIZE == 0)
        {
            FlashHal_ErasePage(dst_address);
        }

        FlashHal_ProgramWord(dst_address, *((uint32_t *)src_address));

        src_address += FLASH_HAL_WORD_SIZE;
        dst_address += FLASH_HAL_WORD_SIZE;
    }

    FlashHal_Lock();

    // This function is static inline so it is push to RAM automatically
    NVIC_SystemReset();

    return true;
}

static enum FlashHalStatus FlashHal_GetStatus(void)
{
    if ((FLASH->SR & FLASH_SR_BSY) == FLASH_SR_BSY)
    {
        return FLASH_HAL_STATUS_BUSY;
    }

    if ((FLASH->SR & FLASH_SR_PGERR) != 0)
    {
        return FLASH_HAL_STAUTS_ERROR_PG;
    }

    if ((FLASH->SR & FLASH_SR_WRPRTERR) != 0)
    {
        return FLASH_HAL_STATUS_ERROR_WRP;
    }

    return FLASH_HAL_STATUS_COMPLETE;
}

static enum FlashHalStatus FlashHal_WaitForLastOperation(uint32_t timeout)
{
    enum FlashHalStatus status = FlashHal_GetStatus();

    while ((status == FLASH_HAL_STATUS_BUSY) && (timeout != 0))
    {
        // This blocking delay cannot base at Tick because the Tick implementation
        // can be erase when this part of code is used
        FlashHal_BlockingDelay();

        status = FlashHal_GetStatus();

        timeout--;
    }

    if (timeout == 0)
    {
        return FLASH_HAL_STATUS_TIMEOUT;
    }

    return status;
}

static enum FlashHalStatus FlashHal_ErasePage(uint32_t page_address)
{
    // Assert cannot be called here because the Flash containing Assert code can be erased

    enum FlashHalStatus status = FlashHal_WaitForLastOperation(FLASH_HAL_PAGE_ERASE_TIMEOUT);

    if (status == FLASH_HAL_STATUS_COMPLETE)
    {
        FLASH->CR |= FLASH_CR_PER;
        FLASH->AR = page_address;
        FLASH->CR |= FLASH_CR_STRT;

        status = FlashHal_WaitForLastOperation(FLASH_HAL_PAGE_ERASE_TIMEOUT);

        FLASH->CR &= ~FLASH_CR_PER;
    }

    return status;
}

static enum FlashHalStatus FlashHal_ProgramWord(uint32_t address, uint32_t data)
{
    // Assert cannot be called here because the Flash containing Assert code can be erased

    enum FlashHalStatus status = FlashHal_WaitForLastOperation(FLASH_HAL_PAGE_ERASE_TIMEOUT);

    if (status == FLASH_HAL_STATUS_COMPLETE)
    {
        FLASH->CR |= FLASH_CR_PG;
        *(uint16_t *)address = (uint16_t)data;

        status = FlashHal_WaitForLastOperation(FLASH_HAL_HALF_WORD_PROG_TIMEOUT);
        if (status == FLASH_HAL_STATUS_COMPLETE)
        {
            uint32_t tmp_address     = address + 2;
            *(uint16_t *)tmp_address = data >> 16;

            status = FlashHal_WaitForLastOperation(FLASH_HAL_HALF_WORD_PROG_TIMEOUT);
            FLASH->CR &= ~FLASH_CR_PG;
        }
        else
        {
            FLASH->CR &= ~FLASH_CR_PG;
        }
    }

    return status;
}

static void FlashHal_Unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != RESET)
    {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void FlashHal_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void FlashHal_BlockingDelay(void)
{
    volatile size_t i = 0;
    for (i = 0; i < 0xFF; i++)
    {
        // Do nothing
    }
}

static bool FlashHal_CheckIsSpaceBlank(void)
{
    uint32_t space_page_addess = FlashHal_GetSpaceAddress();

    while (space_page_addess < FLASH_HAL_FLASH_END_ADDRESS)
    {
        if (*(uint32_t *)space_page_addess != FLASH_HAL_BLANK_WORD)
        {
            return false;
        }

        space_page_addess += sizeof(uint32_t);
    }
    return true;
}
