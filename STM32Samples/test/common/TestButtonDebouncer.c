#include <stddef.h>

#include "ButtonDebouncer.h"
#include "unity.h"

#define UINT16_NUMBER_OF_BITS 16

void test_PressedDetection(void)
{
    uint16_t                   button1_state = 0;
    enum ButtonDebouncerStatus button_status;

    size_t i;
    for (i = 0; i < UINT16_NUMBER_OF_BITS; i++)
    {
        button_status = ButtonDebouncer_Debounce(&button1_state, true);

        if ((i != 0) && (i != UINT16_NUMBER_OF_BITS - 1))
        {
            TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_UNSTABLE);
        }
    }

    TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_PRESSED);
}

void test_ReleasedDetection(void)
{
    uint16_t                   button1_state = 0xFFFF;
    enum ButtonDebouncerStatus button_status;

    size_t i;
    for (i = 0; i < UINT16_NUMBER_OF_BITS; i++)
    {
        button_status = ButtonDebouncer_Debounce(&button1_state, false);

        if ((i != 0) && (i != UINT16_NUMBER_OF_BITS - 1))
        {
            TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_UNSTABLE);
        }
    }

    TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_RELEASED);
}

void test_FallingEdgeDetection(void)
{
    uint16_t                   button1_state = 0;
    enum ButtonDebouncerStatus button_status;

    size_t i;
    for (i = 0; i < UINT16_NUMBER_OF_BITS; i++)
    {
        button_status = ButtonDebouncer_Debounce(&button1_state, false);
    }

    button_status = ButtonDebouncer_Debounce(&button1_state, true);

    TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_FALLING_EDGE);
}

void test_RisingEdgeDetection(void)
{
    uint16_t                   button1_state = 0;
    enum ButtonDebouncerStatus button_status;

    size_t i;
    for (i = 0; i < UINT16_NUMBER_OF_BITS; i++)
    {
        button_status = ButtonDebouncer_Debounce(&button1_state, true);
    }

    button_status = ButtonDebouncer_Debounce(&button1_state, false);

    TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_RISING_EDGE);
}


void test_UnstableDetection(void)
{
    uint16_t                   button1_state = 0;
    enum ButtonDebouncerStatus button_status;

    button_status = ButtonDebouncer_Debounce(&button1_state, false);
    button_status = ButtonDebouncer_Debounce(&button1_state, true);
    button_status = ButtonDebouncer_Debounce(&button1_state, true);
    button_status = ButtonDebouncer_Debounce(&button1_state, false);

    TEST_ASSERT_EQUAL_INT(button_status, BUTTON_DEBOUNCER_STATUS_UNSTABLE);
}
