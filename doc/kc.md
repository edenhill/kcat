kc(1) -- generic producer and consumer for Apache Kafka
=======================================================

SYNOPSIS
--------

```
kc producer [--brokers=<brks>] [--partition=<part>] [--compression=<comp>]
            [--delimiter=<delim>] [--key-delimiter=<delim>] [--count=<cnt>]
            [-T | --tee] [-q | --quiet] [-v | --verbose] <topic> [<file>...]

kc consumer [--brokers=<brks>] [--partition=<part>] [-e | --exit]
            [--delimiter=<delim>] [--key-delimiter=<delim>] [--count=<cnt>]
            [-q | --quiet] [-v | --verbose] <topic>

kc metadata [--brokers=<brks>] [--partition=<part>] [<topic>]

kc --help

kc --version
```

DESCRIPTION
-----------

`kc` also known as `kafkacat` is a generic non-JVM producer and consumer for
Apache Kafka 0.8, think of it as a netcat for Kafka.

`kc` works in different _command_: producer, consumer and metadata. Each one
is described in the [COMMAND] section.

COMMAND
-------

* `producer`
  reads messages from stdin, delimited with a configurable delimeter and
  produces them to the provided Kafka cluster, topic and partition. See
  [PRODUCER] section for more information on producer.

* `consumer`
  reads messages from a topic (and possibly a specific partition ) and prints
  them to stdout using the configured message delimiter.  See
  [CONSUMER] section for more information on consumer.

* `metadata`
  displays the current state of the Kafka cluster and its topics and partitions.
  See [METADATA] for more information on metadata.

PRODUCER
--------

CONSUMER
--------

METADATA
--------

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
  Compression applied to messages: snappy or gzip.

* `-T, --tee`
  Output queued messages to stdout, acting like tee. Note that this does not
  guarantee that the message was correctly sent.

### Consumer options

* `-d <delim>, --delimiter=<delim>`
  Message delimiter character: a-z.. | \\r | \\n | \\t | \\xNN [default: \\n].


* `-k <delim>, --key-delimiter=<delim>`
  Key delimiter character: a-z.. | \\r | \\n | \\t | \\xNN.

* `-o <off>, --offset=<off>`
  Exit after consumer `<cnt>` messages.

* `-c <cnt>, --count=<cnt>`
  Exit after consumer `<cnt>` messages.

* `-e , --exit`
  Exit when there is no message left to consumer in the topic.

CONFIGURATION
-------------

### General configuration

### Topic configuration

EXAMPLES
--------

* Producing a single message

```
    $ echo "test message" | kc producer test
```

* Consuming last message

```
    $ kc consumer -e -o -1 test
    test message
```

* Showing metadata on the test topic

```
    $ kc metadata test
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

DISCUSSION
----------
