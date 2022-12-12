/*
 * kcat - Apache Kafka consumer and producer
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

#include "kcat.h"
#include "rdendian.h"
#include <time.h>

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
#if ENABLE_JSON
        if (conf.flags & CONF_F_FMT_JSON)
                fmt_init_json();
#endif
}

void fmt_term (void) {
#if ENABLE_JSON
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
 * @brief Check that the pack-format string is valid.
 */
void pack_check (const char *what, const char *fmt) {
        const char *f = fmt;
        static const char *valid = " :,<>bBhHiIqQcsSvCt$";

        if (!*fmt)
                KC_FATAL("%s pack-format must not be empty", what);

        while (*f) {
                if (!strchr(valid, (int)(*f)))
                        KC_FATAL("Invalid token '%c' in %s pack-format, "
                                 "see -s usage.",
                                 (int)(*f), what);
                f++;
        }
}

#define expect(sz) do {                                         \
                if ((sz) > remaining) {                                 \
                        snprintf(errstr, errstr_size,                   \
                                 "%s truncated, expected %d bytes "     \
                                 "to unpack %c but only %d bytes remaining", \
                                 what, (int)(sz), (int)(*f),            \
                                 (int)remaining);                       \
                        return -1;                                      \
                }                                                       \
        } while (0)

#define fup_copy(dst,sz) do {                                           \
		expect(sz);                                             \
                memcpy(dst, b, sz);                                     \
                b += (sz);                                              \
        } while (0)


/**
 * @brief Unpack (deserialize) the data at \p buf using the
 *        pack-format string \p fmt.
 *        \p fmt must be a valid pack-format string.
 *        Prints result to \p fp.
 *
 * Format is inspired by Python's struct.unpack()
 *
 * @returns 0 on success or -1 on error.
 */
static int unpack (FILE *fp, const char *what, const char *fmt,
                   const char *buf, size_t len,
                   char *errstr, size_t errstr_size) {
        const char *b = buf;
        const char *end = buf+len;
        const char *f = fmt;
        enum {
                big_endian,
                little_endian
        } endian = big_endian;
#define endian_swap(val,to_little,to_big)                       \
        (endian == big_endian ? to_big(val) : to_little(val))


        while (*f) {
                size_t remaining = (int)(end - b);

                switch ((int)*f)
                {
                case ' ':
                        fprintf(fp, " ");
                        break;
		case ':':
			fprintf(fp, ":");
			break;
		case ',':
			fprintf(fp, ",");
			break;
                case '<':
                        endian = little_endian;
                        break;
                case '>':
                        endian = big_endian;
                        break;
                case 'b':
                {
                        int8_t v;
                        fup_copy(&v, sizeof(v));
                        fprintf(fp, "%d", (int)v);
                }
                break;
                case 'B':
                {
                        uint8_t v;
                        fup_copy(&v, sizeof(v));
                        fprintf(fp, "%u", (unsigned int)v);
                }
                break;
                case 'h':
                {
                        int16_t v;
                        fup_copy(&v, sizeof(v));
                        v = endian_swap(v, be16toh, htobe16);
                        fprintf(fp, "%hd", v);
                }
                break;
                case 'H':
                {
                        uint16_t v;
                        fup_copy(&v, sizeof(v));
                        v = endian_swap(v, be16toh, htobe16);
                        fprintf(fp, "%hu", v);
                }
                break;
                case 'i':
                {
                        int32_t v;
                        fup_copy(&v, sizeof(v));
                        v = endian_swap(v, be32toh, htobe32);
                        fprintf(fp, "%d", v);
                }
                break;
                case 'I':
                {
                        uint32_t v;
                        fup_copy(&v, sizeof(v));
                        v = endian_swap(v, be32toh, htobe32);
                        fprintf(fp, "%u", v);
                }
                break;
                case 'q':
                {
                        int64_t v;
                        fup_copy(&v, sizeof(v));
                        v = endian_swap(v, be64toh, htobe64);
                        fprintf(fp, "%"PRId64, v);
                }
                break;
                case 'Q':
                {
                        uint64_t v;
                        fup_copy(&v, sizeof(v));
                        v = endian_swap(v, be64toh, htobe64);
                        fprintf(fp, "%"PRIu64, v);
                }
                break;
                case 'c':
                        fprintf(fp, "%c", (int)*b);
                        b++;
                        break;
                case 's':
                {
                        fprintf(fp, "%.*s", (int)remaining, b);
                        b += remaining;
                }
                break;
		case 'S':
		{
			int16_t v;
			fup_copy(&v, sizeof(v));
			v = be16toh(v);
			if (v < 0)
				fprintf(fp, "(null)");
			else {
				fprintf(fp, "%.*s", v, b);
				expect((unsigned) v);
				b += v;
			}
		}
		break;
		case 'v':
		{
			uint8_t v;
			uint64_t value = 0;
			int i = 0;
			do {
				fup_copy(&v, sizeof(v));
				value |= ((uint64_t) (v & 0x7f)) << i;
				i += 7;
			} while (v & 0x80);
			fprintf(fp, "%"PRIu64, value);
		}
		break;
		case 'C':
		{
			uint8_t v;
			uint64_t value = 0;
			int i = 0;
			do {
				fup_copy(&v, sizeof(v));
				value |= ((uint64_t) (v & 0x7f)) << i;
				i += 7;
			} while (v & 0x80);
			expect((unsigned) value);
			fprintf(fp, "%.*s", (unsigned) value, b);
			b += value;
		}
		break;
		case 't':
		{
			char* buf;
                        int64_t v;
			time_t timestamp;
                        fup_copy(&v, sizeof(v));
			v = be64toh(v);
			if (v == -1) {
				fprintf(fp, "(none)");
			} else {
				timestamp = (time_t) (v / 1000);
				buf = ctime(&timestamp);
				/* this is totally vile but it works */
				*(buf + strlen(buf) - 1) = '\0';
				fprintf(fp, "%s", buf);
			}
		}
		break;	
                case '$':
                {
                        if (remaining > 0) {
                                snprintf(errstr, errstr_size,
                                         "expected %s end-of-input, "
                                         "but %d bytes remaining",
                                         what, (int)remaining);
                                return -1;
                        }
                }
                break;

                default:
                        KC_FATAL("Invalid pack-format token '%c'", (int)*f);
                        break;

                }

                f++;
        }

        /* Ignore remaining input bytes, if any. */
        return 0;

#undef endian_swap
}

