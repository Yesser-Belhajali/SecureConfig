#ifndef SCAN_H
#define SCAN_H

#include <pthread.h>
#include <stdbool.h>
#include <microhttpd.h>


struct xccdf_policy; // forward declaration, évite d'inclure xccdf_policy.h ici
typedef struct cJSON cJSON; // forward declaration, évite d'inclure cJSON.h ici

enum scan_state {
    SCAN_RUNNING,      // valeur 0 : doit rester en tête pour que calloc() donne cet état par défaut
    SCAN_SUCCEEDED,     // évaluation terminée avec succès, event "done" pas encore envoyé
    SCAN_DONE_SENT,      // event "done" déjà envoyé, le flux peut se clore au prochain appel
    SCAN_FAILED         // évaluation échouée à une étape quelconque
};

struct scan_context {
    pthread_mutex_t mutex;

    char **items;
    int count;
    int capacity;
    int read_index;

    size_t item_offset;   // octets déjà envoyés de ctx->items[read_index], pour les items plus grands que le buffer MHD

    double final_score;

    // métadonnées de la règle en cours d'évaluation : posées par
    // scan_start_callback, consommées par scan_output_callback juste après.
    // Pas de mutex nécessaire : seul le thread producteur touche ces champs.
    char *pending_title;
    char *pending_description;
    char *pending_rationale;
    char *pending_question;
    cJSON *pending_fixes;
    cJSON *pending_warnings;
    cJSON *pending_platforms;
    cJSON *pending_checks;
    cJSON *pending_references;

    struct MHD_Connection *connection;
    pthread_t producer_thread;

    struct xccdf_policy *policy;

    char benchmark_id[64];
    char profile_id[128];

    int ref_count;
    bool cancelled;
    enum scan_state state;
    bool suspended;   // vrai si la connexion est actuellement suspendue (protégé par mutex)
};

struct scan_context *scan_context_new(const char *benchmark_id, const char *profile_id);
void scan_context_set_connection(struct scan_context *ctx, struct MHD_Connection *connection);
bool scan_context_start(struct scan_context *ctx);   // false si pthread_create a échoué
void scan_context_abort(struct scan_context *ctx);    // nettoyage si scan_context_start() a échoué

void consumer_finish(void *cls);

ssize_t scan_reader_callback(void *cls, uint64_t pos, char *buf, size_t max);

void scan_cancel(struct scan_context *ctx);


#endif