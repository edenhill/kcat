/*
 * kafkacat - Apache Kafka consumer and producer
 *
 * Copyright (c) 2015, Magnus Edenhill
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

#include "kafkacat.h"

#ifndef _MSC_VER
#include <arpa/inet.h>  /* for htonl() */
#endif

static void fmt_add (fmt_type_t type, const char *str, int len) {
        if (conf.fmt_cnt == KC_FMT_MAX_SIZE)
                KC_FATAL("Too many formatters & strings (KC_FMT_MAX_SIZE=%i)",
                         KC_FMT_MAX_SIZE);

        conf.fmt[conf.fmt_cnt].type = type;

        /* For STR types */
        if (len) {
                const char *s;
                char *d;
                conf.fmt[conf.fmt_cnt].str = d = malloc(len+1);
                memcpy(d, str, len);
                d[len] = '\0';
                s = d;

                /* Convert \.. sequences */
                while (*s) {
                        if (*s == '\\' && *(s+1)) {
                                int base = 0;
                                const char *next;
                                s++;
                                switch (*s) {
                                case 't':
                                        *d = '\t';
                                        break;
                                case 'n':
                                        *d = '\n';
                                        break;
                                case 'r':
                                        *d = '\r';
                                        break;
                                case 'x':
                                        s++;
                                        base = 16;
                                        /* FALLTHRU */
                                default:
                                        if (*s >= '0' && *s <= '9') {
                                                *d = (char)strtoul(
                                                        s, (char **)&next,
                                                        base);
                                                if (next > s)
                                                        s = next - 1;
                                        } else {
                                                *d = *s;
                                        }
                                        break;
                                }
                        } else {
                                *d = *s;
                        }
                        s++;
                        d++;
                }

                *d = '\0';

                conf.fmt[conf.fmt_cnt].str_len =
                        strlen(conf.fmt[conf.fmt_cnt].str);
        }

        conf.fmt_cnt++;
}


/**
 * Parse a format string to create a formatter list.
 */
void fmt_parse (const char *fmt) {
        const char *s = fmt, *t;

        while (*s) {
                if ((t = strchr(s, '%'))) {
                        if (t > s)
                                fmt_add(KC_FMT_STR, s, (int)(t-s));

                        s = t+1;
                        switch (*s)
                        {
                        case 'o':
                                fmt_add(KC_FMT_OFFSET, NULL, 0);
                                break;
                        case 'k':
                                fmt_add(KC_FMT_KEY, NULL, 0);
                                break;
                        case 'K':
                                fmt_add(KC_FMT_KEY_LEN, NULL, 0);
                                break;
                        case 's':
                                fmt_add(KC_FMT_PAYLOAD, NULL, 0);
                                break;
                        case 'S':
                                fmt_add(KC_FMT_PAYLOAD_LEN, NULL, 0);
                                break;
                        case 'R':
                                fmt_add(KC_FMT_PAYLOAD_LEN_BINARY, NULL, 0);
                                break;
                        case 't':
                                fmt_add(KC_FMT_TOPIC, NULL, 0);
                                break;
                        case 'p':
                                fmt_add(KC_FMT_PARTITION, NULL, 0);
                                break;
                        case 'T':
                                fmt_add(KC_FMT_TIMESTAMP, NULL, 0);
                                conf.flags |= CONF_F_APIVERREQ;
                                break;
#if HAVE_HEADERS
                        case 'h':
                                fmt_add(KC_FMT_HEADERS, NULL, 0);
                                conf.flags |= CONF_F_APIVERREQ;
                                break;
#endif
                        case '%':
                                fmt_add(KC_FMT_STR, s, 1);
                                break;
                        case '\0':
                                KC_FATAL("Empty formatter");
                                break;
                        default:
                                KC_FATAL("Unsupported formatter: %%%c", *s);
                                break;
                        }
                        s++;
                } else {
                        fmt_add(KC_FMT_STR, s, strlen(s));
                        break;
                }

        }
}




void fmt_init (void) {
#ifdef ENABLE_JSON
        if (conf.flags & CONF_F_FMT_JSON)
                fmt_init_json();
#endif
}

void fmt_term (void) {
#ifdef ENABLE_JSON
        if (conf.flags & CONF_F_FMT_JSON)
                fmt_term_json();
#endif
}