static int unpack_offset_key(FILE *fp,
			     const char *buf, size_t len,
			     char *errstr, size_t errstr_size) {
	const char *b = buf;
	const char *end = buf + len;
	size_t remaining = (int)(end - b);
	const char *what = "key";
	const char *f = "h";

	uint16_t version = 0;
	int retval = 0;

	fup_copy(&version, sizeof(version));
	version = be16toh(version);
	buf += 2;
	len -= 2;
	remaining = (int)(end -  b);

	if (version < 0 || version > 2) {
		snprintf(errstr, errstr_size,
			 "Unknown key offset version V%d", (int) version);
		return -1;
	}
	retval = version;
	if (version < 2) {
		fprintf(fp, "[offset] ");
		if (unpack(fp, what, "S: S:i", buf, len, errstr, errstr_size) == -1)
			retval = -1;
	} else {
		fprintf(fp, "[group] ");
		if (unpack(fp, what, "S", buf, len, errstr, errstr_size) == -1)
			retval = -1;
	}
	return retval;
}

static int unpack_offset_value(FILE *fp,
			       int offset_key_type,
			       const char *buf, size_t len,
			       char *errstr, size_t errstr_size) {
	const char *b = buf;
	const char *end = buf + len;
	size_t remaining = (int)(end - b);
	const char *what = "value";
	const char *f = "h";

	uint16_t version = 0;
	
	fup_copy(&version, sizeof(version));
	version = be16toh(version);
	buf += 2;
	len -= 2;
	remaining = (int)(end - b);


	if (version >= 0 && version <= 3)
		fprintf(fp, "V%d: ", (int) version);
	if (offset_key_type == 0 || offset_key_type == 1) {
		switch(version) {
		case 0:
			return unpack(fp, what, "q S,t", buf, len, errstr, errstr_size);
		case 1:
			return unpack(fp, what, "q S,t,t", buf, len, errstr, errstr_size);
		case 2:
			return unpack(fp, what, "q S,t", buf, len, errstr, errstr_size);
		case 3:
			return unpack(fp, what, "q i,S,t", buf, len, errstr, errstr_size);
		default:
			snprintf(errstr, errstr_size,
				 "Unknown offset version V%d", (int) version);
			return -1;
		}
	} else if (offset_key_type == 2) {
		switch (version) {
		case 0:
		case 1:
			return unpack(fp, what, "S i S S", buf, len, errstr, errstr_size);
		case 2:
		case 3:
			return unpack(fp, what, "S i S S t", buf, len, errstr, errstr_size);
		default:
			/* cannot decode this yet */
			return 0;
		}
	} else {
		fprintf(fp, "(cannot decode)");
		return 0;
	}
}
	
#undef fup_copy
#undef expect


/**
 * Delimited output
 */
