#ifndef OS_TIME_H
#define OS_TIME_H

#if defined(__linux__)
# include <time.h>
#elif defined(_WIN32)
# include "win32/nanosleep_win32.h"
#else
# pragma message("Currently only Linux and Windows build is supported. " \
                 "Trying to use the same \"time\" header as on Linux.")
# include <time.h>
#endif

#endif /* OS_TIME_H */
