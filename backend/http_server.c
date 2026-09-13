// http_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>
#include <oscap.h>
#include "scap_service.h"
#include "json_utils.h"
#include "scan.h"
#include "remediate.h"
#include "active_operation.h"



#define PORT 8000


struct connection_info {
    char *body;
    size_t body_size;
};

static int resolve_ds_path(const char *benchmark_id, char *out_path, size_t out_size) {
    // convention : data/<id>/ssg-<id>-ds.xml

    if (!is_valid_id_component(benchmark_id)) {
        return 0;
    }

    int n = snprintf(out_path, out_size, "../data/%s/ssg-%s-ds.xml", benchmark_id, benchmark_id);
    if (n < 0 || (size_t)n >= out_size) {
        return 0; // troncature, id trop long
    }
    return 1;
}



static void request_completed(void *cls, struct MHD_Connection *connection,
                               void **con_cls, enum MHD_RequestTerminationCode toe) {
    struct connection_info *con_info = *con_cls;
    if (con_info != NULL) {
        free(con_info->body);
        free(con_info);
        *con_cls = NULL;
    }
}

// construit le corps JSON de réponse pour une création réussie: {"id": "..."}
static char *build_created_response(const char *new_id) {
    cJSON *resp = cJSON_CreateObject();
    if (resp == NULL) return NULL;
    cJSON_AddStringToObject(resp, "id", new_id);
    char *json_str = cJSON_Print(resp);
    cJSON_Delete(resp);
    return json_str;
}


// construit le corps JSON pour un ID de règle invalide (-8) : le message
// inclut l'ID fautif, indispensable pour que le frontend sache lequel des
// checkboxes cochées ne correspond à aucune règle du benchmark
static char *build_invalid_id_response(const char *invalid_id) {
    cJSON *resp = cJSON_CreateObject();
    if (resp == NULL) return NULL;
    cJSON_AddStringToObject(resp, "error", "invalid rule id");
    cJSON_AddStringToObject(resp, "invalid_id", invalid_id != NULL ? invalid_id : "");
    char *json_str = cJSON_Print(resp);
    cJSON_Delete(resp);
    return json_str;
}


static int extract_benchmark_profiles(const char *url, char *out_id, size_t out_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles%n", out_id, &pos) != 1) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0; // ex: laisse passer /profiles/{id}/... vers leurs propres handlers
    }
    return 1;
}


static int extract_profile_selected_rules_request(const char *url, char *out_benchmark_id, size_t bid_size,
                                            char *out_profile_id, size_t pid_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles/%127[^/]/rules%n",
               out_benchmark_id, out_profile_id, &pos) != 2) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0; // ex: laisse passer /rules/all vers son propre handler
    }
    return 1;
}

// GET /benchmarks/{id}/profiles/{id}/rules/all
// doit être vérifiée AVANT extract_profile_rules_request : sans le contrôle
// de fin de chaîne (%n + url[pos]=='\0'), cette dernière matcherait aussi
// une URL se terminant par /rules/all (sscanf ignore silencieusement le
// suffixe non consommé par le format)
static int extract_profile_all_rules_request(const char *url, char *out_benchmark_id, size_t bid_size,
                                       char *out_profile_id, size_t pid_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles/%127[^/]/rules/all%n",
               out_benchmark_id, out_profile_id, &pos) != 2) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0; // caractères en trop après /all -> pas une correspondance
    }
    return 1;
}


static int extract_benchmark_rules_request(const char *url, char *out_id, size_t out_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/rules%n", out_id, &pos) != 1) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0;
    }
    return 1;
}

static int extract_scan_request(const char *url, char *out_benchmark_id, size_t bid_size,
                                  char *out_profile_id, size_t pid_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles/%127[^/]/scan%n",
               out_benchmark_id, out_profile_id, &pos) != 2) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0;
    }
    return 1;
}

// DELETE /benchmarks/{id}/profiles/{id} — exactement 2 segments, sans suffixe
// (/rules, /rules/all, /scan filent vers leurs propres handlers grâce au
// contrôle de fin de chaîne)
static int extract_single_profile_request(const char *url, char *out_benchmark_id, size_t bid_size,
                                             char *out_profile_id, size_t pid_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles/%127[^/]%n",
               out_benchmark_id, out_profile_id, &pos) != 2) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0;
    }
    return 1;
}

