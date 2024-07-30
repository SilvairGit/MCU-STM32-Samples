#include "OnOffDeltaButtons.h"

#include "Config.h"
#include "Utils.h"

#if ENABLE_CLIENT

#include "ButtonDebouncer.h"
#include "GpioHal.h"
#include "Log.h"
#include "Mesh.h"
#include "Timestamp.h"

#define FSM_INFO(x) LOG_D("Button %s %s %d\n", (button_fsm->button_type ? "ON" : "OFF"), x, button_fsm->action);

enum MachineStates
{
    FSM_IDLE = 0,
    FSM_PRESS,
    FSM_LONG_PRESS,
    FSM_PRESS_OVERRIDE,
};

enum MachineActions
{
    NO_ACTION = 0,
    ACTION_PRESSED,
    ACTION_RELEASED
};

enum ButtonType
{
    BUTTON_OFF = 0,
    BUTTON_ON  = 1
};

#define SEQUENCE_A_TIMEOUT 400
#define SEQUENCE_C_TIMEOUT 100
#define SEQUENCE_B_TIMEOUT 250

#define BUTTON_DEBOUNCE_TIME_MS 20 /**< Defines buttons debounce time in milliseconds. */

#define SEQUENCE_A_NUMBER_OF_REPEATS 3 /**< Defines number of repeats for Generic OnOff message */
#define SEQUENCE_B_NUMBER_OF_REPEATS 3 /**< Defines number of repeats for first Generic Delta message  */
#define SEQUENCE_C_NUMBER_OF_REPEATS 0 /**< Defines number of repeats for Generic Deltas during dimming  */
#define SEQUENCE_D_NUMBER_OF_REPEATS 5 /**< Defines number of repeats for last Generic Delta message */

#define ON_OFF_TRANSITION_TIME_MS 1000 /**< Defines transition time */
#define ON_OFF_DELAY_TIME_MS 0         /**< Defines message delay in milliseconds */
#define REPEATS_INTERVAL_MS 50         /**< Defines interval between repeats in milliseconds */

#define DIMMING_STEP_VALUE 0xA00     /**< Defines Generic Delta minimal step on button long press */
#define DELTA_TRANSITION_TIME_MS 200 /**< Defines transition time */
#define DELTA_DELAY_TIME_MS 0        /**< Defines delay time */

#define GENERIC_OFF 0x00 /**< Defines Generic OnOff Off payload */
#define GENERIC_ON 0x01  /**< Defines Generic OnOff On payload */

#define SWITCH_BUTTONS_LOOP_PERIOD_MS 1

#define MAX_DELTA ((0x10000) / DIMMING_STEP_VALUE)

struct ButtonInstance
{
    enum GpioHalPin     pin_number;
    enum ButtonType     button_type;
    uint32_t            event_time;
    uint32_t            timeout;
    enum MachineStates  state;
    int16_t             delta_value_on_press;
    enum MachineActions action;
    uint8_t            *p_instance_idx;
    uint8_t            *p_onoff_tid;
    uint8_t            *p_delta_tid;
};

struct ButtonInstance Button1;
struct ButtonInstance Button2;
struct ButtonInstance Button3;
struct ButtonInstance Button4;

uint16_t Button1DebounceState;
uint16_t Button2DebounceState;
uint16_t Button3DebounceState;
uint16_t Button4DebounceState;

static void UpdateButtonStatus(struct ButtonInstance *p_button, uint16_t *p_button_state);
static void CheckButtonState(struct ButtonInstance *button_fsm, struct ButtonInstance *paired_button_fsm);
static void SendSequenceDGenericDelta(uint16_t instance_Index, uint32_t delta_value, uint8_t tid);
static void ClearButtonStates(struct ButtonInstance *button_fsm);

