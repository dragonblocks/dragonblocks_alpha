#ifndef _DAY_H_
#define _DAY_H_

#include <stdbool.h>
#include "types.h"
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define MINUTES_PER_DAY (HOURS_PER_DAY * MINUTES_PER_HOUR)

// 1 second in real life = 1 minute ingame

f64 get_time_of_day();
void set_time_of_day(f64 new_time);
f64 get_sun_angle();
f64 get_daylight();
void split_time_of_day(int *hours, int *minutes);

extern bool timelapse;

#endif