static int extract_remediate_request(const char *url, char *out_benchmark_id, size_t bid_size,
                                       char *out_profile_id, size_t pid_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles/%127[^/]/remediate%n",
               out_benchmark_id, out_profile_id, &pos) != 2) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0;
    }
    return 1;
}


static void handle_create_profile(const char *benchmark_id, const char *body,
                                    const char **response_text, int *status_code,
                                    enum MHD_ResponseMemoryMode *mem_mode) {
    cJSON *json = cJSON_Parse(body != NULL ? body : "");
    if (json == NULL) {
        *response_text = "{\"error\":\"invalid json\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
        return;
    }

    cJSON *name_item = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *description_item = cJSON_GetObjectItemCaseSensitive(json, "description");
    cJSON *base_profile_item = cJSON_GetObjectItemCaseSensitive(json, "base_profile_id");
    cJSON *added_item = cJSON_GetObjectItemCaseSensitive(json, "added");
    cJSON *removed_item = cJSON_GetObjectItemCaseSensitive(json, "removed");

    const char *name = cJSON_IsString(name_item) ? name_item->valuestring : NULL;
    const char *description = cJSON_IsString(description_item) ? description_item->valuestring : NULL;
    const char *base_profile_id = cJSON_IsString(base_profile_item) ? base_profile_item->valuestring : NULL;

    if (name == NULL || name[0] == '\0' || !cJSON_IsArray(added_item) || !cJSON_IsArray(removed_item)) {
        *response_text = "{\"error\":\"missing or invalid fields (name, added, removed required)\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
        cJSON_Delete(json);
        return;
    }

    int added_count = cJSON_GetArraySize(added_item);
    int removed_count = cJSON_GetArraySize(removed_item);

    const char **added_ids = added_count > 0 ? malloc(added_count * sizeof(char *)) : NULL;
    const char **removed_ids = removed_count > 0 ? malloc(removed_count * sizeof(char *)) : NULL;

    bool arrays_ok = (added_count == 0 || added_ids != NULL) && (removed_count == 0 || removed_ids != NULL);

    for (int i = 0; arrays_ok && i < added_count; i++) {
        cJSON *item = cJSON_GetArrayItem(added_item, i);
        if (!cJSON_IsString(item)) { arrays_ok = false; break; }
        added_ids[i] = item->valuestring;
    }
    for (int i = 0; arrays_ok && i < removed_count; i++) {
        cJSON *item = cJSON_GetArrayItem(removed_item, i);
        if (!cJSON_IsString(item)) { arrays_ok = false; break; }
        removed_ids[i] = item->valuestring;
    }

    if (!arrays_ok) {
        *response_text = "{\"error\":\"added/removed must be arrays of strings\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
        free(added_ids);
        free(removed_ids);
        cJSON_Delete(json);
        return;
    }

    char new_id[256];
    char invalid_id[256];
    int ret = create_tailoring_profile(benchmark_id, name, description, base_profile_id,
                                        added_ids, added_count, removed_ids, removed_count,
                                        new_id, sizeof(new_id),
                                        invalid_id, sizeof(invalid_id));

    free(added_ids);
    free(removed_ids);

    if (ret == 0) {
        *response_text = build_created_response(new_id);
        if (*response_text == NULL) {
            *response_text = "{\"error\":\"json serialization failed\"}";
            *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        }
        else {
            *status_code = MHD_HTTP_CREATED;
            *mem_mode = MHD_RESPMEM_MUST_FREE;
        }
    }
    else if (ret == -2) {
        *response_text = "{\"error\":\"a profile with this name already exists\"}";
        *status_code = MHD_HTTP_CONFLICT;
    }
    else if (ret == -3) {
        *response_text = "{\"error\":\"base profile not found\"}";
        *status_code = MHD_HTTP_NOT_FOUND;
    }
    else if (ret == -4) {
        *response_text = "{\"error\":\"Un profil créé à partir de zéro doit contenir au moins une règle.\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
    }
    else if (ret == -5) {
        *response_text = "{\"error\":\"Ce profil ne contient aucune règle une fois résolu — vérifiez votre sélection.\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
    }
    else if (ret == -6) {
        *response_text = "{\"error\":\"Aucune règle ajoutée ou retirée par rapport au profil de base — dupliquer un profil à l'identique n'est pas encore pris en charge.\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
    }
    else if (ret == -8) {
        *response_text = build_invalid_id_response(invalid_id);
        if (*response_text == NULL) {
            *response_text = "{\"error\":\"invalid rule id\"}";
            *status_code = MHD_HTTP_BAD_REQUEST;
        }
        else {
            *status_code = MHD_HTTP_BAD_REQUEST;
            *mem_mode = MHD_RESPMEM_MUST_FREE;
        }
    }
    else {
        *response_text = "{\"error\":\"failed to create profile\"}";
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }

    cJSON_Delete(json);
}

