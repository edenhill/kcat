#!/bin/bash
#

set -e
source helpers.sh


#
# Verify that single and multi-byte delimiters work.
#


topic=$(make_topic_name)


# Multi-byte delimiters

echo -n "Key1;KeyDel;Value1:MyDilemma::MyDilemma:;KeyDel;Value2:MyDilemma:Key3;KeyDel;:MyDilemma:Value4" |
    $KAFKACAT -t $topic -K ';KeyDel;' -D ':MyDilemma:' -Z


output=$($KAFKACAT -C -t $topic -o beginning -e -J |
             jq -r '.key + "=" + .payload')

exp="Key1=Value1
=Value2
Key3=
=Value4"

if [[ $output != $exp ]]; then
    FAIL "Expected '$exp', not '$output'"
fi

topic=$(make_topic_name)
#
# Single-byte delimiters, partition 1
#

echo "The First;Message1

Is The;
;Greatest
For sure" |
    $KAFKACAT -t $topic -K ';' -Z


output=$($KAFKACAT -C -t $topic -o beginning -e -J |
             jq -r '.key + "=" + .payload')

exp="The First=Message1
Is The=
=Greatest
=For sure"

if [[ $output != $exp ]]; then
    FAIL "Expected '$exp', not '$output'"
fi

PASS
