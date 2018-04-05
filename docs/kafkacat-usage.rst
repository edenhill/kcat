.. _kafkacat-usage:

kafkacat
========

kafkacat is a command line tool for working with Kafka. Described as "netcat for Kafka", it is a swiss-army knife of tools for inspecting and creating data in Kafka.

If you already know ``kafka-console-producer`` and ``kafka-console-consumer`` then it's like those, on steroids.

kafkacat is an open-source tool, available at https://github.com/edenhill/kafkacat. It is not included in Confluent Platform.

kafkacat the Consumer
-------------------------

Give ``kafkacat`` a broker and a topic, and you'll see its contents:

.. code:: shell

    $ kafkacat -b localhost:9092 -t mysql_users
    % Auto-selecting Consumer mode (use -P or -C to override)
    {"uid":1,"name":"Cliff","locale":"en_US","address_city":"St Louis","elite":"P"}
    {"uid":2,"name":"Nick","locale":"en_US","address_city":"Palo Alto","elite":"G"}
    [...]


Note: ``kafkacat`` doesn't currently support Avro.

``kafkacat`` defaults to being a consumer, but you can also make this explicit with the ``-C`` flag. You can also specify how many messages you want to see with the lowercase ``-c`` and a number. For example:

.. code:: shell

    $ kafkacat -b localhost:9092 -t mysql_users -C -c1
    {"uid":1,"name":"Cliff","locale":"en_US","address_city":"St Louis","elite":"P"}

Seeing the value of the message is useful, but Kafka messages also have a key, which ``kafkacat`` can display with the ``-K`` argument also supplying the delimiter:

.. code:: shell

    $ kafkacat -b localhost:9092 -t mysql_users -C -c1 -K\t
    1   {"uid":1,"name":"Cliff","locale":"en_US","address_city":"St Louis","elite":"P"}

The ``-f`` flag takes arguments specifying both the format of the output and the fields to include. Here's a simple example of pretty-printing the key/value pairs for each message:

.. code:: shell

    $ kafkacat -b localhost:9092 -t mysql_users -C -c1 -f 'Key: %k\nValue: %s\n'
    Key: 1
    Value: {"uid":1,"name":"Cliff","locale":"en_US","address_city":"St Louis","elite":"P"}

Note that the ``-K:`` was replaced because the key parameter is specified in the ``-f`` format string now.

A more advanced use of ``-f`` would be to show even more metadata - offsets, timestamps, and even data lengths:

.. code:: shell

    $ kafkacat -b localhost:9092 -t mysql_users -C -c2 -f '\nKey (%K bytes): %k\t\nValue (%S bytes): %s\nTimestamp: %T\tPartition: %p\tOffset: %o\n--\n'

    Key (1 bytes): 1
    Value (79 bytes): {"uid":1,"name":"Cliff","locale":"en_US","address_city":"St Louis","elite":"P"}
    Timestamp: 1520618381093        Partition: 0    Offset: 0
    --

    Key (1 bytes): 2
    Value (79 bytes): {"uid":2,"name":"Nick","locale":"en_US","address_city":"Palo Alto","elite":"G"}
    Timestamp: 1520618381093        Partition: 0    Offset: 1
    --

kafkacat the Producer
-------------------------

You can easily send data to a topic using ``kafkacat``. Run it with the ``-P`` command and enter the data you want, and then press ``Ctrl-D`` to finish:

.. code:: shell

    $ kafkacat -b localhost:9092 -t new_topic -P
    test

Replay it (replace ``-P`` with ``-C``) to verify:

.. code:: shell

    $ kafkacat -b localhost:9092 -t new_topic -C
    test

You can send data to ``kafkacat`` by adding data from a file (``-l``), and using the ``-T`` flag to also echo the input to ``stdout``:

.. code:: shell

    $ kafkacat -b localhost:9092 -t my_topic -T -P -l /tmp/msgs
    This is
    three messages
    sent through kafkacat

You can specify the key for messages, using the same ``-K`` parameter plus delimiter character that was used for the previous consumer example:

.. code:: shell

    $ kafkacat -b localhost:9092 -t keyed_topic -P -K:
    1:foo
    2:bar

    $ kafkacat -b localhost:9092 -t keyed_topic -C -f 'Key: %k\nValue: %s\n'
    Key: 1
    Value: foo
    Key: 2
    Value: bar

You can set the partition:

.. code:: shell

    $ kafkacat -b localhost:9092 -t partitioned_topic -P -K: -p 1
    1:foo
    $ kafkacat -b localhost:9092 -t partitioned_topic -P -K: -p 2
    2:bar
    $ kafkacat -b localhost:9092 -t partitioned_topic -P -K: -p 3
    3:wibble

Replay, using the format and ``-%`` field as above:

.. code:: shell

    $ kafkacat -b localhost:9092 -t partitioned_topic -C -f '\nKey (%K bytes): %k\t\nValue (%S bytes): %s\nTimestamp: %T\tPartition: %p\tOffset: %o\n--\n'
    % Reached end of topic partitioned_topic [0] at offset 0

    Key (1 bytes): 1
    Value (3 bytes): foo
    Timestamp: 1520620113485        Partition: 1    Offset: 0
    --

    Key (1 bytes): 2
    Value (3 bytes): bar
    Timestamp: 1520620121165        Partition: 2    Offset: 0
    --

    Key (1 bytes): 3
    Value (6 bytes): wibble
    Timestamp: 1520620129112        Partition: 3    Offset: 0
    --
