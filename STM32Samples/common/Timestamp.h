#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdbool.h>
#include <stdint.h>

void Timestamp_Init(void);

bool Timestamp_IsInitialized(void);

/** @brief Get current Timestamp.
 *
 *  This value overflows approx. every 48 days.
 *
 *  @return Current timestamp in milliseconds.
 */
uint32_t Timestamp_GetCurrent(void);


/** @brief Less-than operator for timestamps.
 *
 * @note Values should not differ more than half of the range of UINT32_MAX.
 *
 *  @param [in] timestamp_lhs Timestamp on the left hand side
 *  @param [in] timestamp_rhs Timestamp on the right hand side
 *
 *  @return true if timestamp_lhs lies 'behind' timestamp_rhs on the clock half-face
 */
bool Timestamp_Compare(uint32_t timestamp_lhs, uint32_t timestamp_rhs);


/** @brief Get Time elapsed between two timestamps.
 *
 * @note timestamps values should not be bigger than Max Timestamp that can be obtained using Timestamp_GetMax()
 *
 *  @param [in] timestamp_earlier Older timestamp
 *  @param [in] timestamp_further Newer timestamp
 *
 *  @return Time elapsed between timestamps in milliseconds.
 */
uint32_t Timestamp_GetTimeElapsed(uint32_t timestamp_earlier, uint32_t timestamp_further);

/** @brief Delay function
 *
 * @note this function is a blocking function
 *
 *  @param [in] delay_time Deleay time in ms
 */

void Timestamp_DelayMs(uint32_t delay_time);

#endif
