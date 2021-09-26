#include <math.h>
#include "day.h"
#include "util.h"

static time_t time_of_day_offset;

f64 get_time_of_day()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	return fmod((f64) ts.tv_sec - time_of_day_offset + ts.tv_nsec / 1.0e9, MINUTES_PER_DAY);
}
void set_time_of_day(time_t new_time)
{
	time_of_day_offset = time(NULL) - new_time;
}

f64 get_sun_angle()
{
	return 2.0 * M_PI * (f64) get_time_of_day() / MINUTES_PER_DAY + M_PI;
}

// workaround for calculating the nth root of a number x, where x may be negative if n is an odd number
double root(double x, int n)
{
	double sign = 1.0;

	if (x < 0.0) {
		if (n % 2 == 0)
			return 0.0 / 0.0;

		x = -x;
		sign = -sign;
	}

	return pow(x, 1.0 / (double) n) * sign;
}

f64 get_daylight()
{
	return root(cos(get_sun_angle()), 3) / 2.0 + 0.5;
}

void split_time_of_day(int *hours, int *minutes)
{
	time_t time_of_day = get_time_of_day();

	*minutes = time_of_day % MINUTES_PER_HOUR;
	*hours = time_of_day / MINUTES_PER_HOUR;
}
