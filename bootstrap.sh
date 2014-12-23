#!/bin/bash
#
# This script provides a quick build alternative:
# * Dependencies are downloaded and built automatically
# * kafkacat is built automatically.
# * kafkacat is linked statically to avoid runtime dependencies.
#
# While this might not be the preferred method of building kafkacat, it
# is the easiest and quickest way.
#

set -o errexit -o nounset -o pipefail

LIBRDKAFKA_VERSION=${LIBRDKAFKA_VERSION:-master}
LIBRDKAFKA_DIR=librdkafka-${LIBRDKAFKA_VERSION}
LIBRDKAFKA_URL=https://github.com/edenhill/librdkafka/archive/${LIBRDKAFKA_VERSION}.tar.gz

mkdir -p tmp-bootstrap
pushd tmp-bootstrap

if [[ ! -d ${LIBRDKAFKA_DIR} ]]; then
    echo "Downloading ${LIBRDKAFKA_DIR}"
    if which wget 2>&1 > /dev/null; then
        DL='wget -q -O-'
    else
        DL='curl -s -L'
    fi
    $DL "${LIBRDKAFKA_URL}" | tar xzf -
fi

echo "Building ${LIBRDKAFKA_DIR}"
pushd ${LIBRDKAFKA_DIR}
./configure
make
make DESTDIR="${PWD}/../" install

popd
popd

echo "Building kafkacat"
export CPPFLAGS="${CPPFLAGS:-} -Itmp-bootstrap/usr/local/include"
export LDFLAGS="${LDFLAGS:-} -Ltmp-bootstrap/usr/local/lib"
export STATIC_LIB_rdkafka="tmp-bootstrap/usr/local/lib/librdkafka.a"
./configure --enable-static
make

echo ""
echo "Success! kafkacat is now built"
echo ""
