kc(1) -- generic producer and consumer for Apache Kafka
=======================================================

Copyright (c) 2015, François Saint-Jacques
Copyright (c) 2014, Magnus Edenhill

https://github.com/fsaintjacques/kc

A custom fork of

https://github.com/edenhill/kafkacat

`kc` is a generic non-JVM producer and consumer for Apache Kafka 0.8,
think of it as a netcat for Kafka.

In `producer` mode `kc` reads messages from stdin, delimited with a
configurable delimeter (-D, defaults to newline), and produces them to the
provided Kafka cluster (-b), topic (-t) and partition (-p).

In `consumer` mode kc reads messages from a topic and partition and
prints them to stdout using the configured message delimiter.

`kc` also features a Metadata list command to display the current
state of the Kafka cluster and its topics and partitions.

`kc` is fast and lightweight; statically linked it is no more than 150Kb.

REQUIREMENTS
------------

 * librdkafka - https://github.com/edenhill/librdkafka

BUILD
-----

    ./configure <usual-configure-options>
    make
    sudo make install

QUICK BUILD
===========

The tools/bootstrap.sh build script will download and build the required
dependencies, providing a quick and easy means of building `kc`.
Internet connectivity and git is required by this script.
The resulting `kc` binary will be linked statically to avoid runtime
dependencies.

    tools/bootstrap.sh

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

DOCUMENATION
------------

See doc/kc.md for a complete documentation.
