
#include <sys/types.h>
#include <stddef.h>
#include <libserdes/serdes-avro.h>

#include "kafkacat.h"

char *cnv_msg_output_avro(const void *data, int data_len) {
    serdes_t *serdes;
    serdes_err_t err;

    serdes_conf_t *sconf = serdes_conf_new(NULL, 0, "schema.registry.url", conf.schema_registry_url, NULL);

    char errstr[512];
    serdes = serdes_new(sconf, errstr, sizeof(errstr));

    avro_value_t avro;
    serdes_schema_t *schema;
    char *as_json = NULL;

    if (data) {
        err = serdes_deserialize_avro(serdes, &avro, &schema,
                                      data, data_len,
                                      errstr, sizeof(errstr));

        if (err) {
            fprintf(stderr, "%% serdes_deserialize_avro failed: %s\n", errstr);
            return NULL;
        }

        if (avro_value_to_json(&avro, 1, &as_json)) {
            fprintf(stderr, "%% avro_to_json failed: %s\n", avro_strerror());
            return NULL;
        }
    }
    return as_json;
}
