#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

typedef enum {
  UNKNOWN,
  PRODUCER,
  CONSUMER,
  METADATA,
} kc_command;

int producer_main(int argc, char **argv);
int consumer_main(int argc, char **argv);
int metadata_main(int argc, char **argv);

static struct option kc_long_options[] = {
    {"help",    no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {0,         0,           0,  0 }
};

static kc_command argparse (int argc, char **argv) {
  int opt;
  int option_index = 0;

  /* Prefixing the optstring with '+' will have the effect of
   * stopping getopt_long on the first non option. In our case, this
   * would be the command. */
  while ((opt = getopt_long(argc, argv,
                            "+Vh",
                            kc_long_options,
                            &option_index)) != -1) {
    switch (opt) {
    case 'V':
      usage(argv[0], 0, "2.0.0-beta");
      break;
    case 'h':
      usage(argv[0], 0, NULL);
      break;
    default:
      usage(argv[0], 1, NULL);
      break;
    }
  }

  if (argc - optind == 0)
    usage(argv[0], 1, "Command missing");

  const char *cmd = argv[optind];
  if (!strcmp("producer", cmd))
    return PRODUCER;
  else if (!strcmp("consumer", cmd))
    return CONSUMER;
  else if (!strcmp("metadata", cmd))
    return METADATA;

  return UNKNOWN;
}

int main(int argc, char **argv) {
  int    left_argc;
  char **left_argv;

  set_signals();

  /* Create config containers */
  conf.rk_conf  = rd_kafka_conf_new();
  conf.rkt_conf = rd_kafka_topic_conf_new();

  kc_command cmd = argparse(argc, argv);

  left_argc = argc - optind;
  left_argv = &argv[optind];

  /* Execute command */
  switch (cmd) {
  case PRODUCER:
    producer_main(left_argc, left_argv);
    break;
  case CONSUMER:
    consumer_main(left_argc, left_argv);
    break;
  case METADATA:
    metadata_main(left_argc, left_argv);
    break;
  default:
    usage(argv[0], 1, "Unknown subcommand");
  }

  /* Be warned that changing conf.run is highly risky. */
  conf.run = 1;

  /* Wait for all messages to be transmitted */
  while (conf.run && rd_kafka_outq_len(conf.rk))
    rd_kafka_poll(conf.rk, 50);

  if (conf.rkt) {
    rd_kafka_topic_destroy(conf.rkt);
    conf.rkt = NULL;
  }

  if (conf.rk) {
    rd_kafka_destroy(conf.rk);
    conf.rk = NULL;
  }

  rd_kafka_wait_destroyed(5000);

  return conf.exitcode;
}
