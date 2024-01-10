#ifndef TAI_LOCAL_TIME_CONVERTER_H
#define TAI_LOCAL_TIME_CONVERTER_H

#include <stdint.h>

#define TIME_TAI_SECONDS_TIME_UNKNOWN (0x0000000000)
#define TIME_TAI_SECONDS_TIME_MAX (0xFFFFFFFFFF)

#define TIME_TAI_UTC_DELTA_MIN (-255)
#define TIME_TAI_UTC_DELTA_MAX (32512)
#define TIME_TAI_UTC_DELTA_STATE_TO_MESH(tai_utc_delta) ((tai_utc_delta)-TIME_TAI_UTC_DELTA_MIN)
#define TIME_TAI_UTC_DELTA_STATE_TO_SEC(tai_utc_delta) ((tai_utc_delta) + TIME_TAI_UTC_DELTA_MIN)

#define TIME_ZONE_MIN_IN_UNIT 15
#define TIME_ZONE_OFFSET_MIN (-64 * TIME_ZONE_MIN_IN_UNIT)
#define TIME_ZONE_OFFSET_MAX (191 * TIME_ZONE_MIN_IN_UNIT
#define TIME_ZONE_OFFSET_STATE_TO_MESH(tz) (((tz)-TIME_ZONE_OFFSET_MIN) / TIME_ZONE_MIN_IN_UNIT)
#define TIME_ZONE_OFFSET_STATE_TO_MIN(tz) ((tz)*TIME_ZONE_MIN_IN_UNIT + TIME_ZONE_OFFSET_MIN)

#define TIME_SUBSECONDS_IN_SECOND (UINT8_MAX + 1)
#define TIME_SUBSECONDS_TO_MS(subseconds) ((subseconds * 1000) / TIME_SUBSECONDS_IN_SECOND)
#define TIME_MS_TO_SUBSECONDS(milliseconds) ((milliseconds * TIME_SUBSECONDS_IN_SECOND) / MS_IN_S)

enum Month
{
    MONTH_JANUARY   = 0,
    MONTH_FEBRUARY  = 1,
    MONTH_MARCH     = 2,
    MONTH_APRIL     = 3,
    MONTH_MAY       = 4,
    MONTH_JUNE      = 5,
    MONTH_JULY      = 6,
    MONTH_AUGUST    = 7,
    MONTH_SEPTEMBER = 8,
    MONTH_OCTOBER   = 9,
    MONTH_NOVEMBER  = 10,
    MONTH_DECEMBER  = 11,
    MONTH_COUNT     = 12,
};

struct LocalTime
{
    uint16_t   year;
    enum Month month;
    uint8_t    day;
    uint8_t    hour;
    uint8_t    minutes;
    uint8_t    seconds;
};

/** @brief Converting Date to TAI seconds value from 2000.01.01 0:00:00
 *
 *  @param [in]      p_local_time              Pointer to date to be converted.
 *  @param [in]      time_zone_offset_minutes  Time zone offset in minutes.
 *  @param [in]      leap_seconds              TAI-UTC difference seconds.
 *
 *  @return uint64_t                           TAI seconds for passed date.
 */
uint64_t TAILocalTimeConverter_LocalTimeToTAI(struct LocalTime *p_local_time, int16_t time_zone_offset_minutes, int16_t leap_seconds);


/** @brief Converting TAI seconds to Date value from 2000.01.01 0:00:00
 *
 *  @param [in]      tai_seconds               TAI seconds value.
 *  @param [in]      time_zone_offset_minutes  Time zone offset in minutes.
 *  @param [in]      leap_seconds              TAI-UTC difference seconds.
 *
 *  @return struct LocalTime                   Date for passed TAI seconds value.
 */
struct LocalTime TAILocalTimeConverter_TAIToLocalTime(uint64_t tai_seconds, int16_t time_zone_offset_minutes, int16_t leap_seconds);


/** @brief Converting Date to seconds from 2000.01.01 0:00:00 including time zone and leap seconds value
 *
 *  @param [in]      p_local_time  Pointer to date to be converted.
 *
 *  @return uint64_t             seconds for passed date.
 */
uint64_t TAILocalTimeConverter_LocalTimeToLocalSeconds(struct LocalTime *p_local_time);

/** @brief Compare two LocalTime structs and return which date is later
 *
 *  @param [in]      p_local_time         Pointer to date to be converted.
 *  @param [in]      p_second_local_time  Pointer to date to be converted.
 *
 *  @return int8_t           Return:    1: p_local_time > p_second_local_time
 *                                      0: p_local_time == p_second_local_time
 *                                     -1: p_local_time < p_second_local_time
 *
 */
int8_t TAILocalTimeConverter_Compare(struct LocalTime *p_local_time, struct LocalTime *p_second_local_time);

#endif