void OnOffDeltaButtons_Setup(uint8_t *p_button_pair_1_instance_index,
                             uint8_t *p_button_pair_2_instance_index,
                             uint8_t *p_on_off_pair_1_tid,
                             uint8_t *p_on_off_pair_2_tid,
                             uint8_t *p_delta_pair_1_tid,
                             uint8_t *p_delta_pair_2_tid)
{
    if (!GpioHal_IsInitialized())
    {
        GpioHal_Init();
    }

    Button1.p_instance_idx       = p_button_pair_1_instance_index;
    Button1.pin_number           = GPIO_HAL_PIN_SW1;
    Button1.state                = FSM_IDLE;
    Button1.delta_value_on_press = 0;
    Button1.button_type          = BUTTON_ON;
    Button1.p_onoff_tid          = p_on_off_pair_1_tid;
    Button1.p_delta_tid          = p_delta_pair_1_tid;
    ClearButtonStates(&Button1);
    GpioHal_PinMode(Button1.pin_number, GPIO_HAL_MODE_INPUT_PULLUP);

    Button2.p_instance_idx       = p_button_pair_1_instance_index;
    Button2.pin_number           = GPIO_HAL_PIN_SW2;
    Button2.state                = FSM_IDLE;
    Button2.delta_value_on_press = 0;
    Button2.button_type          = BUTTON_OFF;
    Button2.p_onoff_tid          = p_on_off_pair_1_tid;
    Button2.p_delta_tid          = p_delta_pair_1_tid;
    ClearButtonStates(&Button2);
    GpioHal_PinMode(Button2.pin_number, GPIO_HAL_MODE_INPUT_PULLUP);

    Button3.p_instance_idx       = p_button_pair_2_instance_index;
    Button3.pin_number           = GPIO_HAL_PIN_SW3;
    Button3.state                = FSM_IDLE;
    Button3.delta_value_on_press = 0;
    Button3.button_type          = BUTTON_ON;
    Button3.p_onoff_tid          = p_on_off_pair_2_tid;
    Button3.p_delta_tid          = p_delta_pair_2_tid;
    ClearButtonStates(&Button3);
    GpioHal_PinMode(Button3.pin_number, GPIO_HAL_MODE_INPUT_PULLUP);

    Button4.p_instance_idx       = p_button_pair_2_instance_index;
    Button4.pin_number           = GPIO_HAL_PIN_SW4;
    Button4.state                = FSM_IDLE;
    Button4.delta_value_on_press = 0;
    Button4.button_type          = BUTTON_OFF;
    Button4.p_onoff_tid          = p_on_off_pair_2_tid;
    Button4.p_delta_tid          = p_delta_pair_2_tid;
    ClearButtonStates(&Button4);
    GpioHal_PinMode(Button4.pin_number, GPIO_HAL_MODE_INPUT_PULLUP);
}

void OnOffDeltaButtons_Loop(void)
{
    static uint32_t last_loop_timestamp = 0;

    if (Timestamp_GetTimeElapsed(last_loop_timestamp, Timestamp_GetCurrent()) >= SWITCH_BUTTONS_LOOP_PERIOD_MS)
    {
        last_loop_timestamp = Timestamp_GetCurrent();

        UpdateButtonStatus(&Button1, &Button1DebounceState);
        UpdateButtonStatus(&Button2, &Button2DebounceState);
        UpdateButtonStatus(&Button3, &Button3DebounceState);
        UpdateButtonStatus(&Button4, &Button4DebounceState);

        CheckButtonState(&Button1, &Button2);
        CheckButtonState(&Button2, &Button1);
        CheckButtonState(&Button3, &Button4);
        CheckButtonState(&Button4, &Button3);
    }
}

static void UpdateButtonStatus(struct ButtonInstance *p_button, uint16_t *p_button_state)
{
    enum ButtonDebouncerStatus status = ButtonDebouncer_Debounce(p_button_state, !GpioHal_PinRead(p_button->pin_number));

    if (status == BUTTON_DEBOUNCER_STATUS_PRESSED)
    {
        p_button->action = ACTION_PRESSED;
    }
    else if (status == BUTTON_DEBOUNCER_STATUS_RELEASED)
    {
        p_button->action = ACTION_RELEASED;
    }
}

