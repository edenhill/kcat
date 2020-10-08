/*
 * librdkafka - Apache Kafka C library
 *
 * Copyright (c) 2012-2019 Magnus Edenhill
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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


#ifndef MAX
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#endif

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif
