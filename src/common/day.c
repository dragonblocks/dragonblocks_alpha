#include <math.h>
#include <time.h>
#include "common/day.h"

bool timelapse = false;
static f64 time_of_day_offset;

f64 get_unix_time()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ((f64) ts.tv_sec + ts.tv_nsec / 1.0e9) * (timelapse ? 100.0 : 1.0);
}

f64 get_time_of_day()
{
	return fmod(get_unix_time() - time_of_day_offset, MINUTES_PER_DAY);
}

void set_time_of_day(f64 new_time)
{
	time_of_day_offset = get_unix_time() - new_time;
}

f64 get_sun_angle()
{
	return 2.0 * M_PI * (f64) get_time_of_day() / MINUTES_PER_DAY + M_PI;
}

f64 get_daylight()
{
	return f64_clamp(cos(get_sun_angle()) * 2.0 + 0.5, 0.0, 1.0);
}

void split_time_of_day(int *hours, int *minutes)
{
	int time_of_day = get_time_of_day();

	*minutes = time_of_day % MINUTES_PER_HOUR;
	*hours = time_of_day / MINUTES_PER_HOUR;
}
