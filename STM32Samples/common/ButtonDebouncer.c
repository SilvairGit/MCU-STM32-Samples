#include "ButtonDebouncer.h"

#define BUTTON_DEBOUNCER_RELEASED_VALUE 0x0000
#define BUTTON_DEBOUNCER_FALLING_EDGE_VALUE 0x0001
#define BUTTON_DEBOUNCER_PRESSED_VALUE 0xFFFF
#define BUTTON_DEBOUNCER_RISING_EDGE_VALUE 0xFFFE

enum ButtonDebouncerStatus ButtonDebouncer_Debounce(uint16_t *p_button_state, bool is_button_pressed)
{
    *p_button_state = (*p_button_state) << 1;
    if (is_button_pressed)
    {
        *p_button_state |= 0x01;
    }

    switch (*p_button_state)
    {
        case BUTTON_DEBOUNCER_RELEASED_VALUE:
            return BUTTON_DEBOUNCER_STATUS_RELEASED;

        case BUTTON_DEBOUNCER_FALLING_EDGE_VALUE:
            return BUTTON_DEBOUNCER_STATUS_FALLING_EDGE;

        case BUTTON_DEBOUNCER_PRESSED_VALUE:
            return BUTTON_DEBOUNCER_STATUS_PRESSED;

        case BUTTON_DEBOUNCER_RISING_EDGE_VALUE:
            return BUTTON_DEBOUNCER_STATUS_RISING_EDGE;

        default:
            return BUTTON_DEBOUNCER_STATUS_UNSTABLE;
    }
}