#if HAVE_HEADERS
static int print_headers (FILE *fp, const rd_kafka_headers_t *hdrs) {
        size_t idx = 0;
        const char *name;
        const void *value;
        size_t size;

        while (!rd_kafka_header_get_all(hdrs, idx++, &name, &value, &size)) {
                fprintf(fp, "%s%s=", idx > 1 ? "," : "", name);
                if (value && size > 0)
                        fprintf(fp, "%.*s", (int)size, (const char *)value);
                else if (!value)
                        fprintf(fp, "NULL");
        }
        return 1;
}
#endif

/**
 * Delimited output
 */
static void fmt_msg_output_str (FILE *fp,
                                const rd_kafka_message_t *rkmessage) {
        int i;

        for (i = 0 ; i < conf.fmt_cnt ; i++) {
                int r = 1;
                uint32_t belen;

                switch (conf.fmt[i].type)
                {
                case KC_FMT_OFFSET:
                        r = fprintf(fp, "%"PRId64, rkmessage->offset);
                        break;

                case KC_FMT_KEY:
                        if (rkmessage->key_len)
                                r = fwrite(rkmessage->key,
                                           rkmessage->key_len, 1, fp);
                        else if (conf.flags & CONF_F_NULL)
                                r = fwrite(conf.null_str,
                                           conf.null_str_len, 1, fp);

                        break;

                case KC_FMT_KEY_LEN:
                        r = fprintf(fp, "%zd",
                                    /* Use -1 to indicate NULL keys */
                                    rkmessage->key ? (ssize_t)rkmessage->key_len : -1);
                        break;

                case KC_FMT_PAYLOAD:
                        if (rkmessage->len)
                                r = fwrite(rkmessage->payload,
                                           rkmessage->len, 1, fp);
                        else if (conf.flags & CONF_F_NULL)
                                r = fwrite(conf.null_str,
                                           conf.null_str_len, 1, fp);
                        break;

                case KC_FMT_PAYLOAD_LEN:
                        r = fprintf(fp, "%zd",
                                    /* Use -1 to indicate NULL messages */
                                    rkmessage->payload ? (ssize_t)rkmessage->len : -1);
                        break;

                case KC_FMT_PAYLOAD_LEN_BINARY:
                        /* Use -1 to indicate NULL messages */
                        belen = htonl((uint32_t)(rkmessage->payload ?
						 (ssize_t)rkmessage->len : -1));
                        r = fwrite(&belen, sizeof(uint32_t), 1, fp);
                        break;

                case KC_FMT_STR:
                        r = fwrite(conf.fmt[i].str, conf.fmt[i].str_len, 1, fp);
                        break;

                case KC_FMT_TOPIC:
                        r = fprintf(fp, "%s",
                                    rd_kafka_topic_name(rkmessage->rkt));
                        break;

                case KC_FMT_PARTITION:
                        r = fprintf(fp, "%"PRId32, rkmessage->partition);
                        break;

                case KC_FMT_TIMESTAMP:
                {
#if RD_KAFKA_VERSION >= 0x000902ff
                        rd_kafka_timestamp_type_t tstype;
                        r = fprintf(fp, "%"PRId64,
                                    rd_kafka_message_timestamp(rkmessage,
                                                               &tstype));
#else
                        r = fprintf(fp, "-1");
#endif
                        break;
                }

                case KC_FMT_HEADERS:
                {
#if HAVE_HEADERS
                        rd_kafka_headers_t *hdrs;
                        rd_kafka_resp_err_t err;

                        err = rd_kafka_message_headers(rkmessage, &hdrs);
                        if (err == RD_KAFKA_RESP_ERR__NOENT) {
                                r = 1; /* Fake it to continue */
                        } else if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
                                fprintf(stderr,
                                        "%% Failed to parse headers: %s",
                                        rd_kafka_err2str(err));
                                r = 1; /* Fake it to continue */
                        } else {
                                r = print_headers(fp, hdrs);
                        }
#endif
                        break;
                }
                }


                if (r < 1)
                        KC_FATAL("Write error for message "
                              "of %zd bytes at offset %"PRId64"): %s",
                              rkmessage->len, rkmessage->offset,
                              strerror(errno));
        }

}


/**
 * Format and output a received message.
 */
void fmt_msg_output (FILE *fp, const rd_kafka_message_t *rkmessage) {

#ifdef ENABLE_JSON
        if (conf.flags & CONF_F_FMT_JSON)
                fmt_msg_output_json(fp, rkmessage);
        else
#endif
                fmt_msg_output_str(fp, rkmessage);

}
