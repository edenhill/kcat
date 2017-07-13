kafkacat
========
Copyright (c) 2014-2016 Magnus Edenhill

[https://github.com/edenhill/kafkacat](https://github.com/edenhill/kafkacat)

**kafkacat** is a generic non-JVM producer and consumer for Apache Kafka >=0.8,
think of it as a netcat for Kafka.

In **producer** mode kafkacat reads messages from stdin, delimited with a
configurable delimeter (-D, defaults to newline), and produces them to the
provided Kafka cluster (-b), topic (-t) and partition (-p).

In **consumer** mode kafkacat reads messages from a topic and partition and
prints them to stdout using the configured message delimiter.

There's also support for the Kafka >=0.9 high-level balanced consumer, use
the `-G <group>` switch and provide a list of topics to join the group.

kafkacat also features a Metadata list (-L) mode to display the current
state of the Kafka cluster and its topics and partitions.

kafkacat is fast and lightweight; statically linked it is no more than 150Kb.


# Install

On recent enough Debian systems:

````
apt-get install kafkacat
````

And on Mac OS X with homebrew installed:

````
brew install kafkacat
````

Otherwise follow directions below.


# Requirements

 * librdkafka - https://github.com/edenhill/librdkafka
 * libyajl (for JSON support, optional)

On Ubuntu or Debian: `sudo apt-get install librdkafka-dev libyajl-dev`

# Build

    ./configure <usual-configure-options>
    make
    sudo make install

# Quick build

The bootstrap.sh build script will download and build the required dependencies,
providing a quick and easy means of building kafkacat.
Internet connectivity and wget/curl is required by this script.
The resulting kafkacat binary will be linked statically to avoid runtime
dependencies.
**NOTE**: Requires `curl` and `cmake` (for yajl) to be installed.

    ./bootstrap.sh


# Examples

High-level balanced KafkaConsumer: subscribe to topic1 and topic2
(requires broker >=0.9.0 and librdkafka version >=0.9.1)

    $ kafkacat -b mybroker -G mygroup topic1 topic2


Read messages from stdin, produce to 'syslog' topic with snappy compression

    $ tail -f /var/log/syslog | kafkacat -b mybroker -t syslog -z snappy


Read messages from Kafka 'syslog' topic, print to stdout

    $ kafkacat -b mybroker -t syslog


Produce messages from file (one file is one message)

    $ kafkacat -P -b mybroker -t filedrop -p 0 myfile1.bin /etc/motd thirdfile.tgz

Read the last 2000 messages from 'syslog' topic, then exit

    $ kafkacat -C -b mybroker -t syslog -p 0 -o -2000 -e


Consume from all partitions from 'syslog' topic

    $ kafkacat -C -b mybroker -t syslog


Output consumed messages in JSON envelope:

    $ kafkacat -b mybroker -t syslog -J


Output consumed messages according to format string:

    $ kafkacat -b mybroker -t syslog -f 'Topic %t[%p], offset: %o, key: %k, payload: %S bytes: %s\n'

Read the last 100 messages from topic 'syslog' with  librdkafka configuration parameter 'broker.version.fallback' set to '0.8.2.1' :

    $ kafkacat -C -b mybroker -X broker.version.fallback=0.8.2.1 -t syslog -p 0 -o -100 -e

Metadata listing

````
$ kafkacat -L -b mybroker
Metadata for all topics (from broker 1: mybroker:9092/1):
 3 brokers:
  broker 1 at mybroker:9092
  broker 2 at mybrokertoo:9092
  broker 3 at thirdbroker:9092
 16 topics:
  topic "syslog" with 3 partitions:
    partition 0, leader 3, replicas: 1,2,3, isrs: 1,2,3
    partition 1, leader 1, replicas: 1,2,3, isrs: 1,2,3
    partition 2, leader 1, replicas: 1,2, isrs: 1,2
  topic "rdkafkatest1_auto_49f744a4327b1b1e" with 2 partitions:
    partition 0, leader 3, replicas: 3, isrs: 3
    partition 1, leader 1, replicas: 1, isrs: 1
  topic "rdkafkatest1_auto_e02f58f2c581cba" with 2 partitions:
    partition 0, leader 3, replicas: 3, isrs: 3
    partition 1, leader 1, replicas: 1, isrs: 1
  ....
````

JSON metadata listing

    $ kafkacat -b mybroker -L -J

Pretty-printed JSON metadata listing

    $ kafkacat -b mybroker -L -J | jq .


Query offset(s) by timestamp(s)

    $ kafkacat -b mybroker -Q -t mytopic:3:2389238523  mytopic2:0:18921841
