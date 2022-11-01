#!/bin/bash
#

set -e

source helpers.sh


#
# Verify that -e (exit on eof) works on both low-level and high-level consumer
#


topic=$(make_topic_name)

# Produce some messages to topic
info "Priming producer for $topic"
seq 1 10 | $KCAT -t $topic


# Legacy: Consume messages without -e, should timeout.
info "Legacy consumer without -e"
timeout 5 $KCAT -t $topic -o beginning && \
    FAIL "Legacy consumer without -e should have timed out"


# Balanced: Consume messages without -e, should timeout.
info "Balanced consumer without -e"
timeout 10 $KCAT -G $topic -o beginning $topic && \
    FAIL "Balanced consumer without -e should have timed out"


# Legacy: Consume messages with -e, must not timeout
info "Legacy consumer with -e"
timeout 5 $KCAT -t $topic -o beginning -e || \
    FAIL "Legacy consumer with -e timed out"


# Balanced: Consume messages without -e, should timeout.
info "Balanced consumer with -e"
timeout 10 $KCAT -G ${topic}_new -o beginning -e $topic || \
    FAIL "Balanced consumer with -e timed out"

PASS