static void handle_update_profile(const char *benchmark_id, const char *profile_id, const char *body,
                                    const char **response_text, int *status_code,
                                    enum MHD_ResponseMemoryMode *mem_mode) {

    cJSON *json = cJSON_Parse(body != NULL ? body : "");
    if (json == NULL) {
        *response_text = "{\"error\":\"invalid json\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
        return;
    }

    cJSON *added_item = cJSON_GetObjectItemCaseSensitive(json, "added");
    cJSON *removed_item = cJSON_GetObjectItemCaseSensitive(json, "removed");

    if (!cJSON_IsArray(added_item) || !cJSON_IsArray(removed_item)) {
        *response_text = "{\"error\":\"added/removed arrays required\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
        cJSON_Delete(json);
        return;
    }

    int added_count = cJSON_GetArraySize(added_item);
    int removed_count = cJSON_GetArraySize(removed_item);

    const char **added_ids = added_count > 0 ? malloc(added_count * sizeof(char *)) : NULL;
    const char **removed_ids = removed_count > 0 ? malloc(removed_count * sizeof(char *)) : NULL;

    bool arrays_ok = (added_count == 0 || added_ids != NULL) && (removed_count == 0 || removed_ids != NULL);

    for (int i = 0; arrays_ok && i < added_count; i++) {
        cJSON *item = cJSON_GetArrayItem(added_item, i);
        if (!cJSON_IsString(item)) { arrays_ok = false; break; }
        added_ids[i] = item->valuestring;
    }
    for (int i = 0; arrays_ok && i < removed_count; i++) {
        cJSON *item = cJSON_GetArrayItem(removed_item, i);
        if (!cJSON_IsString(item)) { arrays_ok = false; break; }
        removed_ids[i] = item->valuestring;
    }

    if (!arrays_ok) {
        *response_text = "{\"error\":\"added/removed must be arrays of strings\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
        free(added_ids);
        free(removed_ids);
        cJSON_Delete(json);
        return;
    }

    char invalid_id[256];
    int ret = update_tailoring_profile(benchmark_id, profile_id, added_ids, added_count,
                                        removed_ids, removed_count,
                                        invalid_id, sizeof(invalid_id));

    free(added_ids);
    free(removed_ids);
    cJSON_Delete(json);

    if (ret == 0) {
        *response_text = "{\"updated\":true}";
        *status_code = MHD_HTTP_OK;
    }
    else if (ret == -2) {
        *response_text = "{\"error\":\"no tailoring file for this benchmark\"}";
        *status_code = MHD_HTTP_NOT_FOUND;
    }
    else if (ret == -3) {
        *response_text = "{\"error\":\"profile not found\"}";
        *status_code = MHD_HTTP_NOT_FOUND;
    }
    else if (ret == -5) {
        *response_text = "{\"error\":\"Cette modification viderait entièrement le profil.\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
    }
    else if (ret == -6) {
        *response_text = "{\"error\":\"Cette modification viderait un autre profil qui en hérite.\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
    }
    else if (ret == -7) {
        *response_text = "{\"error\":\"Aucune modification à enregistrer.\"}";
        *status_code = MHD_HTTP_BAD_REQUEST;
    }
    else if (ret == -8) {
        *response_text = build_invalid_id_response(invalid_id);
        if (*response_text == NULL) {
            *response_text = "{\"error\":\"invalid rule id\"}";
            *status_code = MHD_HTTP_BAD_REQUEST;
        }
        else {
            *status_code = MHD_HTTP_BAD_REQUEST;
            *mem_mode = MHD_RESPMEM_MUST_FREE;
        }
    }
    else {
        *response_text = "{\"error\":\"failed to update profile\"}";
        *status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }
}


