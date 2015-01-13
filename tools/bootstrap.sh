#!/bin/bash
#
# This script provides a quick build alternative:
# * Dependencies are downloaded and built automatically
# * kafkacat is built automatically.
# * kafkacat is linked statically to avoid runtime dependencies.
#
# While this might not be the preferred method of building kafkacat, it
# is the easiest and quickest way.

set -o errexit -o nounset -o pipefail

: ${LIBRDKAFKA_VERSION:=0.8.5}
: ${LIBRDKAFKA_GIT_TAG:=$LIBRDKAFKA_VERSION}
: ${LIBRDKAFKA_GIT_REPO:=https://github.com/edenhill/librdkafka}

function bootstrap_librdkafka() {
  mkdir -p tmp
  if [[ ! -d tmp/librdkafka ]]; then
    git -C tmp clone "$LIBRDKAFKA_GIT_REPO"
    git -C tmp/librdkafka fetch --tags
    git -C tmp/librdkafka checkout -B "$LIBRDKAFKA_GIT_TAG"
  fi

  echo "Building librdkafka"
  pushd tmp/librdkafka > /dev/null
    ./configure
    make DESTDIR="${PWD}/../" all install
  popd > /dev/null
}

: ${SCALA_VERSION:=2.10}
: ${KAFKA_VERSION:=0.8.1 0.8.1.1 0.8.2-beta}
: ${KAFKA_ARCHIVE_URL:=https://archive.apache.org/dist/kafka}

function bootstrap_kafka() {
  mkdir -p tmp/kafka
  pushd tmp/kafka > /dev/null
    for version in ${KAFKA_VERSION}; do
      local kafka_dir="kafka-$version"
      if [ ! -d "$kafka_dir" ]; then
        echo "Downloading kafka-$version"
        local kafka_tmp_dir=kafka_${SCALA_VERSION}-${version}
        local tar_file=${kafka_tmp_dir}.tgz
        wget -q -N "${KAFKA_ARCHIVE_URL}/$version/$tar_file" || (rm -f $tar_file; exit 1)
        tar xzf $tar_file || (rm -f $tar_file; exit 1)
        rm $tar_file
        mv $kafka_tmp_dir  $kafka_dir
      fi
    done
  popd > /dev/null
}

function build() {
  echo "Building kafkacat"
  export STATIC_LIB_rdkafka="tmp/usr/local/lib/librdkafka.a"
  ./configure --enable-static \
      --CPPFLAGS=-Itmp/usr/local/include \
      --LDFLAGS=-Ltmp/usr/local/lib
  make clean all
}

bootstrap_librdkafka
# bootstrap_kafka
build
