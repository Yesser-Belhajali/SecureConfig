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
};

struct scan_context *scan_context_new(const char *benchmark_id, const char *profile_id);
void scan_context_set_connection(struct scan_context *ctx, struct MHD_Connection *connection);
void scan_context_start(struct scan_context *ctx);

// signature imposée par MHD_ContentReaderFreeCallback : void (*)(void *cls)
void scan_context_free(void *cls);

ssize_t scan_reader_callback(void *cls, uint64_t pos, char *buf, size_t max);

#endif