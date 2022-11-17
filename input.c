/*
 * kcat - Apache Kafka consumer and producer
 *
 * Copyright (c) 2020-2021, Magnus Edenhill
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
#include "input.h"

#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <assert.h>

void buf_destroy (struct buf *b) {
#ifdef MREMAP_MAYMOVE
        munmap(b->buf, b->size);
#else
        free(b->buf);
#endif

        free(b);
}

static struct buf *buf_new (void *buf, size_t size) {
        struct buf *b;
        b = malloc(sizeof(*b));

        b->buf = buf;
        b->size = size;

        return b;
}




void inbuf_destroy (struct inbuf *inbuf) {

        if (inbuf->buf) {
#ifdef MREMAP_MAYMOVE
                munmap(inbuf->buf, inbuf->size);
#else
                free(inbuf->buf);
#endif
        }
}


/**
 * @returns the initial allocation size.
 */
static size_t inbuf_get_alloc_size (const struct inbuf *inbuf,
                                    size_t min_size) {
        const size_t max_size =
#ifdef MREMAP_MAYMOVE
                   4096
#else
                   1024
#endif
                ;

        if (inbuf->max_size < min_size)
                KC_FATAL("Invalid allocation size: %"PRIu64,
                         (uint64_t)min_size);

        return MAX(min_size, max_size);
}


void inbuf_free_buf (void *buf, size_t size) {
#ifdef MREMAP_MAYMOVE
        munmap(buf, size);
#else
        free(buf);
#endif
}


/**
 * @brief Allocate buffer memory. This memory MUST be freed with
 *        inbuf_free_buf().
 */
static char *inbuf_alloc_buf (size_t size) {
        void *p;

#ifdef MREMAP_MAYMOVE
        p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        if (!p)
                KC_FATAL("Failed to allocate(map) input buffer of %"
                         PRIu64" bytes: %s",
                         size, strerror(errno));
#else
        p = malloc(size);
        if (!p)
                KC_FATAL("Failed to allocate(malloc) input buffer of %"
                         PRIu64" bytes: %s",
                         size, strerror(errno));
#endif

        return (char *)p;
}


/**
 * @brief Grows the input buffer by at least \p min_size.
 */
static void inbuf_grow (struct inbuf *inbuf, size_t min_size) {
        size_t new_size;
        size_t req_size = inbuf->size + min_size;
        void *p;
#ifdef MREMAP_MAYMOVE
        float multiplier = 1.75;
#else
        float multiplier = 7.9;  /* Keep down the number of reallocs */
#endif

        new_size = MIN(inbuf->max_size, (size_t)((float)req_size * multiplier));

        if (new_size < req_size)
                KC_FATAL("Input is too large, maximum size is %"PRIu64,
                         inbuf->max_size);

#ifdef MREMAP_MAYMOVE
        p = mremap((void *)inbuf->buf, inbuf->size, new_size, MREMAP_MAYMOVE);
        if (p == MAP_FAILED)
                KC_FATAL("Failed to allocate(mremap) input buffer of size "
                         "%"PRIu64" bytes: %s",
                         (uint64_t)new_size, strerror(errno));

#else
        p = realloc(inbuf->buf, new_size);
        if (!p)
                KC_FATAL("Failed to allocate(realloc) input buffer of size "
                         "%"PRIu64" bytes: %s",
                         (uint64_t)new_size, strerror(errno));
#endif

        inbuf->buf = (char *)p;
        inbuf->size = new_size;
}


void inbuf_init (struct inbuf *inbuf, size_t max_size,
                 const char *delim, size_t delim_size) {
        memset(inbuf, 0, sizeof(*inbuf));

        inbuf->max_size = max_size + delim_size;

        inbuf->delim = delim;
        inbuf->dsize = delim_size;
        inbuf->sof = inbuf->dsize - 1;

        inbuf_grow(inbuf, inbuf_get_alloc_size(inbuf, 0));
}




/**
 * @brief Ensure there's at least \p min_size bytes remaining.
 */
static size_t inbuf_ensure (struct inbuf *inbuf, size_t min_size) {
        size_t remaining = inbuf->size - inbuf->len;

        if (remaining > min_size)
                return remaining;

        inbuf_grow(inbuf, min_size);

        return inbuf->size - inbuf->len;
}

/**
 * @brief Scan input buffer for delimiter and sets the delimiter offset
 *        in \p dofp. Updates the scan position.
 *
 * @returns 1 if delimiter is found, else 0.
 */
