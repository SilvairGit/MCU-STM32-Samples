#include "TAILocalTimeConverter.h"

#include <stdbool.h>

#define HOUR_MAX (24)
#define MINUTE_MAX (60)
#define SECOND_MAX (60)
#define DAY_SECONDS (HOUR_MAX * MINUTE_MAX * SECOND_MAX)
#define WEEK_SECONDS (DAY_SECONDS * 7)
#define YEAR_SECONDS (DAY_SECONDS * 365)

#define TIME_ZONE_OFFSET_TO_SECONDS(x) ((x)*60)
#define EPOCH_YEAR (2000)
#define MAX_DAYS_IN_MONTH (31)

static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool IsLeapYear(uint16_t year);

static uint8_t ValidDayGet(uint8_t day, enum Month month, uint16_t year);

static bool IsLeapYear(uint16_t year)
{
    if (year % 4 != 0)
    {
        return false;
    }
    if (year % 100 != 0)
    {
        return true;
    }
    if (year % 400 != 0)
    {
        return false;
    }
    return true;
}

static uint8_t ValidDayGet(uint8_t day, enum Month month, uint16_t year)
{
    if (month != MONTH_FEBRUARY)
    {
        return (day > days_in_month[month]) ? (days_in_month[month]) : day;
    }

    uint8_t num_days_feb = days_in_month[MONTH_FEBRUARY];

    if (IsLeapYear(year))
    {
        num_days_feb += 1;
    }

    return (day > num_days_feb) ? (num_days_feb) : day;
}

uint64_t TAILocalTimeConverter_LocalTimeToTAI(struct LocalTime *p_local_time, int16_t time_zone_offset_minutes, int16_t leap_seconds)
{
    uint64_t tai_seconds = 0;
    for (uint16_t cur_year = EPOCH_YEAR; cur_year < p_local_time->year; cur_year++)
    {
        tai_seconds += IsLeapYear(cur_year) ? YEAR_SECONDS + DAY_SECONDS : YEAR_SECONDS;
    }

    for (uint8_t cur_mon = MONTH_JANUARY; cur_mon < p_local_time->month; cur_mon++)
    {
        tai_seconds += ValidDayGet(MAX_DAYS_IN_MONTH, (enum Month)cur_mon, p_local_time->year) * DAY_SECONDS;
    }

    tai_seconds += (ValidDayGet(p_local_time->day, p_local_time->month, p_local_time->year) - 1) * DAY_SECONDS;
    tai_seconds += p_local_time->hour * 3600 + p_local_time->minutes * 60 + p_local_time->seconds;
    tai_seconds += leap_seconds;
    tai_seconds -= TIME_ZONE_OFFSET_TO_SECONDS(time_zone_offset_minutes);

    return tai_seconds;
}

struct LocalTime TAILocalTimeConverter_TAIToLocalTime(uint64_t tai_seconds, int16_t time_zone_offset_minutes, int16_t leap_seconds)
{
    struct LocalTime local_time;

    int64_t  local_time_tai = tai_seconds + TIME_ZONE_OFFSET_TO_SECONDS(time_zone_offset_minutes) - leap_seconds;
    uint64_t L;
    if (local_time_tai < 0)
    {
        L = 0;
    }
    else if (local_time_tai > TIME_TAI_SECONDS_TIME_MAX)
    {
        L = TIME_TAI_SECONDS_TIME_MAX;
    }
    else
    {
        L = local_time_tai;
    }
    // Use algorithm passed in Mesh Model Specification,  page 136
    uint64_t D       = (L / 86400);
    uint8_t  HOURS   = ((L - D * 86400) / 3600);
    uint8_t  MINUTES = ((L - D * 86400 - HOURS * 3600) / 60);
    uint8_t  SECONDS = L - D * 86400 - HOURS * 3600 - MINUTES * 60;

    uint32_t B    = D + 730119;
    uint32_t Q    = B % 146097;
    uint32_t C    = Q / 36524;
    uint16_t H1   = Q % 36524;
    uint16_t X    = ((H1 % 1461) / 365);
    uint16_t YEAR = (B / 146097) * 400 + C * 100 + (H1 / 1461) * 4 + X + (!((C == 4) || (X == 4)) ? 1 : 0);
    uint16_t Z    = YEAR - 1;
    uint32_t V    = B - 365 * Z - (Z / 4) + (Z / 100) - (Z / 400);
    uint8_t  A;
    if ((YEAR % 4 == 0) && ((YEAR % 100 != 0) || (YEAR % 400 == 0)))
    {
        A = 1;
    }
    else
    {
        A = 2;
    }
    uint8_t J;
    if (V + A < 61)
    {
        J = 0;
    }
    else
    {
        J = A;
    }

    uint8_t MONTH = (((V + J) * 12 + 373) / 367);

    uint8_t K;
    if (MONTH <= 2)
    {
        K = 0;
    }
    else
    {
        K = A;
    }
    uint8_t DAY = V + K + 1 - ((367 * MONTH - 362) / 12);

    local_time.year    = YEAR;
    local_time.month   = (enum Month)(MONTH - 1);    // Adjust to enum Month
    local_time.day     = DAY;
    local_time.hour    = HOURS;
    local_time.minutes = MINUTES;
    local_time.seconds = SECONDS;

    return local_time;
}

uint64_t TAILocalTimeConverter_LocalTimeToLocalSeconds(struct LocalTime *p_local_time)
{
    return TAILocalTimeConverter_LocalTimeToTAI(p_local_time, 0, 0);
}

int8_t TAILocalTimeConverter_Compare(struct LocalTime *p_local_time, struct LocalTime *p_second_local_time)
{
    if (p_local_time->year > p_second_local_time->year)
    {
        return 1;
    }
    if (p_local_time->year < p_second_local_time->year)
    {
        return -1;
    }
    if (p_local_time->month > p_second_local_time->month)
    {
        return 1;
    }
    if (p_local_time->month < p_second_local_time->month)
    {
        return -1;
    }
    if (p_local_time->day > p_second_local_time->day)
    {
        return 1;
    }
    if (p_local_time->day < p_second_local_time->day)
    {
        return -1;
    }
    if (p_local_time->hour > p_second_local_time->hour)
    {
        return 1;
    }
    if (p_local_time->hour < p_second_local_time->hour)
    {
        return -1;
    }
    if (p_local_time->minutes > p_second_local_time->minutes)
    {
        return 1;
    }
    if (p_local_time->minutes < p_second_local_time->minutes)
    {
        return -1;
    }
    if (p_local_time->seconds > p_second_local_time->seconds)
    {
        return 1;
    }
    if (p_local_time->seconds < p_second_local_time->seconds)
    {
        return -1;
    }
    return 0;
}
