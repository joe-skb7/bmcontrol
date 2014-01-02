#ifndef NANOSLEEP_WIN32_H
#define NANOSLEEP_WIN32_H

#ifdef __cplusplus
# include <ctime>
#else
# include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct timespec
{
	time_t tv_sec;
	long int tv_nsec;
};

int nanosleep(const struct timespec *req, struct timespec *rem);

#ifdef __cplusplus
}
#endif

#endif /* NANOSLEEP_WIN32_H */
