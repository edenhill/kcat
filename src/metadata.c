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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"

static struct option metadata_long_options[] = {
    {"brokers",       required_argument, 0, 'b'},
    {"partition",     required_argument, 0, 'p'},
    {"verbose",       no_argument,       0, 'v'},
    {"quiet",         no_argument,       0, 'q'},
    {0,               0,                 0,  0 }
};

static void metadata_argparse (int argc, char **argv) {
  char errstr[512];
  int opt;
  int option_index = 0;

  while ((opt = getopt_long(argc, argv,
                            "b:p:vqX",
                            metadata_long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'p':
      conf.partition = atoi(optarg);
      break;
    case 'b':
      conf.brokers = optarg;
      break;
    case 'q':
      conf.verbosity = 0;
      break;
    case 'v':
      conf.verbosity++;
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
      usage(argv[0], 1, "unknown argument");
      break;
    }
  }

  if (rd_kafka_conf_set(conf.rk_conf, "metadata.broker.list",
            conf.brokers, errstr, sizeof(errstr)) !=
      RD_KAFKA_CONF_OK)
    usage(argv[0], 1, errstr);

  /* Validate topic */
  if (argc - optind == 1)
    conf.topic = argv[optind++];
}

static void metadata_print (const struct rd_kafka_metadata *metadata) {
  printf("{\n");

  /* Iterate brokers */
  printf("  \"brokers\": [\n");
  for (int i = 0 ; i < metadata->broker_cnt ; i++)
    printf("    { \"id\":%"PRId32", \"host\":\"%s:%i\" %s\n",
           metadata->brokers[i].id,
           metadata->brokers[i].host,
           metadata->brokers[i].port,
           (i == metadata->broker_cnt - 1) ? "}" : "},");
  printf("  ],\n");

  /* Iterate topics */
  printf("  \"topics\": [\n");
  for (int i = 0 ; i < metadata->topic_cnt ; i++) {
    const struct rd_kafka_metadata_topic *t = &metadata->topics[i];
    if (t->err) {
      fprintf(stderr, " %s", rd_kafka_err2str(t->err));
      continue;
    }

    printf("    {\n"
           "      \"name\": \"%s\", \n"
           "      \"partitions\": [\n", t->topic);

    /* Iterate topic's partitions */
    for (int j = 0 ; j < t->partition_cnt ; j++) {
      const struct rd_kafka_metadata_partition *p = &t->partitions[j];
      if (p->err)
        fprintf(stderr, "%s\n", rd_kafka_err2str(p->err));

      printf("        { \"id\": %"PRId32", \"leader\": %"PRId32
             ", \"replicas\": [",
             p->id, p->leader);

      /* Iterate partition's replicas */
      for (int k = 0 ; k < p->replica_cnt ; k++)
        printf("%"PRId32"%s", p->replicas[k],
               (k == p->replica_cnt - 1)? "]":",");

      /* Iterate partition's ISRs */
      printf(", \"isrs\": [");
      for (int k = 0 ; k < p->isr_cnt ; k++)
        printf("%"PRId32"%s", p->isrs[k],
               (k == p->isr_cnt - 1) ? "]":",");


      printf(" }%s\n", (j == t->partition_cnt - 1) ? "" : ",");
    }
    printf("      ]\n");

    printf("    }%s\n", (i == metadata->topic_cnt - 1) ? "" : ",");
  }
  printf("  ]\n");

  printf("}");
}

int metadata_main(int argc, char **argv) {
  rd_kafka_resp_err_t err;
  const struct rd_kafka_metadata *metadata;

  /* Parse command line arguments */
  metadata_argparse(argc, argv);

  kc_rdkafka_init(RD_KAFKA_PRODUCER);

  /* Fetch metadata */
  err = rd_kafka_metadata(conf.rk, conf.rkt ? 0 : 1, conf.rkt, &metadata, 5000);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    FATAL("Failed to acquire metadata: %s", rd_kafka_err2str(err));

  /* Print metadata */
  metadata_print(metadata);

  rd_kafka_metadata_destroy(metadata);

  return conf.exitcode;
}
