#ifndef OS_TIME_H
#define OS_TIME_H

#if defined(__linux__)
# include <time.h>
#elif defined(_WIN32)
# include "nanosleep_win32.h"
#else
/* Try to use the same as for Linux */
# include <time.h>
#endif

#endif /* OS_TIME_H */
