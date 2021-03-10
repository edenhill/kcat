set -e

CLR_BGRED="\033[97;41m"
CLR_BGGREEN="\033[97;42m"
CLR_INFO="\033[34m"
CLR="\033[0m"

if [[ -z "$BROKERS" ]]; then
    echo -e "${CLR_BGRED}kafkacat tests requires \$BROKERS to be set${CLR}"
    exit 1
fi

KAFKACAT="../kafkacat -b $BROKERS"
TEST_NAME=$(basename $0 | sed -e 's/\.sh$//')

function make_topic_name {
    local name=$1
    echo "kafkacat_test_$$_${RANDOM}_${TEST_NAME}"
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
