/*
 * stb-style Header only library for profiling code
 */
#ifndef PROFILE_H
#define PROFILE_H

#include <time.h>
double profile();

#endif /* PROFILE_H */

#ifdef PROFILE_IMPLEMENTATION

/*
 * returns time difference in seconds between the last call
 * returns the time since epoch for the first call
 */
double profile()
{
	static struct timespec profile_start = {0};
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	double ret = now.tv_sec - profile_start.tv_sec
		+ (now.tv_nsec <= profile_start.tv_nsec)
		+ (now.tv_nsec - profile_start.tv_nsec) / 1000000000.00;
	clock_gettime(CLOCK_REALTIME, &profile_start);
	return ret;
}


#endif /* PROFILE_IMPLEMENTATION */
