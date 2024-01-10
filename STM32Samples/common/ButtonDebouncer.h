#ifndef BUTTON_DEBOUNCER_H
#define BUTTON_DEBOUNCER_H

#include <stdbool.h>
#include <stdint.h>

enum ButtonDebouncerStatus
{
    BUTTON_DEBOUNCER_STATUS_PRESSED,
    BUTTON_DEBOUNCER_STATUS_RELEASED,
    BUTTON_DEBOUNCER_STATUS_FALLING_EDGE,
    BUTTON_DEBOUNCER_STATUS_RISING_EDGE,
    BUTTON_DEBOUNCER_STATUS_UNSTABLE
};

enum ButtonDebouncerStatus ButtonDebouncer_Debounce(uint16_t *p_button_state, bool is_button_pressed);

#endif
