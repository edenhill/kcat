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


function github_download {
    repo=$1
    version=$2
    dir=$3

    url=https://github.com/${repo}/archive/${version}.tar.gz

    if [[ -d $dir ]]; then
        echo "Directory $dir already exists, not downloading $url"
        return 0
    fi

    echo "Downloading $url to $dir"
    if which wget 2>&1 > /dev/null; then
        DL='wget -q -O-'
    else
        DL='curl -s -L'
    fi

    mkdir -p "$dir"
    pushd "$dir" > /dev/null
    ($DL "$url" | tar -xzf - --strip-components 1) || exit 1
    popd > /dev/null
}


mkdir -p tmp-bootstrap
pushd tmp-bootstrap > /dev/null

github_download "edenhill/librdkafka" "master" "librdkafka"
github_download "lloyd/yajl" "master" "libyajl"


pushd librdkafka > /dev/null
echo "Building librdkafka"
./configure
make
make DESTDIR="${PWD}/../" install
popd > /dev/null


pushd libyajl > /dev/null
echo "Building libyajl"
./configure
make
make DESTDIR="${PWD}/../" install
popd > /dev/null


popd > /dev/null

echo "Building kafkacat"
export CPPFLAGS="${CPPFLAGS:-} -Itmp-bootstrap/usr/local/include"
export LDFLAGS="${LDFLAGS:-} -Ltmp-bootstrap/usr/local/lib"
export STATIC_LIB_rdkafka="tmp-bootstrap/usr/local/lib/librdkafka.a"
export STATIC_LIB_yajl="tmp-bootstrap/usr/local/lib/libyajl.a"
./configure --enable-static --enable-json
make

echo ""
echo "Success! kafkacat is now built"
echo ""
