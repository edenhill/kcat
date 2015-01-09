/*
 * kc - Apache Kafka consumer and producer
 *
 * Copyright (c) 2015, François Saint-Jacques
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

conf_t conf = {
  .run = 1,
  .verbosity = 1,
  .partition = RD_KAFKA_PARTITION_UA,
  .msg_size = 1024*1024,
  .delim = '\n',
  .key_delim = '\t',
  .brokers = "localhost",
};

/**
 * Fatal error: print error and exit
 */
void __attribute__((noreturn)) fatal0 (const char *func, int line,
                                       const char *fmt, ...) {
  va_list ap;
  char buf[1024];

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  fprintf(stderr, "%% ERROR: %s\n", buf);
  exit(1);
}

int run = 1;

/**
 * Terminate by putting out the run flag.
 */
void term(int sig) {
  run = 0;
}

void set_signals() {
  if (signal(SIGINT,  term) == SIG_ERR ||
      signal(SIGTERM, term) == SIG_ERR ||
      signal(SIGPIPE, term) == SIG_ERR)
    FATAL("Could not set signal handler for SIGINT");
}

/**
* Parse delimiter string from command line arguments.
*/
int parse_delim (const char *str) {
  int delim;
  if (!strncmp(str, "\\x", strlen("\\x")))
    delim = strtoul(str+strlen("\\x"), NULL, 16) & 0xff;
  else if (!strcmp(str, "\\n"))
    delim = (int)'\n';
  else if (!strcmp(str, "\\t"))
    delim = (int)'\t';
  else
    delim = (int)*str & 0xff;
  return delim;
}

/**
 * Print usage and exit.
 */
void __attribute__((noreturn)) usage (const char *argv0, int exitcode,
                                      const char *reason) {
  if (reason)
    printf("%s%s\n\n", exitcode ? "Error: " : "" , reason);

  printf("Usage:\n"
         "\n"
         "kc producer <topic> [<file>...]\n"
         "kc consumer <topic>\n"
         "kc metadata [<topic>]\n"
         "kc --help\n"
         "kc --version\n"
         "\n"
         "General options:\n"
         "  -b <brokers,..>    Bootstrap broker(s) (host[:port])\n"
         "  -p <partition>     Partition\n"
         "  -d <delim>   Message delimiter character:\n"
         "         a-z.. | \\r | \\n | \\t | \\xNN\n"
         "         Default: \\n\n"
         "  -k <delim>   Key delimiter (same format as -D)\n"
         "  -c <cnt>     Exit after producing this number "
         "  -X list      List available librdkafka configuration "
         "properties\n"
         "  -X prop=val  Set librdkafka configuration property.\n"
         "         Properties prefixed with \"topic.\" are\n"
         "         applied as topic properties.\n"
         "  -X dump      Dump configuration and exit.\n"
         "  -V <dbg1,...>      Enable librdkafka debugging:\n"
         "         " RD_KAFKA_DEBUG_CONTEXTS "\n"
         "  -q     Be quiet (verbosity set to 0)\n"
         "  -v     Increase verbosity\n"
         "\n"
         "Producer options:\n"
         "  -z snappy|gzip     Message compression. Default: none\n"
         "  -p -1        Use random partitioner\n"
         "  -T     Output sent messages to stdout, acting like tee.\n"
         "\n"
         "See kc(1) for more information\n"
         "\n"
         "https://github.com/edenhill/kafkacat\n"
         "Copyright (c) 2014, Magnus Edenhill\n"
         "\n");
  exit(exitcode);
}
