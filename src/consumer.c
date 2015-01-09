#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <syslog.h>
#include <stdbool.h>

#include "common.h"

static struct option consumer_long_options[] = {
    {"brokers",       required_argument, 0, 'b'},
    {"partition",     required_argument, 0, 'p'},
    {"delimiter",     required_argument, 0, 'd'},
    {"key-delimiter", required_argument, 0, 'k'},
    {"offset",        required_argument, 0, 'o'},
    {"count",         required_argument, 0, 'c'},
    {"exit",          no_argument,       0, 'e'},
    {"verbose",       no_argument,       0, 'v'},
    {"quiet",         no_argument,       0, 'q'},
    {0,               0,                 0,  0 }
};

static void consumer_argparse (int argc, char **argv) {
  char errstr[512];
  int opt;
  int option_index = 0;

  while ((opt = getopt_long(argc, argv,
                            "b:p:d:k:o:c:euX:vq",
                            consumer_long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'p':
      conf.partition = atoi(optarg);
      break;
    case 'b':
      conf.brokers = optarg;
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
    case 'o':
      if (!strcmp(optarg, "end"))
        conf.offset = RD_KAFKA_OFFSET_END;
      else if (!strcmp(optarg, "beginning"))
        conf.offset = RD_KAFKA_OFFSET_BEGINNING;
      else if (!strcmp(optarg, "stored"))
        conf.offset = RD_KAFKA_OFFSET_STORED;
      else {
        conf.offset = strtoll(optarg, NULL, 10);
        if (conf.offset < 0)
          conf.offset = RD_KAFKA_OFFSET_TAIL(-conf.offset);
      }
      break;
    case 'e':
      conf.exit_eof = 1;
      break;
    case 'q':
      conf.verbosity = 0;
      break;
    case 'v':
      conf.verbosity++;
      break;
    case 'u':
      setbuf(stdout, NULL);
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
    usage("kc", 1, errstr);
}

/* Partition's at EOF state array */
int *part_eof = NULL;
/* Number of partitions that has reached EOF */
int part_eof_cnt = 0;
/* Threshold level (partitions at EOF) before exiting */
int part_eof_thres = 0;

/**
 * Consume callback, called for each message consumed.
 */
static void consume_cb (rd_kafka_message_t *rkmessage, void *opaque) {
  FILE *fp = opaque;

  /* FIXME: We dont want to commit offsets if we're not running. */
  if (!conf.run)
    return;

  if (rkmessage->err) {
    if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
      if (conf.exit_eof) {
        if (!part_eof[rkmessage->partition]) {
          part_eof[rkmessage->partition] = 1;
          part_eof_cnt++;

          if (part_eof_cnt >= part_eof_thres)
            conf.run = 0;
        }

        INFO(2, "Reached end of topic %s [%"PRId32"] "
             "at offset %"PRId64"%s\n",
             rd_kafka_topic_name(rkmessage->rkt),
             rkmessage->partition,
             rkmessage->offset,
             !conf.run ? ": exiting" : "");
      }
      return;
    }

    FATAL("Topic %s [%"PRId32"] error: %s",
    rd_kafka_topic_name(rkmessage->rkt),
    rkmessage->partition,
    rd_kafka_message_errstr(rkmessage));
  }

  /* Print offset (using key delim), if desired */
  if (conf.flags & CONF_F_OFFSET)
    fprintf(fp, "%"PRId64"%c", rkmessage->offset, conf.key_delim);

  /* Print key, if desired */
  if (conf.flags & CONF_F_KEY_DELIM)
    fprintf(fp, "%.*s%c",
      (int)rkmessage->key_len, (const char *)rkmessage->key,
      conf.key_delim);

  if (fwrite(rkmessage->payload, rkmessage->len, 1, fp) != 1 ||
      fwrite(&conf.delim, 1, 1, fp) != 1)
    FATAL("Write error for message "
    "of %zd bytes at offset %"PRId64"): %s",
    rkmessage->len, rkmessage->offset, strerror(errno));

  if (++stats.rx == conf.msg_cnt)
    conf.run = 0;
}

int consumer_main(int argc, char **argv) {
  char errstr[512];
  rd_kafka_resp_err_t err;
  const struct rd_kafka_metadata *metadata;
  rd_kafka_queue_t *rkqu;
  int i;

  consumer_argparse(argc, argv);

  /* Create consumer */
  if (!(conf.rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf.rk_conf,
             errstr, sizeof(errstr))))
    FATAL("Failed to create producer: %s", errstr);

  if (conf.debug)
    rd_kafka_set_log_level(conf.rk, LOG_DEBUG);
  else if (conf.verbosity == 0)
    rd_kafka_set_log_level(conf.rk, 0);

  /* Create topic */
  if (!(conf.rkt = rd_kafka_topic_new(conf.rk, conf.topic,
              conf.rkt_conf)))
    FATAL("Failed to create topic %s: %s", conf.topic,
          rd_kafka_err2str(rd_kafka_errno2err(errno)));

  conf.rk_conf  = NULL;
  conf.rkt_conf = NULL;

  /* Query broker for topic + partition information. */
  if ((err = rd_kafka_metadata(conf.rk, 0, conf.rkt, &metadata, 5000)))
    FATAL("Failed to query metadata for topic %s: %s",
          rd_kafka_topic_name(conf.rkt), rd_kafka_err2str(err));

  /* Error handling */
  if (metadata->topic_cnt == 0)
    FATAL("No such topic in cluster: %s",
          rd_kafka_topic_name(conf.rkt));

  if ((err = metadata->topics[0].err))
    FATAL("Topic %s error: %s",
          rd_kafka_topic_name(conf.rkt), rd_kafka_err2str(err));

  if (metadata->topics[0].partition_cnt == 0)
    FATAL("Topic %s has no partitions",
          rd_kafka_topic_name(conf.rkt));

  /* If Exit-at-EOF is enabled, set up array to track EOF
   * state for each partition. */
  if (conf.exit_eof) {
    part_eof = calloc(sizeof(*part_eof),
          metadata->topics[0].partition_cnt);

    if (conf.partition != RD_KAFKA_PARTITION_UA)
      part_eof_thres = 1;
    else
      part_eof_thres = metadata->topics[0].partition_cnt;
  }

  /* Create a shared queue that combines messages from
   * all wanted partitions. */
  rkqu = rd_kafka_queue_new(conf.rk);

  /* Start consuming from all wanted partitions. */
  for (i = 0 ; i < metadata->topics[0].partition_cnt ; i++) {
    int32_t partition = metadata->topics[0].partitions[i].id;

    /* If -p <part> was specified: skip unwanted partitions */
    if (conf.partition != RD_KAFKA_PARTITION_UA &&
        conf.partition != partition)
      continue;

    /* Start consumer for this partition */
    if (rd_kafka_consume_start_queue(conf.rkt, partition,
             conf.offset, rkqu) == -1)
      FATAL("Failed to start consuming "
            "topic %s [%"PRId32"]: %s",
            conf.topic, partition,
            rd_kafka_err2str(rd_kafka_errno2err(errno)));

    if (conf.partition != RD_KAFKA_PARTITION_UA)
      break;
  }

  if (conf.partition != RD_KAFKA_PARTITION_UA &&
      i == metadata->topics[0].partition_cnt)
    FATAL("Topic %s (with partitions 0..%i): "
          "partition %i does not exist",
          rd_kafka_topic_name(conf.rkt),
          metadata->topics[0].partition_cnt-1,
          conf.partition);


  /* Read messages from Kafka, write to 'stdout'. */
  while (conf.run) {
    rd_kafka_consume_callback_queue(rkqu, 100,
            consume_cb, stdout);
  }

  /* Stop consuming */
  for (i = 0 ; i < metadata->topics[0].partition_cnt ; i++) {
    int32_t partition = metadata->topics[0].partitions[i].id;

    /* If -p <part> was specified: skip unwanted partitions */
    if (conf.partition != RD_KAFKA_PARTITION_UA &&
        conf.partition != partition)
      continue;

    rd_kafka_consume_stop(conf.rkt, partition);
  }

  /* Destroy shared queue */
  rd_kafka_queue_destroy(rkqu);

  return conf.exitcode;
}
