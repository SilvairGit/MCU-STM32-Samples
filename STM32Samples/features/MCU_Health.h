#ifndef MCU_HEALTH_H
#define MCU_HEALTH_H

#include <stdbool.h>
#include <stdint.h>

/** Silvair Mesh Model Company Id */
#define SILVAIR_ID 0x0136u

// Setup health hardware
void MCU_Health_Setup(void);

/*  Send Health Set Fault Request
 *
 *  @param company_id      Company id
 *  @param fault_id        Fault id
 *  @param instance_idx    Instance index
 */
void MCU_Health_SendSetFaultRequest(uint16_t company_id, uint8_t fault_id, uint8_t instance_idx);

/*  Send Health Clear Fault Request
 *
 *  @param company_id      Company id
 *  @param fault_id        Fault id
 *  @param instance_idx    Instance index
 */
void MCU_Health_SendClearFaultRequest(uint16_t company_id, uint8_t fault_id, uint8_t instance_idx);

// Check if health test is in progress
bool MCU_Health_IsTestInProgress(void);

#endif
