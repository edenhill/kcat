#!/bin/bash
#

set -e
source helpers.sh

topic=$(make_topic_name)

echo -n '{"topic":"sorted","partition":0,"offset":0,"tstype":"create","ts":1613826868173,"broker":0,"key":"TheKey","payload":"this is payload"}' | $KAFKACAT -P -t $topic -J

output=$($KAFKACAT -C -t $topic -o beginning -e -K "---")

exp="TheKey---this is payload"

if [[ $output != "$exp" ]]; then
    FAIL "Expected '$exp', not '$output'"
fi


topic=$(make_topic_name)

echo -n '{"topic":"sorted","partition":0,"offset":0,"tstype":"create","ts":1613826868173,"broker":0,"key":"TheKey","payload":"payload1"}
{"topic":"sorted","partition":0,"offset":0,"tstype":"create","ts":1613826868173,"broker":0,"key":"TheKey","payload":"payload2"}' | $KAFKACAT -P -t $topic -J

output=$($KAFKACAT -C -t $topic -o beginning -e -K "---")

exp="TheKey---payload1
TheKey---payload2"

if [[ $output != "$exp" ]]; then
    FAIL "Expected '$exp', not '$output'"
fi

PASS