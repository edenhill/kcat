kafkacat
========
Copyright (c) 2014, Magnus Edenhill

https://github.com/edenhill/kafkacat

**kafkacat** is a generic producer and consumer for Apache Kafka 0.8,
think of it as a netcat for Kafka.

In **producer** mode kafkacat reads messages from stdin, delimited with a
configurable delimeter (-D, defaults to newline), and produces them to the
provided Kafka cluster (-b), topic (-t) and partition (-p).

In **consumer** mode kafkacat reads messages from a topic and partition and
prints them to stdout using the configured message delimiter.

kafkacat also features a Metadata list (-L) mode to display the current
state of the Kafka cluster and its topics and partitions.




# Requirements

 * librdkafka - https://github.com/edenhill/librdkafka


# Build

    ./configure <usual-configure-options>
    make
    sudo make install


# Examples

## Read messages from stdin, produce to 'syslog' topic with snappy compression

    tail -f /var/log/syslog | kafkacat -b mybroker -t syslog -p 0 -z snappy

## Read messages from Kafka 'syslog' topic, print to stdout

    kafkacat -b mybroker -t syslog -p 0

## Metadata listing

    kafkacat -L -b mybroker