static void CheckButtonState(struct ButtonInstance *button_fsm, struct ButtonInstance *paired_button_fsm)
{
    switch (button_fsm->state)
    {
        case FSM_IDLE:
        {
            if (button_fsm->action == ACTION_PRESSED)
            {
                FSM_INFO("Button pressed");
                button_fsm->state      = FSM_PRESS;
                button_fsm->action     = NO_ACTION;
                button_fsm->event_time = Timestamp_GetCurrent();
                button_fsm->timeout    = SEQUENCE_A_TIMEOUT;
                if ((paired_button_fsm->state == FSM_PRESS) || (paired_button_fsm->state == FSM_LONG_PRESS))
                {
                    paired_button_fsm->state = FSM_PRESS_OVERRIDE;
                }
            }
            else if (button_fsm->action == ACTION_RELEASED)
            {
                ClearButtonStates(button_fsm);
            }
        }
        break;

        case FSM_PRESS:
        {
            if (button_fsm->action == ACTION_RELEASED)
            {
                FSM_INFO("Short press - turn on/off lightness");
                ClearButtonStates(button_fsm);
                Mesh_IncrementTid(button_fsm->p_onoff_tid);
                Mesh_SendGenericOnOffSetWithRepeatsInterval(*button_fsm->p_instance_idx,
                                                            (button_fsm->button_type ? GENERIC_ON : GENERIC_OFF),
                                                            ON_OFF_TRANSITION_TIME_MS,
                                                            ON_OFF_DELAY_TIME_MS,
                                                            SEQUENCE_A_NUMBER_OF_REPEATS,
                                                            REPEATS_INTERVAL_MS,
                                                            *button_fsm->p_onoff_tid);
                return;
            }

            if ((Timestamp_GetCurrent() - button_fsm->event_time) > button_fsm->timeout)
            {
                FSM_INFO("Long Press - dim lightness");
                button_fsm->state      = FSM_LONG_PRESS;
                button_fsm->event_time = Timestamp_GetCurrent();
                button_fsm->timeout    = SEQUENCE_B_TIMEOUT;

                button_fsm->button_type == BUTTON_ON ? button_fsm->delta_value_on_press++ : button_fsm->delta_value_on_press--;

                Mesh_IncrementTid(button_fsm->p_delta_tid);
                Mesh_SendGenericDeltaSetWithRepeatsInterval(*button_fsm->p_instance_idx,
                                                            DIMMING_STEP_VALUE * button_fsm->delta_value_on_press,
                                                            DELTA_TRANSITION_TIME_MS,
                                                            DELTA_DELAY_TIME_MS,
                                                            SEQUENCE_B_NUMBER_OF_REPEATS,
                                                            REPEATS_INTERVAL_MS,
                                                            *button_fsm->p_delta_tid);
            }
        }
        break;

        case FSM_LONG_PRESS:
        {
            if (Timestamp_GetCurrent() - button_fsm->event_time <= button_fsm->timeout)
            {
                return;
            }

            if (button_fsm->action == ACTION_RELEASED)
            {
                FSM_INFO("Released long press");
                int16_t delta_end_correction = DIMMING_STEP_VALUE * (Timestamp_GetCurrent() - button_fsm->event_time) / button_fsm->timeout;
                if (button_fsm->button_type == BUTTON_OFF)
                {
                    delta_end_correction = -delta_end_correction;
                }

                SendSequenceDGenericDelta(*button_fsm->p_instance_idx,
                                          DIMMING_STEP_VALUE * button_fsm->delta_value_on_press + delta_end_correction,
                                          *button_fsm->p_delta_tid);
                ClearButtonStates(button_fsm);

                return;
            }

            FSM_INFO("Long Press dimming");
            button_fsm->button_type == BUTTON_ON ? button_fsm->delta_value_on_press++ : button_fsm->delta_value_on_press--;

            if ((button_fsm->delta_value_on_press == MAX_DELTA) || (button_fsm->delta_value_on_press == -MAX_DELTA))
            {
                SendSequenceDGenericDelta(*button_fsm->p_instance_idx, DIMMING_STEP_VALUE * button_fsm->delta_value_on_press, *button_fsm->p_delta_tid);
                ClearButtonStates(button_fsm);
            }
            else
            {
                button_fsm->event_time = Timestamp_GetCurrent();
                button_fsm->timeout    = SEQUENCE_C_TIMEOUT;
                Mesh_SendGenericDeltaSet(*button_fsm->p_instance_idx,
                                         DIMMING_STEP_VALUE * button_fsm->delta_value_on_press,
                                         DELTA_TRANSITION_TIME_MS,
                                         DELTA_DELAY_TIME_MS,
                                         SEQUENCE_C_NUMBER_OF_REPEATS,
                                         *button_fsm->p_delta_tid);
            }
        }
        break;

        case FSM_PRESS_OVERRIDE:
        {
            if (button_fsm->action == ACTION_RELEASED)
            {
                ClearButtonStates(button_fsm);
            }
        }
        break;


        default:
            break;
    }
}

static void SendSequenceDGenericDelta(uint16_t instance_Index, uint32_t delta_value, uint8_t tid)
{
    size_t i;
    for (i = 0; i < SEQUENCE_D_NUMBER_OF_REPEATS; i++)
    {
        Mesh_SendGenericDeltaSetWithDispatchTime(instance_Index, delta_value, DELTA_TRANSITION_TIME_MS, DELTA_DELAY_TIME_MS, 50 * i, tid);
    }
}

static void ClearButtonStates(struct ButtonInstance *button_fsm)
{
    button_fsm->state                = FSM_IDLE;
    button_fsm->action               = NO_ACTION;
    button_fsm->event_time           = 0;
    button_fsm->timeout              = 0;
    button_fsm->delta_value_on_press = 0;
}

#else

void OnOffDeltaButtons_Setup(uint8_t *p_button_pair_1_instance_index,
                             uint8_t *p_button_pair_2_instance_index,
                             uint8_t *p_on_off_pair_1_tid,
                             uint8_t *p_on_off_pair_2_tid,
                             uint8_t *p_delta_pair_1_tid,
                             uint8_t *p_delta_pair_2_tid)
{
    UNUSED(p_button_pair_1_instance_index);
    UNUSED(p_button_pair_2_instance_index);
    UNUSED(p_on_off_pair_1_tid);
    UNUSED(p_on_off_pair_2_tid);
    UNUSED(p_delta_pair_1_tid);
    UNUSED(p_delta_pair_2_tid);
}

void OnOffDeltaButtons_Loop(void)
{
}

#endif
