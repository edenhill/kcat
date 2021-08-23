set -e

CLR_BGRED="\033[37;41m"
CLR_BGGREEN="\033[37;42m"
CLR_YELLOW="\033[33m"
CLR_INFO="\033[34m"
CLR="\033[0m"

if [[ -z "$BROKERS" ]]; then
    echo -e "${CLR_BGRED}kcat tests requires \$BROKERS to be set${CLR}"
    exit 1
fi

KCAT="../kcat -b $BROKERS"
TEST_NAME=$(basename $0 | sed -e 's/\.sh$//')

function make_topic_name {
    local name=$1
    echo "kcat_test_$$_${RANDOM}_${TEST_NAME}name"
}

function create_topic {
    local topic=$1
    local partitions=$2
    info "Creating topic $topic with $partitions partition(s)"
    $KAFKA_PATH/bin/kafka-topics.sh \
        --bootstrap-server $BROKERS \
        --create \
        --topic "$topic" \
        --partitions $partitions \
        --replication-factor 1
}


function info {
    local str=$1
    echo -e "${CLR_INFO}${TEST_NAME} | $str${CLR}"
}

function FAIL {
    local str=$1
    echo -e "${CLR_BGRED}${TEST_NAME} | TEST FAILED: $str${CLR}"
    exit 1
}

function PASS {
    local str=$1
    echo -e "${CLR_BGGREEN}${TEST_NAME} | TEST PASSED: $str${CLR}"
}


function SKIP {
    local str=$1
    echo -e "${CLR_YELLOW}${TEST_NAME} | TEST SKIPPED: $str${CLR}"
    exit 0
}
