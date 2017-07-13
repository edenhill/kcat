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

function build {
    dir=$1
    cmds=$2


    echo "Building $dir"
    pushd $dir > /dev/null
    set +o errexit
    eval $cmds
    ret=$?
    set -o errexit
    popd > /dev/null

    if [[ $ret == 0 ]]; then
        echo "Build of $dir SUCCEEDED!"
    else
        echo "Build of $dir FAILED!"
    fi

    return $ret
}

function pkg_cfg_lib {
    pkg=$1

    local libs=$(PKG_CONFIG_PATH=tmp-bootstrap/usr/local/lib/pkgconfig pkg-config --libs --static $pkg)

    # If pkg-config isnt working try grabbing the library list manually.
    if [[ -z "$libs" ]]; then
        libs=$(grep ^Libs.private tmp-bootstrap/usr/local/lib/pkgconfig/${pkg}.pc | sed -e s'/^Libs.private: //g')
    fi

    # Since we specify the exact .a files to link further down below
    # we need to remove the -l<libname> here.
    libs=$(echo $libs | sed -e "s/-l${pkg}//g")
    echo " $libs"

    >&2 echo "Using $libs for $pkg"
}

mkdir -p tmp-bootstrap
pushd tmp-bootstrap > /dev/null

github_download "edenhill/librdkafka" "master" "librdkafka"
github_download "lloyd/yajl" "master" "libyajl"

build librdkafka "([ -f config.h ] || ./configure) && make && make DESTDIR=\"${PWD}/\" install" || (echo "Failed to build librdkafka: bootstrap failed" ; false)

build libyajl "([ -f config.h ] || ./configure) && make && make DESTDIR=\"${PWD}/\" install" || (echo "Failed to build libyajl: JSON support will probably be disabled" ; true)


popd > /dev/null

echo "Building kafkacat"
export CPPFLAGS="${CPPFLAGS:-} -Itmp-bootstrap/usr/local/include"
export LIBS="$(pkg_cfg_lib rdkafka) $(pkg_cfg_lib yajl)"
export STATIC_LIB_rdkafka="tmp-bootstrap/usr/local/lib/librdkafka.a"
export STATIC_LIB_yajl="tmp-bootstrap/usr/local/lib/libyajl_s.a"
./configure --enable-static --enable-json
make

echo ""
echo "Success! kafkacat is now built"
echo ""

./kafkacat -h
