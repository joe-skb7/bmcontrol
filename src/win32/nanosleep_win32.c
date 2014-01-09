/*
* nanosleep() implementation for Win32 platform.
*
* Given from:
* http://lists.gnu.org/archive/html/bug-gnulib/2010-04/msg00045.html
*
* Written by Jim Meyering and Bruno Haible for the Woe32 part.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include "win32/nanosleep_win32.h"

enum { BILLION = 1000 * 1000 * 1000 };

static int initialized;
static double ticks_per_nanosecond;

static void init_ticks_per_nsec(void)
{
	LARGE_INTEGER ticks_per_second;

	if (QueryPerformanceFrequency(&ticks_per_second))
		ticks_per_nanosecond = (double)ticks_per_second.QuadPart
				/ 1000000000.0;

	initialized = 1;
}

/*
 * Wait for less than sec.
 *
 * RETURN CODES:
 * 1 - sleep finished successfull
 * 0 - unsuccessfull sleeping
 */
static int short_sleep(const struct timespec *req)
{
	int sleep_millis = (int)req->tv_nsec / 1000000 - 10;
	LONGLONG wait_ticks;
	LONGLONG wait_until;
	LARGE_INTEGER counter_before;

	if (!initialized)
		init_ticks_per_nsec();
	if (!ticks_per_nanosecond)
		return 0;

	wait_ticks = req->tv_nsec * ticks_per_nanosecond;

	if (!QueryPerformanceCounter(&counter_before))
		return 0;

	wait_until = counter_before.QuadPart + wait_ticks;

	if (sleep_millis > 0)
		Sleep(sleep_millis);

	for (;;) {
		LARGE_INTEGER counter_after;

		if (!QueryPerformanceCounter(&counter_after))
			break;

		if (counter_after.QuadPart >= wait_until)
			break;
	}

	return 1;
}

/*
 * Wait for more than sec.
 */
static void long_sleep(const struct timespec *req)
{
	Sleep(req->tv_sec * 1000 + req->tv_nsec / 1000000);
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	if (req->tv_nsec < 0 || req->tv_nsec >= BILLION) {
		errno = EINVAL;
		return -1;
	}

	if (req->tv_sec != 0) {
		long_sleep(req);
	} else {
		if (!short_sleep(req))
			long_sleep(req);
	}

	if (rem != NULL) {
		rem->tv_sec = 0;
		rem->tv_nsec = 0;
	}

	return 0;
}
