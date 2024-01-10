#ifndef ONOFF_DELTA_BUTTONS_H
#define ONOFF_DELTA_BUTTONS_H

#include <stdint.h>

// Setup the on/off delta buttons
void OnOffDeltaButtons_Setup(uint8_t *p_button_pair_1_instance_index,
                             uint8_t *p_button_pair_2_instance_index,
                             uint8_t *p_on_off_pair_1_tid,
                             uint8_t *p_on_off_pair_2_tid,
                             uint8_t *p_delta_pair_1_tid,
                             uint8_t *p_delta_pair_2_tid);
// Loop the on/off delta buttons
void OnOffDeltaButtons_Loop(void);

#endif /* ONOFF_DELTA_BUTTONS_H */
