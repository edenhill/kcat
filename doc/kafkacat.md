kafkacat(1) -- generic producer and consumer for Apache Kafka
=============================================================

SYNOPSIS
--------

`kafkacat` -C [generic options] [-o offset] [-e] [-O] [-u]
           -b brokers -t topic

`kafkacat` -P [generic options] [-z snappy | gzip] [-p -1] [file [...]]
           -b brokers -t topic

`kafkacat` -L [generic options] [-t topic]

DESCRIPTION
-----------

`kafkacat` is a generic non-JVM producer and consumer for Apache Kafka 0.8,
think of it as a netcat for Kafka.

In producer mode ( -P ), `kafkacat` reads messages from stdin, delimited with a
configurable delimeter and produces them to the provided Kafka cluster, topic
and partition. In consumer mode ( -C ), `kafkacat` reads messages from a topic
and partition and prints them to stdout using the configured message delimiter.

If neither -P or -C are specified `kafkacat` attempts to figure out the mode
automatically based on stdin/stdout tty types.

`kafkacat` also features a metadata list mode ( -L ), to display the
current state of the Kafka cluster and its topics and partitions.

OPTIONS
-------

### Generic Options

* `-t <topic>`
  Topic to consume from, produce to, or list.

* `-b <brokers>`
  Comma separated list of broker(s) to bootstrap of the form
  "host[:port][,...]", e.g. "host1:9292,host2:9293".

* `-p <partition>`
  Partition.

* `-D <delim>`
  Message delimiter character: a-z.. | \\r | \\n | \\t | \\xNN [default: \\n].


* `-K <delim>`
  Key delimiter character: a-z.. | \\r | \\n | \\t | \\xNN.

* `-c <count>`
  Limit message count. For the producer, exit after producing `<count>`
  messages. For the consumer, exit after consuming `<count>` messages.

* `-X list|dump|<prop>=<val>`

  * `list` will list available librdkafka configuration properties.

  * `dump` will dump the configuration and exit.

  * `<prop>=<val>` will set the property <prop> to value <val>. Properties
    prefixed with "topic" are applied as topic properties.

* `-d <dbg>`
  Enable librdkafka debugging. List of comma separated labels. See librdkafka
  documentation for supported values.

* `-q`
  Quiet mode (set verbosity to 0).

* `-v`
  Increase verbosity, can be called multiple time.

### Producer Options

* `-z snappy|gzip`
  Message compression [default: none].

* `-p -1`
  Use random partitionner.

* `-T`
  Output sent messages to stdout, acting like tee.

### Consumer Options

* `-o beginning|end|stored|<offset>|-<offset>`
  Offset to start consuming from.

  * `beginning` will start at the beginning.

  * `end` will start at the end.

  * `stored` will start at the last stored offset.

  * `<offset>` will start at absolute offset.

  * `-<offset>` will start at relative offset from the end.

* `-e`
  Exit consuming after last message received.

* `-O`
  Print message offset using `-K` delimiter.
