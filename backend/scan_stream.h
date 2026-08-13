#ifndef SCAN_STREAM_H
#define SCAN_STREAM_H

#include <pthread.h>
#include <stdbool.h>
#include <microhttpd.h>

struct xccdf_policy; // forward declaration, évite d'inclure xccdf_policy.h ici

struct scan_context {
    pthread_mutex_t mutex;
    char **items;
    int count;
    int capacity;
    int read_index;

    bool finished;
    bool failed;
    bool done_sent;
    double final_score;

    struct MHD_Connection *connection;
    pthread_t producer_thread;

    struct xccdf_policy *policy; // renseigné après xccdf_session_set_profile_id, utilisé par scan_start_callback

    char benchmark_id[64];
    char profile_id[128];

    int ref_count;       // 2 : un pour le producer thread, un pour MHD
    bool cancelled;       // client parti, plus personne pour lire les events

    // chaînage intrusif dans le registre global des scans actifs
    struct scan_context *registry_next;
    struct scan_context *registry_prev;
};

struct scan_context *scan_context_new(const char *benchmark_id, const char *profile_id);
void scan_context_set_connection(struct scan_context *ctx, struct MHD_Connection *connection);
void scan_context_start(struct scan_context *ctx);

// signature imposée par MHD_ContentReaderFreeCallback : void (*)(void *cls)
void scan_context_free(void *cls);

ssize_t scan_reader_callback(void *cls, uint64_t pos, char *buf, size_t max);

// à appeler dans main(), juste avant MHD_stop_daemon() : réveille tous les
// scans en cours (suspendus ou non) et bloque jusqu'à ce qu'ils se soient
// tous terminés proprement, pour que MHD_stop_daemon ne rencontre plus
// aucune connexion suspendue.
void scan_stream_shutdown(void);

#endif