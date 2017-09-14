export KAFKACAT_HOME="/app/kafkacat"

# write kafka ssl config vars to files
mkdir -p $KAFKACAT_HOME/env

echo "$KAFKA_TRUSTED_CERT" > $KAFKACAT_HOME/env/KAFKA_TRUSTED_CERT
echo "$KAFKA_CLIENT_CERT" > $KAFKACAT_HOME/env/KAFKA_CLIENT_CERT
echo "$KAFKA_CLIENT_CERT_KEY" > $KAFKACAT_HOME/env/KAFKA_CLIENT_CERT_KEY

export PATH="/app/bin:$PATH"