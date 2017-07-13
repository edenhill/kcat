#pragma once

/**
 * Porting
 */

#ifdef _MSC_VER
/* Windows win32 native */

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

#define RD_NORETURN
#define RD_UNUSED

/* MSVC loves prefixing POSIX functions with underscore */
#define _COMPAT(FUNC)  _ ## FUNC

#define STDIN_FILENO 0

typedef SSIZE_T ssize_t;

ssize_t getdelim (char **bufptr, size_t *n,
				  int delim, FILE *fp);


/**
* @brief gettimeofday() for win32
*/
static RD_UNUSED
int rd_gettimeofday (struct timeval *tv, struct timezone *tz) {
	SYSTEMTIME st;
	FILETIME   ft;
	ULARGE_INTEGER d;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	d.HighPart = ft.dwHighDateTime;
	d.LowPart = ft.dwLowDateTime;
	tv->tv_sec = (long)((d.QuadPart - 116444736000000000llu) / 10000000L);
	tv->tv_usec = (long)(st.wMilliseconds * 1000);

	return 0;
}


#else
/* POSIX */

#define RD_NORETURN __attribute__((noreturn))
#define RD_UNUSED __attribute__((unused))

#define _COMPAT(FUNC) FUNC

#define rd_gettimeofday(tv,tz) gettimeofday(tv,tz)
#endif