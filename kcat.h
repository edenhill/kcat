/*
 * kcat - Apache Kafka consumer and producer
 *
 * Copyright (c) 2015-2019, Magnus Edenhill
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

#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <librdkafka/rdkafka.h>

#include "rdport.h"

#ifdef _MSC_VER
#pragma comment(lib, "librdkafka.lib")
#include "win32/win32_config.h"
#else
#include "config.h"
#endif

#if ENABLE_AVRO
#include <libserdes/serdes.h>
#endif

#ifdef RD_KAFKA_V_HEADER
#define HAVE_HEADERS 1
#else
#define HAVE_HEADERS 0
#endif

#if RD_KAFKA_VERSION >= 0x000b0500
#define HAVE_CONTROLLERID 1
#else
#define HAVE_CONTROLLERID 0
#endif


typedef enum {
        KC_FMT_STR,
        KC_FMT_OFFSET,
        KC_FMT_KEY,
        KC_FMT_KEY_LEN,
        KC_FMT_PAYLOAD,
        KC_FMT_PAYLOAD_LEN,
        KC_FMT_PAYLOAD_LEN_BINARY,
        KC_FMT_TOPIC,
        KC_FMT_PARTITION,
        KC_FMT_TIMESTAMP,
        KC_FMT_HEADERS
} fmt_type_t;

#define KC_FMT_MAX_SIZE  128

typedef enum {
        KC_MSG_FIELD_VALUE,
        KC_MSG_FIELD_KEY,
        KC_MSG_FIELD_CNT
} kc_msg_field_t;

struct conf {
        int     run;
        int     verbosity;
        int     exitcode;
        int     exitonerror;
        char    mode;
        int     flags;
#define CONF_F_FMT_JSON   0x1 /* JSON formatting */
#define CONF_F_KEY_DELIM  0x2 /* Producer: use key delimiter */
#define CONF_F_OFFSET     0x4 /* Print offsets */
#define CONF_F_TEE        0x8 /* Tee output when producing */
#define CONF_F_NULL       0x10 /* -Z: Send empty messages as NULL */
#define CONF_F_LINE       0x20 /* Read files in line mode when producing */
#define CONF_F_APIVERREQ  0x40 /* Enable api.version.request=true */
#define CONF_F_APIVERREQ_USER 0x80 /* User set api.version.request */
#define CONF_F_NO_CONF_SEARCH 0x100 /* Disable default config file search */
#define CONF_F_BROKERS_SEEN   0x200 /* Brokers have been configured */
#define CONF_F_FMT_AVRO_KEY   0x400 /* Convert key from Avro to JSON */
#define CONF_F_FMT_AVRO_VALUE 0x800 /* Convert value from Avro to JSON  */
#define CONF_F_SR_URL_SEEN    0x1000 /* schema.registry.url/-r seen */
        char   *delim;
        size_t  delim_size;
        char   *key_delim;
        size_t  key_delim_size;

        struct {
                fmt_type_t type;
                const char *str;
                int         str_len;
        } fmt[KC_FMT_MAX_SIZE];
        int     fmt_cnt;

        /**< Producer: per-field pack-format, see pack()
         *   Consumer: per-field unpack-format, see unpack(). */
        const char *pack[KC_MSG_FIELD_CNT];

        int     msg_size;
        char   *brokers;
        char   *topic;
        int32_t partition;
        rd_kafka_headers_t *headers;
        char   *group;
        char   *fixed_key;
        int32_t fixed_key_len;
        int64_t offset;
#if RD_KAFKA_VERSION >= 0x00090300
        int64_t startts;
        int64_t stopts;
#endif
        int64_t msg_cnt;
        int     exit_eof;
        int     eof_cnt;  /**< Current number of partitions reached EOF */
        rd_kafka_topic_partition_list_t *assignment; /**< Current -G consumer
                                                      *   assignment */
        int     metadata_timeout;
        char   *null_str;
        int     null_str_len;
        int     txn;

        rd_kafka_conf_t       *rk_conf;
        rd_kafka_topic_conf_t *rkt_conf;

        rd_kafka_t            *rk;
        rd_kafka_topic_t      *rkt;

        char   *debug;

        int term_sig;  /**< Termination signal */

#if ENABLE_AVRO
        serdes_conf_t *srconf;
        char   *schema_registry_url;
#endif
};

extern struct conf conf;


void RD_NORETURN fatal0 (const char *func, int line,
                         const char *fmt, ...);

void error0 (int erroronexit, const char *func, int line,
             const char *fmt, ...);

#define KC_FATAL(.../*fmt*/)  fatal0(__FUNCTION__, __LINE__, __VA_ARGS__)

#define KC_ERROR(.../*fmt*/)  error0(conf.exitonerror, __FUNCTION__, __LINE__, __VA_ARGS__)

/* Info printout */
#define KC_INFO(VERBLVL,.../*fmt*/) do {                        \
                if (conf.verbosity >= (VERBLVL))                \
                        fprintf(stderr, "%% " __VA_ARGS__);     \
        } while (0)



/*
 * format.c
 */
void pack_check (const char *what, const char *fmt);

void fmt_msg_output (FILE *fp, const rd_kafka_message_t *rkmessage);

void fmt_parse (const char *fmt);

void fmt_init (void);
void fmt_term (void);



#if ENABLE_JSON
/*
 * json.c
 */
void fmt_msg_output_json (FILE *fp, const rd_kafka_message_t *rkmessage);
void metadata_print_json (const struct rd_kafka_metadata *metadata,
                          int32_t controllerid);
void partition_list_print_json (const rd_kafka_topic_partition_list_t *parts,
                                void *json_gen);
void fmt_init_json (void);
void fmt_term_json (void);
int  json_can_emit_verbatim (void);
#endif

#if ENABLE_AVRO
/*
 * avro.c
 */
char *kc_avro_to_json (const void *data, size_t data_len, int *schema_idp,
                       char *errstr, size_t errstr_size);

void kc_avro_init (const char *key_schema_name,
                   const char *key_schema_path,
                   const char *value_schema_name,
                   const char *value_schema_path);
void kc_avro_term (void);
#endif


/*
 * tools.c
 */
int query_offsets_by_time (rd_kafka_topic_partition_list_t *offsets);