static void fmt_msg_output_str (FILE *fp,
                                const rd_kafka_message_t *rkmessage) {
        int i;
        char errstr[256];

        *errstr = '\0';
	int value_reprocess = -1;
	int continue_after = -1;
	int offset_key_type = -1;

        for (i = 0 ; i < conf.fmt_cnt ; i++) {
                int r = 1;
                uint32_t belen;
                const char *what_failed = "";

		if (i < continue_after && i != value_reprocess)
			continue;

                switch (conf.fmt[i].type)
                {
                case KC_FMT_OFFSET:
                        r = fprintf(fp, "%"PRId64, rkmessage->offset);
                        break;

                case KC_FMT_KEY:
                        if (rkmessage->key_len) {
                                if (conf.flags & CONF_F_FMT_AVRO_KEY) {
#if ENABLE_AVRO
                                        char *json = kc_avro_to_json(
                                                rkmessage->key,
                                                rkmessage->key_len,
                                                NULL,
                                                errstr, sizeof(errstr));

                                        if (!json) {
                                                what_failed =
                                                        "Avro/Schema-registry "
                                                        "key deserialization";
                                                goto fail;
                                        }

                                        r = fprintf(fp, "%s", json);
                                        free(json);
#else
                                        KC_FATAL("NOTREACHED");
#endif
				} else if (conf.flags & CONF_F_FMT_OFFSET_KEY) {
					offset_key_type = unpack_offset_key(fp,
									    rkmessage->key,
									    rkmessage->key_len,
									    errstr,
									    sizeof(errstr));
					if (offset_key_type == -1)
						goto fail;
					else if (value_reprocess != -1) {
						continue_after = i;
						i = value_reprocess;
					}
                                } else if (conf.pack[KC_MSG_FIELD_KEY]) {
                                        if (unpack(fp,
                                                   "key",
                                                   conf.pack[KC_MSG_FIELD_KEY],
                                                   rkmessage->key,
                                                   rkmessage->key_len,
                                                   errstr, sizeof(errstr)) ==
                                            -1)
                                                goto fail;
                                } else
                                        r = fwrite(rkmessage->key,
                                                   rkmessage->key_len, 1, fp);

                        } else if (conf.flags & CONF_F_NULL)
                                r = fwrite(conf.null_str,
                                           conf.null_str_len, 1, fp);

                        break;

                case KC_FMT_KEY_LEN:
                        r = fprintf(fp, "%zd",
                                    /* Use -1 to indicate NULL keys */
                                    rkmessage->key ? (ssize_t)rkmessage->key_len : -1);
                        break;

                case KC_FMT_PAYLOAD:
                        if (rkmessage->len) {
                                if (conf.flags & CONF_F_FMT_AVRO_VALUE) {
#if ENABLE_AVRO
                                        char *json = kc_avro_to_json(
                                                rkmessage->payload,
                                                rkmessage->len,
                                                NULL,
                                                errstr, sizeof(errstr));

                                        if (!json) {
                                                what_failed =
                                                        "Avro/Schema-registry "
                                                        "message "
                                                        "deserialization";
                                                goto fail;
                                        }

                                        r = fprintf(fp, "%s", json);
                                        free(json);
#else
                                        KC_FATAL("NOTREACHED");
#endif
				} else if (conf.flags & CONF_F_FMT_OFFSET_VALUE) {
					if ((conf.flags & CONF_F_FMT_OFFSET_KEY)) {
						if (offset_key_type == -1) {
							value_reprocess = i;
							break;
						}
					} else {
						/* assume "regular" offset, will work 99% of the time */
						offset_key_type = 1;
					}
							
					if (unpack_offset_value(fp,
								offset_key_type,
								rkmessage->payload,
								rkmessage->len,
								errstr,
								sizeof(errstr)) ==
					    -1)
						goto fail;
                                } else if (conf.pack[KC_MSG_FIELD_VALUE]) {
                                        if (unpack(fp,
                                                   "value",
                                                   conf.pack[KC_MSG_FIELD_VALUE],
                                                   rkmessage->payload,
                                                   rkmessage->len,
                                                   errstr, sizeof(errstr)) ==
                                            -1)
                                                goto fail;
                                } else
                                        r = fwrite(rkmessage->payload,
                                                   rkmessage->len, 1, fp);

                        } else if (conf.flags & CONF_F_NULL)
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
                        belen = htobe32((uint32_t)(rkmessage->payload ?
                                                   (ssize_t)rkmessage->len :
                                                   -1));
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
                                what_failed = "Failed to parse headers";
                                snprintf(errstr, sizeof(errstr), "%s",
                                         rd_kafka_err2str(err));
                                goto fail;
                        } else {
                                r = print_headers(fp, hdrs);
                        }
#endif
                        break;
                }
                }


                if (r < 1)
                        KC_FATAL("Write error for message "
                                 "of %zd bytes in %s [%"PRId32"] "
                                 "at offset %"PRId64": %s",
                                 rkmessage->len,
                                 rd_kafka_topic_name(rkmessage->rkt),
                                 rkmessage->partition,
                                 rkmessage->offset,
                                 strerror(errno));

                continue;

        fail:
                KC_ERROR("Failed to format message in %s [%"PRId32"] "
                         "at offset %"PRId64": %s%s%s",
                         rd_kafka_topic_name(rkmessage->rkt),
                         rkmessage->partition,
                         rkmessage->offset,
                         what_failed, *what_failed ? ": " : "",
                         errstr);
                return;
        }

}


/**
 * Format and output a received message.
 */
void fmt_msg_output (FILE *fp, const rd_kafka_message_t *rkmessage) {

#if ENABLE_JSON
        if (conf.flags & CONF_F_FMT_JSON)
                fmt_msg_output_json(fp, rkmessage);
        else
#endif
                fmt_msg_output_str(fp, rkmessage);

}
