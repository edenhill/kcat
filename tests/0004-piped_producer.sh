#!/bin/bash
#

set -e
source helpers.sh


#
# Verify that piping messages to the producer works.
#


topic=$(make_topic_name)



echo "msg1" | $KCAT -P -t $topic
