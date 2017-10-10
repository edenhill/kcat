kafkacat-buildpack
========

Heroku buildpack for [kafkacat](https://github.com/edenhill/kafkacat), a command line based Apache Kafka producer and consumer.

Makes an auto-configured `kafkacat` command available on the dyno.

# Set Up

**Your app must have the [Apache Kafka on Heroku add-on](https://elements.heroku.com/addons/heroku-kafka) attached to use this buildpack.**

Add this buildpack as the first one:

```bash
heroku buildpacks:add \
  --app $APP_NAME \
  --index 1 \
  https://github.com/trevorscott/kafkacat-buildpack
```

✏️ *Replace `$APP_NAME` with the name for your specific Heroku app.*

If your app didn't already have a buildpack explicitly set, then it was relying on autodetect of an [officially supported buildpack](https://devcenter.heroku.com/articles/buildpacks#officially-supported-buildpacks). In that case, add the required language buildpack as the last one. Example: `heroku buildpacks:add heroku/nodejs`

# Default Config

The [Kafka add-on](https://elements.heroku.com/addons/heroku-kafka) creates four config vars:

 * `KAFKA_TRUSTED_CERT`
 * `KAFKA_CLIENT_CERT`
 * `KAFKA_CLIENT_CERT_KEY`
 * `KAFKA_URL`
 
This buildpack automatically configures `kafkacat` with these config vars, so it connects to a broker in the Kafka cluster with SSL by default. The configuration uses the first broker URL of the `KAFKA_URL` config var. (See the [.profile.d script](/.profile.d/000-kafkacat.sh) and the configured [`kafkacat` command](/bin/app/kafkacat) for more details.)

# Usage

Since our version of `kafkacat` comes preconfigured, you should omit SSL & broker configuration. 

For example, to read messages from the topic `your-topic`:

```
$ kafkacat -t your-topic
```

See the [kafkacat README](https://github.com/edenhill/kafkacat/blob/master/README.md) for more examples.
