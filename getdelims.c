/*
 * kafkacat - Apache Kafka consumer and producer
 *
 * Copyright (c) 2016, Magnus Edenhill
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "kafkacat.h"

#define MAGIC 0xCAFC
#define DEFAULT_LINE_SIZE 1024
#define MIN_LINE_SIZE 4

#define GET_STATE(voidp) (assert(((state_t *) voidp)->magic == MAGIC), ((state_t *) voidp))

typedef struct {
    int magic; // Just for sanity checking incoming args.
    size_t delim_len;
    char *delim;
    char *pushback;
    size_t pushback_index;
    size_t pushback_read_pointer;
} state_t;

static char _getc(FILE *fp, state_t *state);
static void _ungetc(state_t *state, char c);
static int _at_delim(FILE *stream, state_t *state);


/**
 * void *getdelims_init(const char *delim)
 * int getdelims(char **bufptr, size_t *n, void *state, FILE *stream)
 * void getdelims_done(void *state)
 *
 * This works _exactly_ like `getdelim()` but allows a multi-byte delimiter. Because of
 * the need to keep buffers between calls, this function needs to keep state so a first
 * call to `getdelims_init()` to allocate and initialize the state is needed; afterwards,
 * `getdelimis_done()` will free up the state.
 *
 * `getdelims_init` will copy the delimiter into a newly allocated buffer.
 *
 * The code for getdelims is pretty much a straight grab from the `getdelim()` code
 * by RedHat.
 */

void *getdelims_init(const char *delim) {
    state_t *state = (state_t *) malloc(sizeof(state_t));
    state->magic = MAGIC;
    state->delim_len = strlen(delim);
    state->delim = malloc(state->delim_len + 1);
    if (!state->delim) {
        KC_FATAL("getdelims_init: Cannot allocate memory for delimiter");
    }
    state->pushback = malloc(state->delim_len);
    if (!state->pushback) {
        free(state->delim);
        KC_FATAL("getdelims_init: Cannot allocate memory for pushback");
    }
    strcpy(state->delim, delim);
    state->pushback_index = state->pushback_read_pointer = 0;
    return state;
}
void getdelims_done(void *statep) {
    state_t *state = GET_STATE(statep);
    // TODO free buffers in state
    free(state->delim);
    free(state->pushback);
    free(state);
}

int getdelims(char **bufptr, size_t *n, void *statep, FILE *fp) {
    state_t *state = GET_STATE(statep);
    if (fp == NULL || bufptr == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    char *buf = *bufptr;
    if (buf == NULL || *n < MIN_LINE_SIZE) {
        buf = (char *) realloc(*bufptr, DEFAULT_LINE_SIZE);
        if (buf == NULL) {
            return -1;
        }
        *bufptr = buf;
        *n = DEFAULT_LINE_SIZE;
    }

    size_t numbytes = *n;
    char *ptr = buf;
    int cont = 1;

    while (cont)
    {
        while (--numbytes > 0) {
            int ch;
            if ((ch = _getc(fp, state)) == EOF) {
                cont = 0;
                break;
            }
            else {
                if (ch == state->delim[0] && _at_delim(fp, statep)) {
                    cont = 0;
                    break;
                }
                else {
                    *ptr++ = ch;
                }
            }
        }
        if (cont) {
            // Buffer is too small so reallocate a larger one
            int pos = ptr - buf;
            size_t newsize = *n << 1;
            buf = realloc(buf, newsize);
            if (buf == NULL) {
                cont = 0;
                break;
            }
            *bufptr = buf;
            *n = newsize;
            ptr = buf + pos;
            numbytes = newsize - pos;
        }
    }
    if (ptr == buf) {
        return -1;
    }
    *ptr = '\0';
    return (ssize_t) (ptr - buf);
}

// Stack pop and push. We push to index 0, 1, 2, ... so the popping needs to follow that order as well.
static char _getc(FILE *fp, state_t *state) {
    if (state->pushback_index > state->pushback_read_pointer) {
        size_t index = state->pushback_read_pointer++;
        if (index == state->pushback_index) {
            // stack empty, reset to beginning
            state->pushback_read_pointer = state->pushback_index = 0;
        }
        return state->pushback[index];
    } else {
        state->pushback_read_pointer = state->pushback_index = 0;
        return getc(fp);
    }
}
static void _ungetc(state_t *state, char c) {
    if (state->pushback_index == state->delim_len) {
        KC_FATAL("get_delims: BUG - overflow of pushback register. index = %d, len = %d, contents = '%s'\n", state->pushback_index, state->delim_len, state->pushback);
    }
    state->pushback[state->pushback_index++] = c;
}


static int _at_delim(FILE *stream, state_t *state) {
    // we have seen the first byte of the delimiter. Check whether the rest is there.
    int scan_rest = state->delim_len - 1;
    while (scan_rest > 0) {
        char c = fgetc(stream);
        if (c != state->delim[state->delim_len - scan_rest]) {
            // nope. we need to push what we've scanned into the pushback thing. As we compared true so far,
            // what we scanned can be taken from the delimiter.
            size_t i;
            for (i = 1; i < state->delim_len - scan_rest; i++) {
                _ungetc(state, state->delim[i]);
            }
            _ungetc(state, c);
            return 0;
        }
        scan_rest--;
    }
    return 1;
}

// while inotifywait getdelims.c; do cc -o test -DTEST -Itmp-bootstrap/usr/local/include getdelims.c && ./test; done
#ifdef TEST
int main(int _argc, char **_argv) {
    void *state = getdelims_init("123");
    char *test = "line1123and12line2123";
    FILE *testfd = fmemopen(test, strlen(test), "r");

    char *bufptr = NULL;
    size_t linelen = 0;

    assert(5 == getdelims(&bufptr, &linelen, state, testfd));
    assert(0 == strcmp(bufptr, "line1"));
    assert(10 == getdelims(&bufptr, &linelen, state, testfd));
    assert(0 == strcmp(bufptr, "and12line2"));
    assert(-1 == getdelims(&bufptr, &linelen, state, testfd));

    getdelims_done(state); // hard to test but should be fine

    printf("Wow. All tests passed!");
}

// Hack. Copy from kafkacat.c but can't link to that while testing.
#include <stdarg.h>
void RD_NORETURN fatal0(const char *func, int line, const char *fmt, ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    KC_INFO(2, "Fatal error at %s:%i:\n", func, line);
    fprintf(stderr, "%% ERROR: %s\n", buf);

    exit(1);
}
struct conf conf;
#endif
