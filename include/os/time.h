#ifndef OS__TIME_H
#define OS__TIME_H

#if defined(__linux__)
# include <time.h>
#elif defined(_WIN32)
# include <win32/nanosleep.h>
#else
# pragma message("Currently only Linux and Windows build is supported. " \
                 "Trying to use the same \"time\" header as on Linux.")
# include <time.h>
#endif

#endif /* OS__TIME_H */
