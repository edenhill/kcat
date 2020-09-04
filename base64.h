/*
 * Base64 encoding/decoding (RFC 4648)
 * Copyright (c) 2005, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2019, Jeremy Lin <jeremy.lin@gmail.com>
 *
 * Based on:
 * <https://github.com/freebsd/freebsd/blob/b53b2423/contrib/wpa/src/utils/base64.h>
 *
 * - Removed the logic for adding line feeds every 72 chars, so this code is
 *   no longer conformant to RFC 1341 (instead, RFC 4648).
 *
 * This software may be distributed under the terms of the BSD license.
 * See LICENSE.base64 for more details.
 */

#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

size_t base64_encode_alloc_len(size_t len);
char* base64_encode(const char *src, size_t len, size_t *out_len);
char* base64_decode(const char *src, size_t len, size_t *out_len);
char* base64_url_encode(const char *src, size_t len, size_t *out_len, int add_pad);
char* base64_url_decode(const char *src, size_t len, size_t *out_len);

#endif /* BASE64_H */
