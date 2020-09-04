/*
 * Base64 encoding/decoding (RFC 4648)
 * Copyright (c) 2005-2019, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2019, Jeremy Lin <jeremy.lin@gmail.com>
 *
 * Based on:
 * <https://github.com/freebsd/freebsd/blob/25d65ba7/contrib/wpa/src/utils/base64.c>
 *
 * - Removed the logic for adding line feeds every 72 chars, so this code is
 *   no longer conformant to RFC 1341 (instead, RFC 4648).
 *
 * This software may be distributed under the terms of the BSD license.
 * See LICENSE.base64 for more details.
 */

#include <stdlib.h>
#include <string.h>

#include "base64.h"

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const unsigned char base64_url_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

size_t base64_encode_alloc_len(size_t len) {
	return (len * 4 / 3 + 4) /* 3-byte blocks to 4-byte */
                + 1; /* nul terminator */
}

static unsigned char* base64_gen_encode(const unsigned char *src, size_t len,
					size_t *out_len,
					const unsigned char *table,
					int add_pad)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	const size_t olen = base64_encode_alloc_len(len);

	if (olen < len)
		return NULL; /* integer overflow */
	out = malloc(olen);
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	while (end - in >= 3) {
		*pos++ = table[(in[0] >> 2) & 0x3f];
		*pos++ = table[(((in[0] & 0x03) << 4) | (in[1] >> 4)) & 0x3f];
		*pos++ = table[(((in[1] & 0x0f) << 2) | (in[2] >> 6)) & 0x3f];
		*pos++ = table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = table[(in[0] >> 2) & 0x3f];
		if (end - in == 1) {
			*pos++ = table[((in[0] & 0x03) << 4) & 0x3f];
			if (add_pad)
				*pos++ = '=';
		} else {
			*pos++ = table[(((in[0] & 0x03) << 4) |
					(in[1] >> 4)) & 0x3f];
			*pos++ = table[((in[1] & 0x0f) << 2) & 0x3f];
		}
		if (add_pad)
			*pos++ = '=';
	}

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}

static unsigned char* base64_gen_decode(const unsigned char *src, size_t len,
					size_t *out_len,
					const unsigned char *table)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;
	size_t extra_pad;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0)
		return NULL;
	extra_pad = (4 - count % 4) % 4;

	olen = (count + extra_pad) / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len + extra_pad; i++) {
		unsigned char val;

		if (i >= len)
			val = '=';
		else
			val = src[i];
		tmp = dtable[val];
		if (tmp == 0x80)
			continue;

		if (val == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
char* base64_encode(const char *src, size_t len, size_t *out_len)
{
	return (char*) base64_gen_encode((const unsigned char*) src, len, out_len,
                                         base64_table, 1);
}

char* base64_url_encode(const char *src, size_t len, size_t *out_len, int add_pad)
{
	return (char*) base64_gen_encode((const unsigned char*) src, len, out_len,
                                         base64_url_table, add_pad);
}

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
char* base64_decode(const char *src, size_t len, size_t *out_len)
{
	return (char*) base64_gen_decode((const unsigned char*) src, len, out_len,
                                         base64_table);
}

char* base64_url_decode(const char *src, size_t len, size_t *out_len)
{
	return (char*) base64_gen_decode((const unsigned char*) src, len, out_len,
                                         base64_url_table);
}
