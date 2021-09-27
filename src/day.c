#include <math.h>
#include "day.h"
#include "util.h"

static time_t time_of_day_offset;

f64 get_time_of_day()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	return fmod((f64) ts.tv_sec - time_of_day_offset + ts.tv_nsec  / 1.0e9, MINUTES_PER_DAY);
}
void set_time_of_day(time_t new_time)
{
	time_of_day_offset = time(NULL) - new_time;
}

f64 get_sun_angle()
{
	return 2.0 * M_PI * (f64) get_time_of_day() / MINUTES_PER_DAY + M_PI;
}

f64 get_daylight()
{
	return clamp(cos(get_sun_angle()) * 2.0 + 0.5, 0.0, 1.0);
}

void split_time_of_day(int *hours, int *minutes)
{
	time_t time_of_day = get_time_of_day();

	*minutes = time_of_day % MINUTES_PER_HOUR;
	*hours = time_of_day / MINUTES_PER_HOUR;
}
