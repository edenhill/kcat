#!/bin/bash
#

set -e
source helpers.sh


#
# Verify Avro consumer, requires docker and a running trivup cluster with
# Kafka and Schema-registry.
#


if [[ -z $SR_URL ]]; then
    SKIP "No schema-registry available (SR_URL env not set)"
fi

if ! $KAFKACAT -V | grep -q ^Version.*Avro.*builtin\.features; then
    SKIP "Kafkacat not built with Avro support"
fi

topic=$(make_topic_name)

create_topic $topic 1

info "Producing Avro message to $topic"

echo '{"number": 63, "name": "TestName"}' |
    docker run --network=host -i \
           confluentinc/cp-schema-registry:6.0.0 \
           kafka-avro-console-producer \
           --bootstrap-server $BROKERS \
           --topic $topic \
           --property schema.registry.url="$SR_URL" \
           --property value.schema="$(< basic_schema.avsc)"

info "Reading Avro messages"
output=$($KAFKACAT -C -r $SR_URL -t $topic -o beginning -e -s value=avro | \
             jq -r '(.name + "=" + (.number | tostring))')

exp="TestName=63"

if [[ $output != $exp ]]; then
    echo "FAIL: Expected '$exp', not '$output'"
    exit 1
fi


PASS "Expected output seen: $output"
