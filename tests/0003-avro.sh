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

if ! $KCAT -V | grep -q ^Version.*Avro.*builtin\.features; then
    SKIP "kcat not built with Avro support"
fi

topic=$(make_topic_name)

create_topic $topic 1

info "Producing Avro message to $topic"

value='{"number": 63, "name": "TestName"}'
echo $value | \
    docker run --network=host -i \
           confluentinc/cp-schema-registry:6.1.0 \
           kafka-avro-console-producer \
           --bootstrap-server $BROKERS \
           --topic $topic \
           --property schema.registry.url="$SR_URL" \
           --property value.schema="$(< basic_schema.avsc)"

info "Producing keyed Avro message to $topic"
key='"An Avro Key"'
echo "$key:$value" |
    docker run --network=host -i \
           confluentinc/cp-schema-registry:6.1.0 \
           kafka-avro-console-producer \
           --bootstrap-server $BROKERS \
           --topic $topic \
           --property schema.registry.url="$SR_URL" \
           --property value.schema="$(< basic_schema.avsc)" \
           --property parse.key=true \
           --property key.separator=: \
           --property key.schema="$(< primitive_string.avsc)"


info "Reading Avro messages without key deserializer"
output=$($KCAT -C -r $SR_URL -t $topic -o beginning -e -D\; -s value=avro)
exp="$value;$value;"
if [[ $output != $exp ]]; then
    echo "FAIL: Expected '$exp', not '$output'"
    exit 1
fi

info "Reading Avro messages with key deserializer"
output=$($KCAT -C -r $SR_URL -t $topic -o beginning -e -D\; -s value=avro -s key=avro)
exp="$value;$key$value;"
if [[ $output != $exp ]]; then
    echo "FAIL: Expected '$exp', not '$output'"
    exit 1
fi


info "Verifying first message's JSON with jq"
output=$($KCAT -C -r $SR_URL -t $topic -o beginning -c 1 -s value=avro | \
             jq -r '(.name + "=" + (.number | tostring))')
exp="TestName=63"



PASS "Expected output seen: $output"
