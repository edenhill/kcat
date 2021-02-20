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
