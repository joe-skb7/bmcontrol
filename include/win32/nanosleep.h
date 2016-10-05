#ifndef WIN32__NANOSLEEP_H
#define WIN32__NANOSLEEP_H

#include <time.h>

struct timespec {
	time_t tv_sec;
	long int tv_nsec;
};

int nanosleep(const struct timespec *req, struct timespec *rem);

#endif /* WIN32__NANOSLEEP_H */
