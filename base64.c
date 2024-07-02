/*
 * https://github.com/badzong/base64
 * Released to public domain
 * Written by Manuel Badzong. If you have suggestions or improvements, let me
 * know.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <assert.h>

static char encoder[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char decoder[256];
static int initialized;

static void
base64_init_decoder(void) {
    int i = 0;

    if (initialized) {
        return;
    }

    // -1 is used for error detection
    memset(decoder, -1, sizeof decoder);

    for (; i < 64; decoder[(int) encoder[i]] = i, ++i);

    initialized = 1;

    return;
}

static int
base64_encsize(int size) {
    return 4 * ((size + 2) / 3);
}

int
base64_encode(char *dest, int size, unsigned char *src, int slen) {
    int dlen, i, j;
    uint32_t a, b, c, triple;

    dlen = base64_encsize(slen);

    // Sanity checks
    if (src == NULL || dest == NULL) {
        return -1;
    }
    if (dlen + 1 > size) {
        return -1;
    }
    if (slen == 0) {
        if (size > 0) {
            dest[0] = 0;
            return 0;
        }
        return -1;
    }

    for (i = 0, j = 0; i < slen;) {
        a = src[i++];

        // b and c may be off limit
        b = i < slen ? src[i++] : 0;
        c = i < slen ? src[i++] : 0;

        triple = (a << 16) + (b << 8) + c;

        dest[j++] = encoder[(triple >> 18) & 0x3F];
        dest[j++] = encoder[(triple >> 12) & 0x3F];
        dest[j++] = encoder[(triple >> 6) & 0x3F];
        dest[j++] = encoder[triple & 0x3F];
    }

    // Pad zeroes at the end
    switch (slen % 3) {
        case 1:
            dest[j - 2] = '=';
        case 2:
            dest[j - 1] = '=';
    }

    // Always add \0
    dest[j] = 0;

    return dlen;
}

char *
base64_enc_malloc(unsigned char *src, int slen) {
    int size;
    char *buffer;

    size = base64_encsize(slen) + 1;

    buffer = (char *) malloc(size);
    if (buffer == NULL) {
        return NULL;
    }

    size = base64_encode(buffer, size, src, slen);
    if (size == -1) {
        free(buffer);
        return NULL;
    }

    return buffer;
}


static int
base64_decsize_with_size(char *src, size_t slen) {
    int size, i;

    if (src == NULL) {
        return 0;
    }
    size = slen / 4 * 3;

    // Count pad chars
    for (i = 0; src[slen - i - 1] == '='; ++i);

    // Remove at most 2 bytes.
    return size - (i >= 2 ? 2 : i);
}

static int
base64_decode(unsigned char *dest, int size, char *src, size_t src_size) {
    int slen, dlen, padlen, i, j;
    uint32_t a, b, c, d, triple;

    // Initialize decoder
    base64_init_decoder();

    // Sanity checks
    if (src == NULL || dest == NULL) {
        return -1;
    }

    slen = src_size;
    if (slen == 0) {
        if (size > 0) {
            dest[0] = 0;
            return 0;
        }
        return -1;
    }

    // Remove trailing pad characters.
    for (padlen = 0; src[slen - padlen - 1] == '='; ++padlen);
    if (padlen > 2) {
        slen -= padlen - 2;
    }
    if (slen % 4) {
        return -1;
    }

    dlen = base64_decsize_with_size(src, src_size);

    // Check buffer size
    if (dlen + 1 > size) {
        return -1;
    }

    for (i = 0, j = 0; i < slen;) {
        a = decoder[(int) src[i++]];
        b = decoder[(int) src[i++]];
        c = decoder[(int) src[i++]];
        d = decoder[(int) src[i++]];

        // Sextet 3 and 4 may be zero at the end
        if (i == slen) {
            if (src[slen - 1] == '=') {
                d = 0;
                if (src[slen - 2] == '=') {
                    c = 0;
                }
            }
        }

        // Non-Base64 char
        if ((int)a == -1 || (int)b == -1 || (int)c == -1 || (int)d == -1) {
            return -1;
        }

        triple = (a << 18) + (b << 12) + (c << 6) + d;

        dest[j++] = (triple >> 16) & 0xFF;
        dest[j++] = (triple >> 8) & 0xFF;
        dest[j++] = triple & 0xFF;
    }

    // Always add \0
    dest[j] = 0;

    return dlen;
}

unsigned char *
base64_dec_malloc(char *src, size_t src_size, size_t *dest_size) {
    size_t size;
    unsigned char *buffer;

    size = base64_decsize_with_size(src, src_size) + 1;
    buffer = (unsigned char *) malloc(size);
    if (buffer == NULL) {
        return NULL;
    }

    size = base64_decode(buffer, size, src, src_size);
    if ((int)size == -1) {
        free(buffer);
        return NULL;
    }

    *dest_size = size;
    return buffer;
}

#ifdef DEBUG
int
main(void)
{
    struct testcase {
        char *dec;
        char *enc;
        int fail;
        int reverse;
    };

    struct testcase cases[] = {
        {"MQ==", "1", 0, 1},
        {"MTI=", "12", 0, 1},
        {"MTIz", "123", 0, 1},
        {"MTIzNA==", "1234", 0, 1},
        {"SGVsbG8gV29ybGQ=", "Hello World", 0, 1},
        {"aGVsbG8gd29ybGQ=", "hello world", 0, 1},
        {"Zm9vYmFy", "foobar", 0, 1},
        {"YmFyZm9v", "barfoo", 0, 1},
        {"dGVzdA==", "test", 0, 1},

        // Edge cases
        {"", "", 0, 1},
        {"dGVzdA===", "test", 0, 0},
        {"dGVzdA====", "test", 0, 0},
        {"=", NULL, 1, 0},
        {"==", NULL, 1, 0},
        {"-", NULL, 1, 0},
        {"dGVzd=A==", NULL, 1, 0},
        {"dGVzd=A=", NULL, 1, 0},
        {"d-GVzdA==", NULL, 1, 0},
        {"dGVzdA.=", NULL, 1, 0},
        {"GVzdA==", NULL, 1, 0},
        {NULL, NULL, 0, 0}
    };
    char *buffer;
    struct testcase *tc;

    for (tc = cases; tc->dec; ++tc)
    {
        // Decode
        buffer = (char *) base64_dec_malloc(tc->dec);
        if (tc->fail)
        {
            assert(buffer == NULL);
            continue;
        }

        assert(buffer != NULL);
        assert(strcmp(buffer, tc->enc) == 0);
        free(buffer);

        // Encode
        if (tc->reverse)
        {
            buffer = base64_enc_malloc((unsigned char *) tc->enc, strlen(tc->enc));
            assert(buffer != NULL);
            assert(strcmp(buffer, tc->dec) == 0);
            free(buffer);
        }
    }

    return 0;
}
#endif