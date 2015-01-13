/*
 * kc - Apache Kafka consumer and producer
 *
 * Copyright (c) 2015, François Saint-Jacques
 * Copyright (c) 2014, Magnus Edenhill
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
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"

static struct option producer_long_options[] = {
    {"brokers",       required_argument, 0, 'b'},
    {"partition",     required_argument, 0, 'p'},
    {"delimiter",     required_argument, 0, 'd'},
    {"key-delimiter", required_argument, 0, 'k'},
    {"count",         required_argument, 0, 'c'},
    {"compression",   required_argument, 0, 'z'},
    {"error-file",    required_argument, 0, 'E'},
    {"tee",           no_argument,       0, 'T'},
    {"verbose",       no_argument,       0, 'v'},
    {"quiet",         no_argument,       0, 'q'},
    {0,               0,                 0,  0 }
};

static void producer_argparse (int argc, char **argv) {
  char errstr[512];
  char **left_argv;
  int opt;
  int option_index = 0;
  int left_argc = 0;

  while ((opt = getopt_long(argc, argv,
                            "b:p:d:k:c:z:uTX:E:vq",
                            producer_long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'p':
      conf.partition = atoi(optarg);
      break;
    case 'b':
      conf.brokers = optarg;
      break;
    case 'z':
      if (rd_kafka_conf_set(conf.rk_conf,
                "compression.codec", optarg,
                errstr, sizeof(errstr)) !=
          RD_KAFKA_CONF_OK)
        FATAL("%s", errstr);
      break;
    case 'd':
      conf.delim = parse_delim(optarg);
      break;
    case 'k':
      conf.key_delim = parse_delim(optarg);
      conf.flags |= CONF_F_KEY_DELIM;
      break;
    case 'c':
      conf.msg_cnt = strtoll(optarg, NULL, 10);
      break;
    case 'q':
      conf.verbosity = 0;
      break;
    case 'v':
      conf.verbosity++;
      break;
    case 'T':
      conf.flags |= CONF_F_TEE;
      break;
    case 'u':
      setbuf(stdout, NULL);
      break;
    case 'E':
      conf.fd_err = fopen(optarg, "a");
      if (conf.fd_err == NULL)
        FATAL("Couldn't open error file %s: %s\n", optarg, strerror(errno));
      break;
    case 'X':
    {
      char *name, *val;
      rd_kafka_conf_res_t res;

      if (!strcmp(optarg, "list") ||
          !strcmp(optarg, "help")) {
        rd_kafka_conf_properties_show(stdout);
        exit(0);
      }

      if (!strcmp(optarg, "dump")) {
        conf.conf_dump = 1;
        continue;
      }

      name = optarg;
      if (!(val = strchr(name, '='))) {
        fprintf(stderr, "%% Expected "
          "-X property=value, not %s, "
          "use -X list to display available "
          "properties\n", name);
        exit(1);
      }

      *val = '\0';
      val++;

      res = RD_KAFKA_CONF_UNKNOWN;
      /* Try "topic." prefixed properties on topic
       * conf first, and then fall through to global if
       * it didnt match a topic configuration property. */
      if (!strncmp(name, "topic.", strlen("topic.")))
        res = rd_kafka_topic_conf_set(conf.rkt_conf,
                    name+
                    strlen("topic."),
                    val,
                    errstr,
                    sizeof(errstr));

      if (res == RD_KAFKA_CONF_UNKNOWN)
        res = rd_kafka_conf_set(conf.rk_conf, name, val,
              errstr, sizeof(errstr));

      if (res != RD_KAFKA_CONF_OK)
        FATAL("%s", errstr);
    }
    break;

    default:
      usage(argv[0], 1, NULL);
      break;
    }
  }

  /* Validate topic */
  if (argc - optind == 0)
    usage(argv[0], 1, "topic missing");
  else
    conf.topic = argv[optind++];

  /* Validate broker list */
  if (rd_kafka_conf_set(conf.rk_conf, "metadata.broker.list",
            conf.brokers, errstr, sizeof(errstr)) !=
      RD_KAFKA_CONF_OK)
    usage(argv[0], 1, errstr);

  /* Set message error log file descriptor */
  if(conf.fd_err == NULL)
    conf.fd_err = stderr;

  /* Retrieve input files */
  left_argc = argc - optind;
  conf.n_inputs = (left_argc == 0) ? 1 : left_argc;
  left_argv = &argv[optind];

  conf.inputs = calloc(conf.n_inputs, sizeof(char *));

  if (left_argc == 0 ||
      (left_argc == 1 && !strcmp(left_argv[0], "-"))) {
    /* Defaults to stdin if no file is passed or if single '-' */
    conf.inputs[0] = stdin;
  } else {
    for (int i = 0; i < conf.n_inputs; i++) {
      FILE *fd = fopen(left_argv[i], "r");

      if (fd == NULL)
        FATAL("Failed to open %s: %s\n", left_argv[i], strerror(errno));

      conf.inputs[i] = fd;
    }
  }
}

