kafkacat-buildpack
========

Heroku buildpack for [kafkacat](https://github.com/edenhill/kafkacat), a command line based Apache Kafka producer and consumer.

# Set Up

** This buildpack requires a heroku app with the heroku kafka addon.

Add the buildpack to your app:

```
 heroku buildpacks:set https://github.com/trevorscott/kafkacat-buildpack -a your-app
```

# Default Config

Your heroku app's kafka addon creates four config vars:

 * KAFKA_TRUSTED_CERT
 * KAFKA_CLIENT_CERT
 * KAFKA_CLIENT_CERT_KEY
 * KAFKA_URL
 
This buildpack configures kafkacat with these config vars so it connects to a broker in your kafka cluster with SSL by default. We configure kafkacat with the first broker url provided in the `KAFKA_URL` config var. (See the [.profile.d script](/.profile.d/000-kafkacat.sh) and the configured [kafkacat binary](/bin/app/kafkacat) for more details)

# Usage

Since our version of kafkacat comes preconfigured, you can omit kafkacat SSL & broker configuration. 

For example, to read messages from the topic `your-kafka-topic`:

```
$ kafkacat -t your-kafka-topic
```

See the [kafkacat README](https://github.com/edenhill/kafkacat) for more examples.
