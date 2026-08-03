// http_server.c
#include <stdio.h>
#include <string.h>
#include <microhttpd.h>

#define PORT 8000

static enum MHD_Result handle_request(void *cls,
                                        struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *version,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls) {

    // MHD appelle ce callback plusieurs fois par requête (une fois pour les headers,
    // une fois par chunk de body). Pour du GET simple sans body, on veut agir
    // une seule fois : on utilise con_cls comme marqueur de "première fois".
    static int dummy;
    if (*con_cls == NULL) {
        *con_cls = &dummy;
        return MHD_YES;  // attendre le vrai traitement au prochain appel
    }

    const char *response_text;
    int status_code;

    if (strcmp(method, "GET") == 0 && strcmp(url, "/hello") == 0) {
        response_text = "{\"message\":\"Salut depuis le backend C!\"}";
        status_code = MHD_HTTP_OK;
    } else {
        response_text = "{\"error\":\"not found\"}";
        status_code = MHD_HTTP_NOT_FOUND;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_text), (void *)response_text, MHD_RESPMEM_MUST_COPY);

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