static enum MHD_Result handle_request(void *cls,
                                        struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *version,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls) {

    if (*con_cls == NULL) {
        struct connection_info *con_info = malloc(sizeof(struct connection_info));
        if (con_info == NULL) {
            return MHD_NO;
        }
        con_info->body = NULL;
        con_info->body_size = 0;
        *con_cls = con_info;
        return MHD_YES;
    }

    struct connection_info *con_info = *con_cls;

    // accumule le corps de la requête tant qu'il en reste (POST notamment)
    if (*upload_data_size != 0) {
        char *tmp = realloc(con_info->body, con_info->body_size + *upload_data_size + 1);
        if (tmp == NULL) {
            return MHD_NO;
        }
        con_info->body = tmp;
        memcpy(con_info->body + con_info->body_size, upload_data, *upload_data_size);
        con_info->body_size += *upload_data_size;
        con_info->body[con_info->body_size] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }

    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    const char *response_text;
    int status_code;
    enum MHD_ResponseMemoryMode mem_mode = MHD_RESPMEM_MUST_COPY;

    char benchmark_id[64];
    char profile_id[128];

    if (strcmp(method, "GET") == 0 && extract_scan_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        struct scan_context *ctx = scan_context_new(benchmark_id, profile_id);
        if (ctx == NULL) {
            struct MHD_Response *err = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, err);
            MHD_destroy_response(err);
            return ret;
        }

        // EventSource ne peut pas lire un code de statut HTTP -> on répond
        // en SSE valide (200 + un seul event "error") plutôt qu'un vrai 409,
        // pour que le frontend distingue "occupé" d'une vraie erreur réseau
        if (!active_operation_try_claim(ACTIVE_OP_SCAN, ctx)) {
            scan_context_abort(ctx);
            const char *busy_sse =
                "data: {\"type\":\"error\",\"message\":\"Un scan ou une remédiation est déjà en cours.\"}\n\n";
            struct MHD_Response *busy = MHD_create_response_from_buffer(strlen(busy_sse), (void *)busy_sse, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(busy, "Content-Type", "text/event-stream");
            MHD_add_response_header(busy, "Access-Control-Allow-Origin", "*");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, busy);
            MHD_destroy_response(busy);
            return ret;
        }

        scan_context_set_connection(ctx, connection);

        if (!scan_context_start(ctx)) {
            scan_context_abort(ctx);
            struct MHD_Response *err = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, err);
            MHD_destroy_response(err);
            return ret;
        }

        struct MHD_Response *sse_response = MHD_create_response_from_callback(
            MHD_SIZE_UNKNOWN, 1024, &scan_reader_callback, ctx, &consumer_finish);
        MHD_add_response_header(sse_response, "Content-Type", "text/event-stream");
        MHD_add_response_header(sse_response, "Cache-Control", "no-cache");
        MHD_add_response_header(sse_response, "Access-Control-Allow-Origin", "*");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, sse_response);
        MHD_destroy_response(sse_response);
        return ret;
    }

    if (strcmp(method, "POST") == 0 && extract_remediate_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        cJSON *json = cJSON_Parse(con_info->body != NULL ? con_info->body : "");
        if (json == NULL) {
            const char *msg = "{\"error\":\"invalid json\"}";
            struct MHD_Response *err = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(err, "Content-Type", "application/json");
            MHD_add_response_header(err, "Access-Control-Allow-Origin", "*");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, err);
            MHD_destroy_response(err);
            return ret;
        }

        cJSON *rule_ids_item = cJSON_GetObjectItemCaseSensitive(json, "rule_ids");
        if (!cJSON_IsArray(rule_ids_item) || cJSON_GetArraySize(rule_ids_item) == 0) {
            cJSON_Delete(json);
            const char *msg = "{\"error\":\"rule_ids must be a non-empty array of strings\"}";
            struct MHD_Response *err = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(err, "Content-Type", "application/json");
            MHD_add_response_header(err, "Access-Control-Allow-Origin", "*");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, err);
            MHD_destroy_response(err);
            return ret;
        }

        int rule_count = cJSON_GetArraySize(rule_ids_item);
        const char **rule_ids = malloc(rule_count * sizeof(char *));
        bool arrays_ok = (rule_ids != NULL);

        for (int i = 0; arrays_ok && i < rule_count; i++) {
            cJSON *item = cJSON_GetArrayItem(rule_ids_item, i);
            if (!cJSON_IsString(item)) { arrays_ok = false; break; }
            rule_ids[i] = item->valuestring;
        }

        if (!arrays_ok) {
            free(rule_ids);
            cJSON_Delete(json);
            const char *msg = "{\"error\":\"rule_ids must be an array of strings\"}";
            struct MHD_Response *err = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(err, "Content-Type", "application/json");
            MHD_add_response_header(err, "Access-Control-Allow-Origin", "*");
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, err);
            MHD_destroy_response(err);
            return ret;
        }

        char invalid_id[256];
        int vret = validate_remediation_request(benchmark_id, profile_id, rule_ids, rule_count, invalid_id, sizeof(invalid_id));

        if (vret != 0) {
            const char *err_text;
            int err_status;
            char *heap_text = NULL;

            if (vret == -3) {
                err_text = "{\"error\":\"profile not found\"}";
                err_status = MHD_HTTP_NOT_FOUND;
            }
            else if (vret == -8) {
                heap_text = build_invalid_id_response(invalid_id);
                err_text = heap_text != NULL ? heap_text : "{\"error\":\"invalid rule id\"}";
                err_status = MHD_HTTP_BAD_REQUEST;
            }
            else if (vret == -9) {
                // réutilise build_invalid_id_response : même forme de payload
                // (error + invalid_id), le frontend distingue déjà par ce champ
                heap_text = build_invalid_id_response(invalid_id);
                err_text = heap_text != NULL ? heap_text : "{\"error\":\"rule not in profile\"}";
                err_status = MHD_HTTP_BAD_REQUEST;
            }
            else {
                err_text = "{\"error\":\"failed to validate remediation request\"}";
                err_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
            }

            struct MHD_Response *err = MHD_create_response_from_buffer(strlen(err_text), (void *)err_text,
                heap_text != NULL ? MHD_RESPMEM_MUST_FREE : MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(err, "Content-Type", "application/json");
            MHD_add_response_header(err, "Access-Control-Allow-Origin", "*");
            enum MHD_Result ret = MHD_queue_response(connection, err_status, err);
            MHD_destroy_response(err);

            free(rule_ids);
            cJSON_Delete(json);
            return ret;
        }

        struct remediate_context *rctx = remediate_context_new(benchmark_id, profile_id, rule_ids, rule_count);
        free(rule_ids);
        cJSON_Delete(json);

        if (rctx == NULL) {
            struct MHD_Response *err = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, err);
            MHD_destroy_response(err);
            return ret;
        }

        // remédiation = fetch() côté frontend, pas EventSource -> un vrai
        // 409 est lisible directement via response.status/response.ok
        if (!active_operation_try_claim(ACTIVE_OP_REMEDIATE, rctx)) {
            remediate_context_abort(rctx);
            const char *msg = "{\"error\":\"Un scan ou une remédiation est déjà en cours.\"}";
            struct MHD_Response *err = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(err, "Content-Type", "application/json");
            MHD_add_response_header(err, "Access-Control-Allow-Origin", "*");
            enum MHD_Result ret = MHD_queue_response(connection, 409, err);
            MHD_destroy_response(err);
            return ret;
        }

        remediate_context_set_connection(rctx, connection);

        if (!remediate_context_start(rctx)) {
            remediate_context_abort(rctx);
            struct MHD_Response *err = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, err);
            MHD_destroy_response(err);
            return ret;
        }

        struct MHD_Response *sse_response = MHD_create_response_from_callback(
            MHD_SIZE_UNKNOWN, 1024, &remediate_reader_callback, rctx, &remediate_consumer_finish);
        MHD_add_response_header(sse_response, "Content-Type", "text/event-stream");
        MHD_add_response_header(sse_response, "Cache-Control", "no-cache");
        MHD_add_response_header(sse_response, "Access-Control-Allow-Origin", "*");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, sse_response);
        MHD_destroy_response(sse_response);
        return ret;
    }

    // POST ou DELETE indifféremment : POST pour navigator.sendBeacon (qui ne
    // supporte que POST, utilisé sur pagehide/fermeture d'onglet), DELETE
    // pour l'appel explicite normal depuis le bouton "Stopper"
    if ((strcmp(method, "DELETE") == 0 || strcmp(method, "POST") == 0) && strcmp(url, "/operations/current") == 0) {
        active_operation_cancel();
        const char *msg = "{\"cancelled\":true}";
        struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(resp, "Content-Type", "application/json");
        MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);
        MHD_destroy_response(resp);
        return ret;
    }



    if (strcmp(method, "GET") == 0 && strcmp(url, "/hello") == 0) {
        response_text = "{\"message\":\"Salut depuis le backend C!\"}";
        status_code = MHD_HTTP_OK;
    }

    // GET /benchmarks/{id}/profiles -> profils DS + profils tailoring (si présents)
    else if (strcmp(method, "GET") == 0 && extract_benchmark_profiles(url, benchmark_id, sizeof(benchmark_id))) {
        struct profile_list *profiles = NULL;
        int profiles_count = 0;
        struct profile_list *tailoring_profiles = NULL;
        int tailoring_count = 0;

        if (list_profiles_for_distro(benchmark_id, &profiles, &profiles_count, &tailoring_profiles, &tailoring_count) != 0) {
            response_text = "{\"error\":\"failed to load benchmark\"}";
            status_code = MHD_HTTP_NOT_FOUND;
        } else {
            response_text = profiles_and_tailoring_to_json(profiles, profiles_count, tailoring_profiles, tailoring_count);
            free_profiles_for_distro(profiles, profiles_count, tailoring_profiles, tailoring_count);

            if (response_text == NULL) {
                response_text = "{\"error\":\"json serialization failed\"}";
                status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            } else {
                status_code = MHD_HTTP_OK;
                mem_mode = MHD_RESPMEM_MUST_FREE;
            }
        }
    }

    // IMPORTANT : cette route doit être testée avant extract_profile_rules_request
    else if (strcmp(method, "GET") == 0 && extract_profile_all_rules_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        struct rule_list *rules = NULL;
        int count = profile_all_rules(benchmark_id, profile_id, &rules);

        if (count < 0) {
            response_text = "{\"error\":\"profile not found or failed to load\"}";
            status_code = MHD_HTTP_NOT_FOUND;
        } else {
            response_text = rules_to_json(rules, count);
            free_rule_info_list(rules, count);

            if (response_text == NULL) {
                response_text = "{\"error\":\"json serialization failed\"}";
                status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            } else {
                status_code = MHD_HTTP_OK;
                mem_mode = MHD_RESPMEM_MUST_FREE;
            }
        }
    }
    else if (strcmp(method, "GET") == 0 && extract_profile_selected_rules_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        struct rule_list *rules = NULL;
        int count = profile_selected_rules(benchmark_id, profile_id, &rules);

        if (count < 0) {
            response_text = "{\"error\":\"profile not found or failed to load\"}";
            status_code = MHD_HTTP_NOT_FOUND;
        } else {
            response_text = rules_to_json(rules, count);
            free_rule_info_list(rules, count);

            if (response_text == NULL) {
                response_text = "{\"error\":\"json serialization failed\"}";
                status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
            } else {
                status_code = MHD_HTTP_OK;
                mem_mode = MHD_RESPMEM_MUST_FREE;
            }
        }
    }
    else if (strcmp(method, "GET") == 0 && extract_benchmark_rules_request(url, benchmark_id, sizeof(benchmark_id))) {
        char ds_path[256];
        if (!resolve_ds_path(benchmark_id, ds_path, sizeof(ds_path))) {
            response_text = "{\"error\":\"invalid benchmark id\"}";
            status_code = MHD_HTTP_BAD_REQUEST;
        }
        else {
            struct rule_list *rules = NULL;
            int count = list_rules_for_ds(ds_path, &rules);

            if (count < 0) {
                response_text = "{\"error\":\"failed to load rules\"}";
                status_code = MHD_HTTP_NOT_FOUND;
            }
            else {
                response_text = rules_to_json(rules, count);
                free_rule_info_list(rules, count);

                if (response_text == NULL) {
                    response_text = "{\"error\":\"json serialization failed\"}";
                    status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
                }
                else {
                    status_code = MHD_HTTP_OK;
                    mem_mode = MHD_RESPMEM_MUST_FREE;
                }
            }
        }
    }
    // POST /benchmarks/{id}/profiles -> création d'un profil de tailoring
    else if (strcmp(method, "POST") == 0 && extract_benchmark_profiles(url, benchmark_id, sizeof(benchmark_id))) {
        handle_create_profile(benchmark_id, con_info->body, &response_text, &status_code, &mem_mode);
    }
    // DELETE /benchmarks/{id}/profiles/{id} -> suppression d'un profil de tailoring
    else if (strcmp(method, "DELETE") == 0 && extract_single_profile_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        int ret = delete_tailoring_profile(benchmark_id, profile_id);
        if (ret == 0) {
            response_text = "{\"deleted\":true}";
            status_code = MHD_HTTP_OK;
        }
        else if (ret == -2) {
            response_text = "{\"error\":\"no tailoring file for this benchmark\"}";
            status_code = MHD_HTTP_NOT_FOUND;
        }
        else if (ret == -3) {
            response_text = "{\"error\":\"profile not found\"}";
            status_code = MHD_HTTP_NOT_FOUND;
        }
        else if (ret == -4) {
            response_text = "{\"error\":\"D'autres profils personnalisés héritent de celui-ci — supprimez-les d'abord\"}";
            status_code = MHD_HTTP_CONFLICT;
        }
        else {
            response_text = "{\"error\":\"failed to delete profile\"}";
            status_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
        }
    }
    // PATCH /benchmarks/{id}/profiles/{id} -> modification d'un profil de tailoring existant
    else if (strcmp(method, "PATCH") == 0 && extract_single_profile_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        handle_update_profile(benchmark_id, profile_id, con_info->body, &response_text, &status_code, &mem_mode);
    }
    else {
        response_text = "{\"error\":\"not found\"}";
        status_code = MHD_HTTP_NOT_FOUND;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_text), (void *)response_text, mem_mode);

    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");

    enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

