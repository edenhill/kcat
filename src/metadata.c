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
  char errstr[512];
  char tmp[16];

  /* Enable quick termination of librdkafka */
  snprintf(tmp, sizeof(tmp), "%i", SIGIO);
  rd_kafka_conf_set(conf.rk_conf, "internal.termination.signal", tmp, NULL, 0);

  /* Parse command line arguments */
  metadata_argparse(argc, argv);

  /* Create handle */
  if (!(conf.rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf.rk_conf,
             errstr, sizeof(errstr))))
    FATAL("Failed to create rd_kafka producer: %s", errstr);

  rd_kafka_set_logger(conf.rk, rd_kafka_log_print);
  if (conf.debug)
    rd_kafka_set_log_level(conf.rk, LOG_DEBUG);
  else if (conf.verbosity == 0)
    rd_kafka_set_log_level(conf.rk, 0);

  /* Create topic, if specified */
  if (conf.topic &&
      !(conf.rkt = rd_kafka_topic_new(conf.rk, conf.topic,
              conf.rkt_conf)))
    FATAL("Failed to create rk_kafka_topic %s: %s", conf.topic,
          rd_kafka_err2str(rd_kafka_errno2err(errno)));

  conf.rk_conf  = NULL;
  conf.rkt_conf = NULL;

  rd_kafka_resp_err_t err;
  const struct rd_kafka_metadata *metadata;

  /* Fetch metadata */
  err = rd_kafka_metadata(conf.rk, conf.rkt ? 0 : 1, conf.rkt, &metadata, 5000);
  if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    FATAL("Failed to acquire metadata: %s", rd_kafka_err2str(err));

  /* Print metadata */
  metadata_print(metadata);

  rd_kafka_metadata_destroy(metadata);

  return conf.exitcode;
}
