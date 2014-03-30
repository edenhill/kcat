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

LIBRDKAFKA_VERSION=master

mkdir -p tmp-bootstrap || exit 1
cd tmp-bootstrap || exit 1

if [[ ! -d librdkafka-$LIBRDKAFKA_VERSION ]]; then
    echo "Downloading librdkafka-$LIBRDKAFKA_VERSION"
    wget -O- "https://github.com/edenhill/librdkafka/archive/${LIBRDKAFKA_VERSION}.tar.gz" | \
        tar xzf -
    [[ $? -eq 0 ]] || exit 1
fi

echo "Building librdkafka-$LIBRDKAFKA_VERSION"
cd librdkafka-$LIBRDKAFKA_VERSION || exit 1
./configure || exit 1
make || exit 1
make DESTDIR="${PWD}/../" install || exit 1

cd ../../
echo "Building kafkacat"
export CFLAGS="$CFLAGS -Itmp-bootstrap/usr/include"
export LDFLAGS="$LDFLAGS -Ltmp-bootstrap/usr/lib"
./configure --enable-static || exit 1
make || exit 1

echo ""
echo "Success! kafkacat is now built"
echo ""
