kfc(1) -- generic producer and consumer for Apache Kafka
=======================================================

SYNOPSIS
--------

```
kfc producer [--brokers=<brks>] [--partition=<part>] [--compression=<comp>]
             [--delimiter=<delim>] [--key-delimiter=<delim>] [--count=<cnt>]
             [--error-file=<file>] [-T | --tee] [-q | --quiet]
             [-v | --verbose] <topic> [<file>...]

kfc consumer [--brokers=<brks>] [--partition=<part>] [--offset=<off>]
             [--delimiter=<delim>] [--key-delimiter=<delim>] [--count=<cnt>]
             [-e | --exit] [-O | --print-offset] [-u | --unbuffered]
             [-q | --quiet] [-v | --verbose] <topic>

kfc metadata [--brokers=<brks>] [--partition=<part>][-q | --quiet]
             [-v | --verbose] [<topic>]

kfc --help

kfc --version
```

DESCRIPTION
-----------

`kfc` also known as `kafkacat` is a generic non-JVM producer and consumer for
Apache Kafka 0.8, think of it as a netcat for Kafka.

`kfc` works in different <command>: producer, consumer and metadata. Each one
is described in the [COMMAND] section.

COMMAND
-------

* `producer`
  reads messages from stdin, delimited with a configurable delimeter and
  produces them to the provided Kafka cluster, topic and partition.

* `consumer`
  reads messages from a topic (and possibly a specific partition ) and prints
  them to stdout using the configured message delimiter.

* `metadata`
  displays the current state of the Kafka cluster and its topics and partitions.

OPTIONS
-------

### Generic options

* `-b <brokers>, --brokers=<brokers>`
  Comma separated list of broker(s) to bootstrap of the form
  "host[:port][,...]", e.g. "host1:9292,host2:9293" [default: localhost].

* `-p <partition>, --partition=<partition>`
  Send messages to a specific partition. If -1 is provided, a partition will
  be randomly uniformly selected for each message.

* `-v, --verbose`
  Augment verbosity level.

* `-q, --quiet`
  Quiet mode, do not report error messages to stderr.

* `-V, --version`
  Show the version on the first line and follow with the usage and help message.

* `-h, --help`
  Show the usage and help message.

### Producer options

* `-d <delim>, --delimiter=<delim>`
  Message delimiter character: a-z.. | \\r | \\n | \\t | \\xNN [default: \\n].


* `-k <delim>, --key-delimiter=<delim>`
  Key delimiter character: a-z.. | \\r | \\n | \\t | \\xNN.

* `-c <cnt>, --count=<cnt>`
  Exit after producing `<cnt>` messages.

* `-z <c>, --compression=<c>`
  Compression applied to messages: none, snappy or gzip [default: none].

* `-T, --tee`
  Output queued messages to stdout, acting like tee. A message is relayed to
  stdout once it hits librdkafka's internal queue, it can still fail to reach
  the producer. See `-E` for more information on failed messages.

* `-E <file>, --error-file=<file>`
  Messages that couldn't be sent are append to `<file>`, formatted according
  to delimiter and key delimiter (if provided). There is not guarantee
  on the ordering of the messages [default: stderr].

### Consumer options

* `-d <delim>, --delimiter=<delim>`
  Message delimiter character: a-z.. | \\r | \\n | \\t | \\xNN [default: \\n].

* `-k <delim>, --key-delimiter=<delim>`
  Key delimiter character: a-z.. | \\r | \\n | \\t | \\xNN.

* `-o <off>, --offset=<off>`
  Define the consuming offset, the possible values are:
    * `beginning`: start from the beginning
    * `end`: start from the end
    * `stored`: start from a stored value
    * `<value>`: start at absolute <value>
    * `-<value>`: start at relative offset from the end (acting like `tail`).

* `-c <cnt>, --count=<cnt>`
  Exit after consuming `<cnt>` messages.

* `-e , --exit`
  Exit when there is no message left to consumer in the topic.

* `-u , --unbuffered`
  Do not buffer stdout, useful for low volume topic.

EXAMPLES
--------

* Producing a single message

```
    $ echo "test message" | kfc producer test
```

* Consuming last message

```
    $ kfc consumer -e -o -1 test
    test message
```

* Showing metadata on the test topic

```
    $ kfc metadata test
    {
      "brokers": [
        { "id":0, "host":"localhost:9092" }
      ],
      "topics": [
        {
          "name": "test",
          "partitions": [
            { "id": 0, "leader": 0, "replicas": [0], "isrs": [0] }
          ]
        }
      ]
    }
```
