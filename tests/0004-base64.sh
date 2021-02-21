#!/bin/bash
#

set -e
source helpers.sh

topic=$(make_topic_name)

echo -n 'VGVzdA==' | $KAFKACAT -P -t $topic -B v

output=$($KAFKACAT -C -t $topic -o beginning -e -K "---")

exp="---Test"

if [[ $output != "$exp" ]]; then
    FAIL "Expected '$exp', not '$output'"
fi


topic=$(make_topic_name)

echo -n 'VGVzdA==---VGVzdA==' | $KAFKACAT -P -t $topic -K "---" -B a

output=$($KAFKACAT -C -t $topic -o beginning -e -K "---")

exp="Test---Test"

if [[ $output != "$exp" ]]; then
    FAIL "Expected '$exp', not '$output'"
fi

PASS