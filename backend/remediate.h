#ifndef REMEDIATE_H
#define REMEDIATE_H

#include <pthread.h>
#include <stdbool.h>
#include <microhttpd.h>

struct xccdf_policy; // forward declaration, comme dans scan.h
typedef struct cJSON cJSON;

enum remediate_state {
    REMEDIATE_RUNNING,       // valeur 0 : doit rester en tête pour calloc()
    REMEDIATE_SUCCEEDED,     // evaluate + remediate terminés, event "done" pas encore envoyé
    REMEDIATE_DONE_SENT,     // event "done" déjà envoyé, flux peut se clore
    REMEDIATE_FAILED
};

struct remediate_context {
    pthread_mutex_t mutex;

    char **items;
    int count;
    int capacity;
    int read_index;
    size_t item_offset;

    double final_score;

    // même rôle que dans scan_context : métadonnées de la règle en cours,
    // posées par le start_callback, consommées par l'output_callback
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

    // règles à remédier, dupliquées depuis la requête HTTP (le body JSON qui
    // les contenait est libéré dès le retour de remediate_context_new) —
    // ce contexte en est l'unique propriétaire
    char **rule_ids;
    int rule_count;

    int ref_count;
    bool cancelled;
    enum remediate_state state;
    bool suspended;
};

// copie benchmark_id/profile_id/rule_ids (chaque chaîne dupliquée) ;
// retourne NULL si rule_count <= 0 ou en cas d'échec d'allocation
struct remediate_context *remediate_context_new(const char *benchmark_id, const char *profile_id,
                                                   const char **rule_ids, int rule_count);
void remediate_context_set_connection(struct remediate_context *ctx, struct MHD_Connection *connection);
bool remediate_context_start(struct remediate_context *ctx);
void remediate_context_abort(struct remediate_context *ctx);

void remediate_consumer_finish(void *cls);
ssize_t remediate_reader_callback(void *cls, uint64_t pos, char *buf, size_t max);


// -1 = erreur générique ; -3 = profil introuvable (natif ou tailoring) ;
// -8 = un rule_ids[i] n'existe pas dans le benchmark (out_invalid_id rempli) ;
// -9 = un rule_ids[i] existe dans le benchmark mais n'est pas sélectionné par
// CE profil précis (out_invalid_id rempli) — distinct de -8, une règle peut
// être valide globalement sans appartenir au périmètre du profil demandé
int validate_remediation_request(const char *benchmark_id, const char *profile_id,
                                   const char **rule_ids, int rule_count,
                                   char *out_invalid_id, size_t out_invalid_id_size);

                                   
void remediate_cancel(struct remediate_context *ctx);

void remediate_set_dry_run(bool enabled);


#endif