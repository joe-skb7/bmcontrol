#ifndef NANOSLEEP_WIN32_H
#define NANOSLEEP_WIN32_H

#include <time.h>

struct timespec
{
	time_t tv_sec;
	long int tv_nsec;
};

int nanosleep(const struct timespec *req, struct timespec *rem);

#endif /* NANOSLEEP_WIN32_H */
