# kcat v1.8.0

 * Added new mock cluster mode
   `kcat -M <broker-cnt>` spins up a mock cluster that applications
   can produce to and consume from.
 * Mixing modes is now prohibited (e.g., `-P .. -C`).
 * Producer: Fix stdin buffering: no messages would be produced
   until Ctrl-D or at least 1024 bytes had accumulated (#343).



# kcat v1.7.1

The edenhill/kcat image has been updated to librdkafka v1.8.2.


# kcat v1.7.0

**kafkacat has been renamed to kcat** to adhere to the
Apache Software Foundation's (ASF) trademark policies.

 * `kafkacat` is now called `kcat`.
 * Add support for multibyte delimiters to `-D` and `-K` (#140, #280)
 * Add support for `-X partition.assignment.strategy=cooperative-sticky` incremental rebalancing.
 * High-level consumer `-G` now supports exit-on-eof `-e` option (#86)
 * Avro consumer with -J will now emit `key_schema_id` and `value_schema_id`.

## Upgrade considerations

 * Please rename any `kafkacat.conf` config files to `kcat.conf`.
   The old path will continue to work for some time but a warning will be
   printed.
 * Please rename any `KAFKACAT_CONF` environment variables to `KCAT_CONF`.
   The old environment variable will continue to work for some time but a
   warning will be printed.



# kafkacat v1.6.0

 * Transactional Producer support (see below).
 * Honour `-k <key>` when producing files (#197).
 * Honour `-o <offset>` in `-G` high-level consumer mode (#231).
 * Added `-m <seconds>` argument to set metadata/query/transaction timeouts.
 * Allow `schema.registry.url` to be configured in config file and
   not only by `-r` (#220).
 * Print broker-id message was produced to (if `-v`),
   or was consumed from (if `-J`).

## Apache Kafka EOS / Transactional Producer support

Messages can now be produced in a single transaction if `-X transactional.id=..`
is passed to the producer in `-P` mode.

If kafkacat is terminated through Ctrl-C (or other signal) the transaction
will be aborted, while normal termination (due to stdin closing or after reading
all supplied files) will commit the transaction.

```bash
$ kafkacat -b $BROKERS -P -t mytopic -X transactional.id=myproducerapp
% Using transactional producer
This is a transactional message
And so is this
:)
[Press Ctrl-D]
% Committing transaction
% Transaction successfully committed
```


# Older releases

See https://github.com/edenhill/kcat/releases
