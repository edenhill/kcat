/*
 * kafkacat - Apache Kafka consumer and producer
 *
 * Copyright (c) 2016, Magnus Edenhill
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "kafkacat.h"

void partition_list_print (rd_kafka_topic_partition_list_t *parts,
                           void *json_gen) {
        int i;

        /* Sort by topic+partition */
        rd_kafka_topic_partition_list_sort(parts, NULL, NULL);

#if ENABLE_JSON
        if (conf.flags & CONF_F_FMT_JSON) {
                partition_list_print_json(parts, json_gen);
                return;
        }
#endif

        for (i = 0 ; i < parts->cnt ; i++) {
                const rd_kafka_topic_partition_t *p = &parts->elems[i];
                printf("%s [%"PRId32"] offset %"PRId64"%s",
                       p->topic, p->partition, p->offset,
                       !p->err ? "\n": "");
                if (p->err)
                        printf(": %s\n", rd_kafka_err2str(p->err));
        }
}

int query_offsets_by_time (rd_kafka_topic_partition_list_t *offsets) {
        rd_kafka_resp_err_t err;
#if RD_KAFKA_VERSION >= 0x00090300
        char errstr[512];

        if (rd_kafka_conf_set(conf.rk_conf, "api.version.request", "true",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
                KC_FATAL("Failed to enable api.version.request: %s", errstr);

        if (!(conf.rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf.rk_conf,
                                     errstr, sizeof(errstr))))
                KC_FATAL("Failed to create producer: %s", errstr);

        err = rd_kafka_offsets_for_times(conf.rk, offsets, 10*1000);
#else
        err = RD_KAFKA_RESP_ERR__NOT_IMPLEMENTED;
#endif
        if (err)
                KC_FATAL("offsets_for_times failed: %s", rd_kafka_err2str(err));

        partition_list_print(offsets, NULL);

        rd_kafka_destroy(conf.rk);

        return 0;
}
