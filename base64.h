/*
 * https://github.com/badzong/base64
 * Released to public domain
 * Written by Manuel Badzong. If you have suggestions or improvements, let me
 * know.
 */
#ifndef _BASE64_H_
#define _BASE64_H_

char *base64_enc_malloc(unsigned char *src, size_t slen);
unsigned char *base64_dec_malloc(char *src, size_t src_size, size_t *dest_size);

#endif