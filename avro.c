/*
 * kcat - Apache Kafka consumer and producer
 *
 * Copyright (c) 2019, Magnus Edenhill
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

#include "kcat.h"
#include <libserdes/serdes-avro.h>

static serdes_t *serdes;
static serdes_schema_t *key_schema, *value_schema;


/**
 * @brief Load schema from file.
 *
 * @param name_and_path is in the format: "schema_name:path/to/schema.avsc"
 *
 * @returns the schema object on success, will KC_FATAL on error.
 */
static serdes_schema_t *load_schema (const char *name, const char *path) {
        serdes_schema_t *schema;
        FILE *fp;
        char *buf;
        long size;
        char errstr[256];

        if (!(fp = fopen(path, "r")))
                KC_FATAL("Failed to open schema file %s: %s",
                         path, strerror(errno));

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size <= 0)
                KC_FATAL("Schema file %s is too small: %ld", path, size);

        buf = malloc(size+1);
        if (!buf)
                KC_FATAL("Failed to allocate buffer for %s of %ld bytes: %s",
                         path, size, strerror(errno));

        if (fread(buf, size, 1, fp) != 1)
                KC_FATAL("Failed to read schema file %s: %s",
                         path, strerror(errno));

        fclose(fp);

        buf[size] = '\0';

        schema = serdes_schema_add(serdes, name, -1, buf, (int)size,
                                   errstr, sizeof(errstr));
        if (!schema)
                KC_FATAL("Failed parse schema file %s: %s", path, errstr);

        return schema;
}


void kc_avro_init (const char *key_schema_name,
                   const char *key_schema_path,
                   const char *value_schema_name,
                   const char *value_schema_path) {
        char errstr[512];

        serdes = serdes_new(conf.srconf, errstr, sizeof(errstr));
        if (!serdes)
                KC_FATAL("Failed to create schema-registry client: %s", errstr);
        conf.srconf = NULL;

        if (key_schema_path)
                key_schema = load_schema(key_schema_name,
                                         key_schema_path);

        if (value_schema_path)
                value_schema = load_schema(value_schema_name,
                                           value_schema_path);

}

void kc_avro_term (void) {
        if (value_schema)
                serdes_schema_destroy(value_schema);
        if (key_schema)
                serdes_schema_destroy(key_schema);
        if (serdes)
                serdes_destroy(serdes);
}


/**
 * @brief Decodes the schema-id framed Avro blob in \p data
 *        and encodes it as JSON, which is returned as a newly allocated
 *        buffer which the caller must free.
 *
 * @returns newly allocated JSON string, or NULL on error.
 */
char *kc_avro_to_json (const void *data, size_t data_len, int *schema_idp,
                       char *errstr, size_t errstr_size) {
        avro_value_t avro;
        serdes_schema_t *schema;
        char *json;
        serdes_err_t err;

        err = serdes_deserialize_avro(serdes, &avro, &schema, data, data_len,
                                      errstr, errstr_size);
        if (err) {
                if (err == SERDES_ERR_FRAMING_INVALID ||
                    strstr(errstr, "Invalid CP1 magic byte")) {
                        static const char badframing[] =
                                ": message not produced with "
                                "Schema-Registry Avro framing";
                        int len = strlen(errstr);

                        if (len + sizeof(badframing) < errstr_size)
                                snprintf(errstr+len, errstr_size-len,
                                         "%s", badframing);
                }
                return NULL;
        }

        if (avro_value_to_json(&avro, 1/*one-line*/, &json)) {
                snprintf(errstr, errstr_size, "Failed to encode Avro as JSON");
                avro_value_decref(&avro);
                return NULL;
        }

        if (schema && schema_idp)
                *schema_idp = serdes_schema_id(schema);

        avro_value_decref(&avro);

        return json;
}
