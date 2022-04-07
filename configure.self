#!/bin/bash
#

mkl_require good_cflags
mkl_require gitversion as KCAT_VERSION default 1.8.0


function checks {

    # Check that librdkafka is available, and allow to link it statically.
    mkl_meta_set "rdkafka" "desc" "librdkafka is available at http://github.com/edenhill/librdkafka. To quickly download all dependencies and build kcat try ./bootstrap.sh"
    mkl_meta_set "rdkafka" "deb" "librdkafka-dev"
    # Try static librdkafka first
    mkl_lib_check --libname=rdkafka-static "rdkafkastatic" "" disable CC "-lrdkafka" \
                  "#include <librdkafka/rdkafka.h>" ||
        mkl_lib_check "rdkafka" "" fail CC "-lrdkafka" \
                  "#include <librdkafka/rdkafka.h>"

    # Make sure rdkafka is new enough.
    mkl_meta_set "librdkafkaver" "name" "librdkafka metadata API"
    mkl_meta_set "librdkafkaver" "desc" "librdkafka 0.8.4 or later is required for the Metadata API"
    mkl_compile_check "librdkafkaver" "" fail CC "" \
"#include <librdkafka/rdkafka.h>
struct rd_kafka_metadata foo;"

    # Enable KafkaConsumer support if librdkafka is new enough
    mkl_meta_set "librdkafka_ge_090" "name" "librdkafka KafkaConsumer support"
    mkl_compile_check "librdkafka_ge_090" ENABLE_KAFKACONSUMER disable CC "" "
    #include <librdkafka/rdkafka.h>
    #if RD_KAFKA_VERSION >= 0x00090000
    #else
    #error \"rdkafka version < 0.9.0\"
    #endif"


    mkl_meta_set "yajl" "deb" "libyajl-dev"
    # Check for JSON library (yajl)
    if [[ $WITH_JSON == y ]] && \
        mkl_lib_check "yajl" HAVE_YAJL disable CC "-lyajl" \
        "#include  <yajl/yajl_version.h>
#if YAJL_MAJOR >= 2
#else
#error \"Requires libyajl2\"
#endif
"
    then
        mkl_allvar_set "json" ENABLE_JSON y
    fi


    mkl_meta_set "avroc" "static" "libavro.a"
    mkl_meta_set "libserdes" "deb" "libserdes-dev"
    mkl_meta_set "libserdes" "static" "libserdes.a"

    # Check for Avro and Schema-Registry client libs
    if [[ $WITH_AVRO == y ]] &&
           mkl_lib_check --libname=avro-c "avroc" "" disable CC "-lavro" "#include <avro.h>" &&
           mkl_lib_check "serdes" HAVE_SERDES disable CC "-lserdes" \
        "#include <sys/types.h>
        #include  <stdlib.h>
        #include  <libserdes/serdes-avro.h>"; then
        mkl_allvar_set "avro" ENABLE_AVRO y
    fi
}


mkl_toggle_option "kcat" WITH_JSON --enable-json "JSON support (requires libyajl2)" y
mkl_toggle_option "kcat" WITH_AVRO --enable-avro "Avro/Schema-Registry support (requires libserdes)" y