static void *poll_loop (void *arg) {
  while(conf.run)
    rd_kafka_poll(conf.rk, 5);

  return NULL;
}

static void produce_cb (rd_kafka_t *rk,
                        const rd_kafka_message_t *msg, void *opaque) {
  FILE *fd = conf.fd_err;
  bool failed = false;

  if (msg->err && msg->payload) {
    /* Write key if required */
    if (conf.flags & CONF_F_KEY_DELIM) {
      failed |= (fwrite(msg->key, msg->key_len, 1, fd) != 1) ||
                (fwrite(&(conf.key_delim), 1, 1, fd) != 1);
    }

    /* Write message */
    failed |= (fwrite(msg->payload, msg->len, 1, fd) != 1) ||
              (fwrite(&(conf.delim), 1, 1, fd) != 1);

    if (failed)
      FATAL("Couldn't write to error log");
  }

}

/**
 * Produces a single message, retries on queue congestion, and
 * exits hard on error.
 */
static void produce_message (void *buf, size_t len,
                             const void *key, size_t key_len, int msgflags) {
  /* Produce message: keep trying until it succeeds. */
  do {
    rd_kafka_resp_err_t err;

    if (!conf.run)
      INFO(LOG_ERR,
           "Program terminated while producing message of %zd bytes", len);

    if (rd_kafka_produce(conf.rkt, conf.partition, msgflags,
                         buf, len, key, key_len, NULL) != -1) {
      stats.tx++;
      break;
    }

    err = rd_kafka_errno2err(errno);

    if (err != RD_KAFKA_RESP_ERR__QUEUE_FULL)
      INFO(LOG_ERR, "Failed to produce message (%zd bytes): %s",
            len, rd_kafka_err2str(err));

    stats.tx_err_q++;

    /* Internal queue full, sleep to allow
     * messages to be produced/time out
     * before trying again.
     */
    usleep(5);
  } while (1);
}

/**
 * Produces a single file exits hard on error.
 */
static void produce_file (FILE *fp) {
  char   *sbuf  = NULL;
  size_t  size = 0;
  ssize_t len;

  /* Read messages from fp, delimited by conf.delim */
  while (conf.run &&
           (len = getdelim(&sbuf, &size, conf.delim, fp)) != -1) {
      int msgflags = 0;
      char *buf = sbuf;
      char *key = NULL;
      size_t key_len = 0;
      size_t orig_len = len;

      if (len == 0)
        continue;

      /* Shave off delimiter */
      if ((int)buf[len-1] == conf.delim)
        len--;

      if (len == 0)
        continue;

      /* Extract key, if desired and found. */
      if (conf.flags & CONF_F_KEY_DELIM) {
        char *t;
        if ((t = memchr(buf, conf.key_delim, len))) {
          key_len = (size_t)(t-sbuf);
          key     = buf;
          buf    += key_len+1;
          len    -= key_len+1;
        }
      }

      if (len > 1024 && !(conf.flags & CONF_F_TEE)) {
        /* If message is larger than this arbitrary
         * threshold it will be more effective to
         * not copy the data but let rdkafka own it
         * instead.
         *
         * Note that CONF_T_TEE must be checked,
         * otherwise a possible race might occur. */
        msgflags |= RD_KAFKA_MSG_F_FREE;
      } else {
        /* For smaller messages a copy is
         * more efficient. */
        msgflags |= RD_KAFKA_MSG_F_COPY;
      }

      /* Produce message */
      produce_message(buf, len, key, key_len, msgflags);

      if (conf.flags & CONF_F_TEE &&
          fwrite(buf, orig_len, 1, stdout) != 1)
        FATAL("Tee write error for message of %zd bytes: %s",
              orig_len, strerror(errno));

      if (msgflags & RD_KAFKA_MSG_F_FREE) {
        /* rdkafka owns the allocated buffer
         * memory now. */
        sbuf  = NULL;
        size = 0;
      }

      /* Enforce -c <cnt> */
      if (stats.tx == conf.msg_cnt)
        conf.run = 0;
    }

    if (conf.run) {
      if (!feof(fp))
        FATAL("Unable to read message: %s",
              strerror(errno));
    }

  if (sbuf)
    free(sbuf);
}

void producer_main (int argc, char **argv) {
  pthread_t poll_thread;

  /* Parse command line arguments */
  producer_argparse(argc, argv);

  /* Set producer callback */
  rd_kafka_conf_set_dr_msg_cb(conf.rk_conf, produce_cb);

  kc_rdkafka_init(RD_KAFKA_PRODUCER);

  if(pthread_create(&poll_thread, NULL, &poll_loop, NULL))
    FATAL("Could not create loop thread");

  for (size_t i = 0; i < conf.n_inputs; i++)
    produce_file(conf.inputs[i]);

  /* successfully completed */
  conf.run = 0;

  pthread_join(poll_thread, NULL);

  /* Close input files */
  for (int i = 0; i < conf.n_inputs; i++)
    fclose(conf.inputs[i]);
}
