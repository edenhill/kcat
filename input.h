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

#ifndef _INPUT_H_
#define _INPUT_H_


struct buf {
        void *buf;
        size_t size;
};


struct inbuf {
        const char *delim;
        size_t dsize;

        size_t sof;  /**< Scan-offset */

        char *buf;
        size_t size;  /**< Allocated size of buf */
        size_t len;   /**< How much of buf is used */

        size_t max_size;  /**< Including dsize */
};


void buf_destroy (struct buf *buf);

void inbuf_free_buf (void *buf, size_t size);
void inbuf_init (struct inbuf *inbuf, size_t max_size,
                 const char *delim, size_t delim_size);
int inbuf_read_to_delimeter (struct inbuf *inbuf, FILE *fp,
                             struct buf **outbuf);

#endif
