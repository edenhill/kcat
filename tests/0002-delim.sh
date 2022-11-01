#!/bin/bash
#

set -e
source helpers.sh


#
# Verify that single and multi-byte delimiters work.
#


topic=$(make_topic_name)



# Multi-byte delimiters, partition 0

echo -n "Key1;KeyDel;Value1:MyDilemma::MyDilemma:;KeyDel;Value2:MyDilemma:Key3;KeyDel;:MyDilemma:Value4" |
    $KCAT -t $topic -p 0 -K ';KeyDel;' -D ':MyDilemma:' -Z


output=$($KCAT -C -t $topic -p 0 -o beginning -e -J |
             jq -r '.key + "=" + .payload')

exp="Key1=Value1
=Value2
Key3=
=Value4"

if [[ $output != $exp ]]; then
    echo "FAIL: Expected '$exp', not '$output'"
    exit 1
fi


#
# Single-byte delimiters, partition 1
#

echo "The First;Message1

Is The;
;Greatest
For sure" |
    $KCAT -t $topic -p 1 -K ';' -Z


output=$($KCAT -C -t $topic -p 1 -o beginning -e -J |
             jq -r '.key + "=" + .payload')

exp="The First=Message1
Is The=
=Greatest
=For sure"

if [[ $output != $exp ]]; then
    echo "FAIL: Expected '$exp', not '$output'"
    exit 1
fi


