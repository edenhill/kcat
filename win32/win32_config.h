#pragma once

#define KCAT_VERSION "1.8.0" /* Manually updated */

/*
    From syslog.h - to fix "E0020: identifier "LOG_DEBUG" is undefined"
    Added "ifndef" directives for the case where someone might use a third party syslog client, e.g. https://github.com/asankah/syslog-win32
*/
#ifndef	LOG_EMERG
#define	LOG_EMERG	0	/* system is unusable */
#endif
#ifndef	LOG_ALERT
#define	LOG_ALERT	1	/* action must be taken immediately */
#endif
#ifndef	LOG_CRIT
#define	LOG_CRIT	2	/* critical conditions */
#endif
#ifndef	LOG_ERR
#define	LOG_ERR		3	/* error conditions */
#endif
#ifndef	LOG_WARNING
#define	LOG_WARNING	4	/* warning conditions */
#endif
#ifndef	LOG_NOTICE
#define	LOG_NOTICE	5	/* normal but significant condition */
#endif
#ifndef	LOG_INFO
#define	LOG_INFO	6	/* informational */
#endif
#ifndef	LOG_DEBUG
#define	LOG_DEBUG	7	/* debug-level messages */
#endif

/* check librdkafka version and define ENABLE_KAFKACONSUMER */
#if RD_KAFKA_VERSION >= 0x00090100
#define ENABLE_KAFKACONSUMER 1
#else
#define ENABLE_KAFKACONSUMER 0
#endif