#include <unistd.h>   // ajouter si pas déjà présent, pour usleep

int main(void) {
    oscap_init();

    bool dry_run = getenv("DRY_RUN_REMEDIATION") != NULL;
    remediate_set_dry_run(dry_run);
    printf(dry_run
        ? "!! DRY-RUN ACTIVÉ : la remédiation ne modifiera PAS la machine !!\n"
        : "Remédiation en mode normal (modifie réellement la machine).\n");

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD | MHD_ALLOW_SUSPEND_RESUME,
        PORT,
        NULL, NULL,
        &handle_request, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
        MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "Echec du démarrage du serveur\n");
        return 1;
    }

    printf("Serveur démarré sur http://localhost:%d\n", PORT);
    printf("Appuie sur Entrée pour arrêter...\n");
    getchar();

    printf("Arrêt en cours — annulation de l'opération active...\n");
    active_operation_cancel();

    int waited_ms = 0;
    const int max_wait_ms = 10000;
    while (active_operation_is_busy() && waited_ms < max_wait_ms) {
        usleep(50 * 1000);
        waited_ms += 50;
    }
    if (waited_ms >= max_wait_ms) {
        fprintf(stderr, "Attention : arrêt forcé malgré des opérations encore actives\n");
    }

    MHD_stop_daemon(daemon);

    oscap_cleanup();
    return 0;
}