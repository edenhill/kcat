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
    if [[ $(which wget 2>&1 > /dev/null) ]]; then
	DL='wget -O-'
    else
	DL='curl -L'
    fi
    $DL "https://github.com/edenhill/librdkafka/archive/${LIBRDKAFKA_VERSION}.tar.gz" | \
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
export CPPFLAGS="$CPPFLAGS -Itmp-bootstrap/usr/local/include"
export LDFLAGS="$LDFLAGS -Ltmp-bootstrap/usr/local/lib tmp-bootstrap/usr/local/lib/librdkafka.a"
./configure || exit 1
make || exit 1

echo ""
echo "Success! kafkacat is now built"
echo ""
