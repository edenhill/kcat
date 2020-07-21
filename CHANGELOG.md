# kafkacat v1.6.0

 * Transactional Producer support (see below).
 * Honour `-k <key>` when producing files (#197).
 * Honour `-o <offset>` in `-G` high-level consumer mode (#231).
 * Added `-m <seconds>` argument to set metadata/query/transaction timeouts.
 * Allow `schema.registry.url` to be configured in config file and
   not only by `-r` (#220).
 * Print broker-id message was produced to (if `-v`),
   or was consumed from (if `-J`).

## Apache Kafke EOS / Transactional Producer support

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

See https://github.com/edenhill/kafkacat/releases