static int inbuf_scan (struct inbuf *inbuf, size_t *dofp) {
        const char *dstart = inbuf->delim;
        const char *dend = inbuf->delim + inbuf->dsize - 1;
        const char *t;

        if (inbuf->len < inbuf->sof)
                return 0; /* Input buffer smaller than delimiter. */

        /* Use Boyer-Moore inspired scan by matching the delimiter from
         * right-to-left. */
        while (inbuf->sof < inbuf->len &&
               (t = (const char *)memchr((void *)(inbuf->buf + inbuf->sof),
                                         (int)*dend,
                                         inbuf->len - inbuf->sof))) {
                /* Found delimiter's last byte and we know that there's at
                 * least the delimiter's size of data to left, so start
                 * scanning backwards. */
                const char *p = t;    /* buffer pointer */
                const char *d = dend; /* delimiter pointer */

                do {
                        d--;
                        p--;
                } while (d >= dstart && *d == *p);

                if (d < dstart) {
                        /* Found a full delimiter. */
                        *dofp = (size_t)(p+1 - inbuf->buf);
                        return 1;
                }

                /* Incomplete delimiter, scan forward. */
                inbuf->sof = (size_t)(t - inbuf->buf) + 1;
        }

        return 0;
}

/*
Key1;KeyDel;Value1:MyDilemma:;KeyDel;Value2:MyDilemma:Key3;KeyDel;
a:b:c
sof=1
sz=1
dof=sof-sz
*/

/**
 * @brief Split input buffer at delimiter offset \d dof and return it in
 *        \p outp and \p out_sizep, and copy the remaining bytes, if any,
 *        to a new buffer in \p inbuf.
 */
static void inbuf_split (struct inbuf *inbuf, size_t dof,
                         char **outp, size_t *out_sizep) {
        size_t nof = dof + inbuf->dsize;
        size_t remaining = inbuf->len - nof;
        void *rbuf = NULL;
        size_t rsize = 0;

        /* Copy right side of buffer (past the delimiter) to a temporary
         * buffer that will be stored on inbuf when the left side is extracted.
         */
        rsize = inbuf_get_alloc_size(inbuf, remaining);
        rbuf = inbuf_alloc_buf(rsize);
        if (remaining > 0)
                memcpy(rbuf, inbuf->buf+nof, remaining);

        /* Shrink the returned buffer to the actual size of the left side. */
        if (remaining + inbuf->dsize > 4096/2) {
#ifdef MREMAP_MAYMOVE
                *outp = mremap((void *)inbuf->buf, inbuf->size, dof,
                               MREMAP_MAYMOVE);
                if (!*outp)
                        KC_FATAL("Failed to shrink(mremap) buffer to %"
                                 PRIu64" bytes: %s",
                                 (uint64_t)dof, strerror(errno));
#else
                *outp = realloc((void *)inbuf->buf, MAX(dof, 16));
                if (!*outp)
                        KC_FATAL("Failed to shrink(REALLOC) buffer to %"
                                 PRIu64" bytes: %s",
                                 (uint64_t)dof, strerror(errno));
#endif
        } else {
                *outp = inbuf->buf;
        }


        *out_sizep = dof;


        /* Set up a new (or the remaining) input buffer. */
        inbuf->buf = rbuf;
        inbuf->size = rsize;
        inbuf->len = remaining;
        inbuf->sof = inbuf->dsize - 1;
}



/**
 * @brief Read up to delimiter and then return accumulated data in *inbuf.
 *
 * Call with *inbuf as NULL.
 *
 * @returns 0 on eof/error, else 1 inbuf is valid.
 */
int inbuf_read_to_delimeter (struct inbuf *inbuf, FILE *fp,
                             struct buf **outbuf) {
        int read_size = MIN(1024, inbuf->max_size);
        int fd = fileno(fp);
        fd_set readfds;

        /*
         * 1. Make sure there is enough output buffer room for read_size.
         * 2. Read up to read_size from input stream.
         * 3. Scan output buffer from current scan position for delimiter using
         *    Boyer-Moore (searched right-to-left).
         * 4. If delimiter is not found, go to 1.
         * 5. Skip delimiter and copy remaining buffer to new buffer.
         * 6. Return original buffer to caller.
         */

        if (!inbuf->buf)
                return 0;  /* Previous EOF encountered, see below. */

        while (conf.run) {
                ssize_t r;
                size_t dof;
                int delim_found;

                /* Scan for delimiter */
                delim_found = inbuf_scan(inbuf, &dof);

                if (delim_found) {
                        char *buf;
                        size_t size;

                        /* Delimiter found, split and return. */
                        inbuf_split(inbuf, dof, &buf, &size);

                        *outbuf = buf_new(buf, size);
                        return 1;
                }

                inbuf_ensure(inbuf, read_size);

                FD_ZERO(&readfds);
                FD_SET(fd, &readfds);
                select(1, &readfds, NULL, NULL, NULL);

                if (FD_ISSET(fd, &readfds))
                        r = read(fd, inbuf->buf+inbuf->len, read_size);
                else
                        r = 0;

                if (r <= 0) {
                        if (inbuf->len == 0) {
                                /* EOF with no accumulated data */
                                inbuf_destroy(inbuf);
                                return 0;
                        } else {
                                /* EOF but we have accumulated data, return what
                                 * we have. */
                                dof = inbuf->len;
                                *outbuf = buf_new(inbuf->buf, inbuf->len);
                                inbuf->buf = NULL;
                                return 1;
                        }
                }

                inbuf->len += (size_t)r;

        }

        return 0; /* NOTREACHED */
}
