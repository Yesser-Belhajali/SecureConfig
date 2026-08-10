// http_server.c
#include <stdio.h>
#include <string.h>
#include <microhttpd.h>
#include "scap_service.h"
#include "json_utils.h"

#define PORT 8000


static int extract_benchmark_id(const char *url, char *out_id, size_t out_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/profiles%n", out_id, &pos) != 1) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0; // ex: laisse passer /profiles/{id}/... vers leurs propres handlers
    }
    return 1;
}


static int extract_rules_request(const char *url, char *out_id, size_t out_size) {
    int pos = 0;
    if (sscanf(url, "/benchmarks/%63[^/]/rules%n", out_id, &pos) != 1) {
        return 0;
    }
    if (url[pos] != '\0') {
        return 0;
    }
    return 1;
}

// GET /benchmarks/{id}/profiles/{id}/rules/all
// doit être vérifiée AVANT extract_profile_rules_request : sans le contrôle
// de fin de chaîne (%n + url[pos]=='\0'), cette dernière matcherait aussi
// une URL se terminant par /rules/all (sscanf ignore silencieusement le
// suffixe non consommé par le format)
static int extract_all_rules_request(const char *url, char *out_benchmark_id, size_t bid_size,
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

static int extract_profile_rules_request(const char *url, char *out_benchmark_id, size_t bid_size,
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

static int resolve_ds_path(const char *benchmark_id, char *out_path, size_t out_size) {
    // convention : data/<id>/ssg-<id>-ds.xml
    int n = snprintf(out_path, out_size, "../data/%s/ssg-%s-ds.xml", benchmark_id, benchmark_id);
    if (n < 0 || (size_t)n >= out_size) {
        return 0; // troncature, id trop long
    }
    return 1;
}

static enum MHD_Result handle_request(void *cls,
                                        struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *version,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls) {

    static int dummy;
    if (*con_cls == NULL) {
        *con_cls = &dummy;
        return MHD_YES;
    }

    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
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

    if (strcmp(method, "GET") == 0 && strcmp(url, "/hello") == 0) {
        response_text = "{\"message\":\"Salut depuis le backend C!\"}";
        status_code = MHD_HTTP_OK;
    }
    // IMPORTANT : cette route doit être testée avant extract_profile_rules_request
    else if (strcmp(method, "GET") == 0 && extract_all_rules_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        struct rule_list *rules = NULL;
        int count = all_rules_with_selection_for_profile(benchmark_id, profile_id, &rules);

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
    else if (strcmp(method, "GET") == 0 && extract_profile_rules_request(url, benchmark_id, sizeof(benchmark_id), profile_id, sizeof(profile_id))) {
        struct rule_list *rules = NULL;
        int count = selected_rules_for_profile(benchmark_id, profile_id, &rules);

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
    // GET /benchmarks/{id}/profiles -> profils DS + profils tailoring (si présents)
    else if (strcmp(method, "GET") == 0 && extract_benchmark_id(url, benchmark_id, sizeof(benchmark_id))) {
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
    else if (strcmp(method, "GET") == 0 && extract_rules_request(url, benchmark_id, sizeof(benchmark_id))) {
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

int main(void) {
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        PORT,
        NULL, NULL,
        &handle_request, NULL,
        MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "Echec du démarrage du serveur\n");
        return 1;
    }

    printf("Serveur démarré sur http://localhost:%d\n", PORT);
    printf("Appuie sur Entrée pour arrêter...\n");
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}