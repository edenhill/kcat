kafkacat
========
Copyright (c) 2014, Magnus Edenhill

https://github.com/edenhill/kafkacat

**kafkacat** is a generic non-JVM producer and consumer for Apache Kafka 0.8,
think of it as a netcat for Kafka.

In **producer** mode kafkacat reads messages from stdin, delimited with a
configurable delimeter (-D, defaults to newline), and produces them to the
provided Kafka cluster (-b), topic (-t) and partition (-p).

In **consumer** mode kafkacat reads messages from a topic and partition and
prints them to stdout using the configured message delimiter.

kafkacat also features a Metadata list (-L) mode to display the current
state of the Kafka cluster and its topics and partitions.

kafkacat is fast and lightweight; statically linked it is no more than 150Kb.



# Requirements

 * librdkafka - https://github.com/edenhill/librdkafka


# Build

    ./configure <usual-configure-options>
    make
    sudo make install

# Quick build

The bootstrap.sh build script will download and build the required dependencies,
providing a quick and easy means of building kafkacat.
Internet connectivity and wget is required by this script.
The resulting kafkacat binary will be linked statically to avoid runtime
dependencies.

    ./bootstrap.sh


# Examples

### Read messages from stdin, produce to 'syslog' topic with snappy compression

    $ tail -f /var/log/syslog | kafkacat -b mybroker -t syslog -p 0 -z snappy

### Read messages from Kafka 'syslog' topic, print to stdout

    $ kafkacat -b mybroker -t syslog -p 0

### Metadata listing

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
