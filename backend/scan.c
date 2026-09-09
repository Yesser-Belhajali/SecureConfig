#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <oscap_source.h>
#include "scan.h"
#include "scap_service.h"



// ---------------------------------------------------------------------
// La file : ajout d'un event, sous mutex, avec croissance dynamique
// ---------------------------------------------------------------------

static void scan_context_push(struct scan_context *ctx, char *json) {
    // construit la trame SSE complète ("data: ...\n\n") dès maintenant : ainsi
    // scan_reader_callback n'a plus qu'à recopier des octets bruts avec memcpy,
    // sans jamais dépendre d'un snprintf dont le retour peut dépasser le buffer
    // fourni par MHD si le contenu (ex: un long script de fix) est volumineux
    int needed = snprintf(NULL, 0, "data: %s\n\n", json);
    if (needed < 0) {
        free(json);
        return;
    }

    char *framed = malloc((size_t)needed + 1);
    if (framed == NULL) {
        free(json);
        return;
    }
    snprintf(framed, (size_t)needed + 1, "data: %s\n\n", json);
    free(json);

    pthread_mutex_lock(&ctx->mutex);

    if (ctx->cancelled) {
        pthread_mutex_unlock(&ctx->mutex);
        free(framed);
        return;
    }

    if (ctx->count == ctx->capacity) {
        int new_cap = ctx->capacity == 0 ? 16 : ctx->capacity * 2;
        char **tmp = realloc(ctx->items, new_cap * sizeof(char *));
        if (tmp == NULL) {
            pthread_mutex_unlock(&ctx->mutex);
            free(framed);
            return;
        }
        ctx->items = tmp;
        ctx->capacity = new_cap;
    }

    ctx->items[ctx->count++] = framed;

    bool need_resume = ctx->suspended;

    if (need_resume) {
        ctx->suspended = false;
        MHD_resume_connection(ctx->connection);   // <-- appelé AVANT de déverrouiller
    }
    pthread_mutex_unlock(&ctx->mutex);
}

static void scan_context_reserve(struct scan_context *ctx, int capacity) {
    if (capacity <= 0) return;
    char **tmp = calloc((size_t)capacity, sizeof(char *));
    if (tmp == NULL) return;

    pthread_mutex_lock(&ctx->mutex);
    ctx->items = tmp;
    ctx->capacity = capacity;
    pthread_mutex_unlock(&ctx->mutex);
}

// ---------------------------------------------------------------------
// Sérialisation globale : un seul scan OpenSCAP à la fois dans tout le
// processus (l'évaluation XCCDF n'est pas garantie réentrante)
// ---------------------------------------------------------------------

static pthread_mutex_t g_scan_serialize_mutex = PTHREAD_MUTEX_INITIALIZER;

static void scan_context_free(struct scan_context *ctx) {
    for (int i = 0; i < ctx->count; i++) free(ctx->items[i]);
    free(ctx->items);
    free(ctx->pending_title);
    free(ctx->pending_description);
    free(ctx->pending_rationale);
    free(ctx->pending_question);
    cJSON_Delete(ctx->pending_fixes);
    cJSON_Delete(ctx->pending_warnings);
    cJSON_Delete(ctx->pending_platforms);
    cJSON_Delete(ctx->pending_checks);
    cJSON_Delete(ctx->pending_references);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
}

// appelé en fin de producer_main, quel que soit le chemin (succès ou échec)
static void producer_finish(struct scan_context *ctx) {
    pthread_mutex_lock(&ctx->mutex);
    bool client_gone = ctx->cancelled;
    int remaining = --ctx->ref_count;
    bool need_resume = !client_gone && ctx->suspended;
    if (need_resume) {
        ctx->suspended = false;
    }
    pthread_mutex_unlock(&ctx->mutex);

    if (need_resume) {
        MHD_resume_connection(ctx->connection);
    }

    if (remaining == 0) {
        pthread_detach(ctx->producer_thread);
        scan_context_free(ctx);
    }
}

// ---------------------------------------------------------------------
// Callbacks OpenSCAP
// ---------------------------------------------------------------------

static cJSON *build_fixes_array(struct xccdf_rule *rule) {
    cJSON *fixes_array = cJSON_CreateArray();
    struct xccdf_fix_iterator *fix_it = xccdf_rule_get_fixes(rule);
    if (fix_it == NULL) {
        return fixes_array;
    }
    while (xccdf_fix_iterator_has_more(fix_it)) {
        struct xccdf_fix *fix = xccdf_fix_iterator_next(fix_it);
        const char *system = xccdf_fix_get_system(fix);
        const char *content = xccdf_fix_get_content(fix);

        cJSON *fix_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(fix_obj, "system", system ? system : "");
        cJSON_AddStringToObject(fix_obj, "content", content ? content : "");
        cJSON_AddItemToArray(fixes_array, fix_obj);
    }
    xccdf_fix_iterator_free(fix_it);
    return fixes_array;
}

// même table de traduction que warning_category_to_str dans scap_service.c —
// dupliquée ici car statique dans les deux fichiers ; toute modification de
// l'une doit être répercutée sur l'autre
static const char *warning_category_to_str(xccdf_warning_category_t c) {
    switch (c) {
        case XCCDF_WARNING_GENERAL: return "general";
        case XCCDF_WARNING_FUNCTIONALITY: return "functionality";
        case XCCDF_WARNING_PERFORMANCE: return "performance";
        case XCCDF_WARNING_HARDWARE: return "hardware";
        case XCCDF_WARNING_LEGAL: return "legal";
        case XCCDF_WARNING_REGULATORY: return "regulatory";
        case XCCDF_WARNING_MANAGEMENT: return "management";
        case XCCDF_WARNING_AUDIT: return "audit";
        case XCCDF_WARNING_DEPENDENCY: return "dependency";
        case XCCDF_WARNING_NOT_SPECIFIED:
        default: return "not_specified";
    }
}

static cJSON *build_warnings_array(struct xccdf_rule *rule) {
    cJSON *arr = cJSON_CreateArray();
    struct xccdf_warning_iterator *it = xccdf_rule_get_warnings(rule);
    if (it == NULL) return arr;
    while (xccdf_warning_iterator_has_more(it)) {
        struct xccdf_warning *w = xccdf_warning_iterator_next(it);
        const char *category = warning_category_to_str(xccdf_warning_get_category(w));
        struct oscap_text *text_obj = xccdf_warning_get_text(w);
        const char *text = text_obj ? oscap_text_get_text(text_obj) : NULL;

        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "category", category ? category : "");
        cJSON_AddStringToObject(obj, "text", text ? text : "");
        cJSON_AddItemToArray(arr, obj);
    }
    xccdf_warning_iterator_free(it);
    return arr;
}

static cJSON *build_platforms_array(struct xccdf_rule *rule) {
    cJSON *arr = cJSON_CreateArray();
    struct oscap_string_iterator *it = xccdf_rule_get_platforms(rule);
    if (it == NULL) return arr;
    while (oscap_string_iterator_has_more(it)) {
        const char *platform = oscap_string_iterator_next(it);
        cJSON_AddItemToArray(arr, cJSON_CreateString(platform ? platform : ""));
    }
    oscap_string_iterator_free(it);
    return arr;
}

static cJSON *build_checks_array(struct xccdf_rule *rule) {
    cJSON *arr = cJSON_CreateArray();
    struct xccdf_check_iterator *it = xccdf_rule_get_checks(rule);
    if (it == NULL) return arr;
    while (xccdf_check_iterator_has_more(it)) {
        struct xccdf_check *check = xccdf_check_iterator_next(it);
        const char *system = xccdf_check_get_system(check);
        const char *selector = xccdf_check_get_selector(check);
        const char *content = xccdf_check_get_content(check);

        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "system", system ? system : "");
        cJSON_AddStringToObject(obj, "selector", selector ? selector : "");
        cJSON_AddStringToObject(obj, "content", content ? content : "");
        cJSON_AddItemToArray(arr, obj);
    }
    xccdf_check_iterator_free(it);
    return arr;
}

static cJSON *build_references_array(struct xccdf_rule *rule) {
    cJSON *arr = cJSON_CreateArray();
    struct oscap_reference_iterator *it = xccdf_rule_get_references(rule);
    if (it == NULL) return arr;

    while (oscap_reference_iterator_has_more(it)) {
        struct oscap_reference *ref = oscap_reference_iterator_next(it);
        const char *href = oscap_reference_get_href(ref);
        const char *text = oscap_reference_get_title(ref); // même convention que scap_service.c

        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "href", href ? href : "");
        cJSON_AddStringToObject(obj, "text", text ? text : "");
        cJSON_AddItemToArray(arr, obj);
    }
    oscap_reference_iterator_free(it);
    return arr;
}

static int scan_start_callback(struct xccdf_rule *rule, void *usr) {
    struct scan_context *ctx = usr;

    pthread_mutex_lock(&ctx->mutex);
    bool cancelled = ctx->cancelled;
    pthread_mutex_unlock(&ctx->mutex);

    if (cancelled) {
        return 1;
    }

    // sécurité : libère les métadonnées restées non consommées d'un tour
    // précédent (ex: règle jamais arrivée jusqu'à output_callback)
    free(ctx->pending_title);
    ctx->pending_title = NULL;
    free(ctx->pending_description);
    ctx->pending_description = NULL;
    free(ctx->pending_rationale);
    ctx->pending_rationale = NULL;
    free(ctx->pending_question);
    ctx->pending_question = NULL;
    cJSON_Delete(ctx->pending_fixes);
    ctx->pending_fixes = NULL;
    cJSON_Delete(ctx->pending_warnings);
    ctx->pending_warnings = NULL;
    cJSON_Delete(ctx->pending_platforms);
    ctx->pending_platforms = NULL;
    cJSON_Delete(ctx->pending_checks);
    ctx->pending_checks = NULL;
    cJSON_Delete(ctx->pending_references);
    ctx->pending_references = NULL;

    const char *rule_id = xccdf_rule_get_id(rule);
    bool selected = xccdf_policy_is_item_selected(ctx->policy, rule_id);


    if (!selected) {
        return 0;
    }

    ctx->pending_title = xccdf_policy_get_readable_item_title(ctx->policy, (struct xccdf_item *)rule, NULL);
    ctx->pending_description = xccdf_policy_get_readable_item_description(ctx->policy, (struct xccdf_item *)rule, NULL);
    ctx->pending_rationale = xccdf_policy_get_readable_item_rationale(ctx->policy, (struct xccdf_item *)rule, NULL);

    // pas d'équivalent xccdf_policy_get_readable_item_question confirmé :
    // fallback sur get_rule_question (non résolue, mais fonctionnelle,
    // déjà utilisée par /rules) — copie nécessaire car get_rule_question
    // retourne un pointeur interne à openscap, pas une chaîne allouée
    const char *question = get_rule_question(rule);
    ctx->pending_question = question ? strdup(question) : NULL;

    ctx->pending_fixes = build_fixes_array(rule);
    ctx->pending_warnings = build_warnings_array(rule);
    ctx->pending_platforms = build_platforms_array(rule);
    ctx->pending_checks = build_checks_array(rule);
    ctx->pending_references = build_references_array(rule);
    return 0;
}

static const char *severity_to_str(xccdf_level_t s) {
    switch (s) {
        case XCCDF_INFO: return "Info";
        case XCCDF_LOW: return "Low";
        case XCCDF_MEDIUM: return "Medium";
        case XCCDF_HIGH: return "High";
        case XCCDF_UNKNOWN: return "Unknown";
        default: return "Not Defined";
    }
}

static int scan_output_callback(struct xccdf_rule_result *rule_result, void *usr) {
    struct scan_context *ctx = usr;

    pthread_mutex_lock(&ctx->mutex);
    bool cancelled = ctx->cancelled;
    pthread_mutex_unlock(&ctx->mutex);

    if (cancelled) {
        return 1;
    }


    xccdf_test_result_type_t result_type = xccdf_rule_result_get_result(rule_result);

    if (result_type == XCCDF_RESULT_NOT_SELECTED) return 0;

    const char *status = "UNKNOWN";
    switch (result_type) {
        case XCCDF_RESULT_PASS: status = "PASS"; break;
        case XCCDF_RESULT_FAIL: status = "FAIL"; break;
        case XCCDF_RESULT_ERROR: status = "ERROR"; break;
        case XCCDF_RESULT_NOT_APPLICABLE: status = "NOT_APPLICABLE"; break;
        case XCCDF_RESULT_NOT_CHECKED: status = "NOT_CHECKED"; break;
        case XCCDF_RESULT_INFORMATIONAL: status = "INFORMATIONAL"; break;
        case XCCDF_RESULT_FIXED: status = "FIXED"; break;
        default: break;
    }

    cJSON *obj = cJSON_CreateObject();

    cJSON_AddStringToObject(obj, "type", "result");
    cJSON_AddStringToObject(obj, "id", xccdf_rule_result_get_idref(rule_result) ? xccdf_rule_result_get_idref(rule_result) : "");    cJSON_AddStringToObject(obj, "title", ctx->pending_title ? ctx->pending_title : "");
    cJSON_AddStringToObject(obj, "description", ctx->pending_description ? ctx->pending_description : "");
    cJSON_AddStringToObject(obj, "rationale", ctx->pending_rationale ? ctx->pending_rationale : "");
    cJSON_AddStringToObject(obj, "question", ctx->pending_question ? ctx->pending_question : "");
    cJSON_AddStringToObject(obj, "status", status);
    cJSON_AddStringToObject(obj, "severity", severity_to_str(xccdf_rule_result_get_severity(rule_result)));

    // transfert d'ownership : obj possède maintenant ces éléments, pas de
    // cJSON_Delete séparé à faire dessus
    cJSON_AddItemToObject(obj, "fixes", ctx->pending_fixes ? ctx->pending_fixes : cJSON_CreateArray());
    ctx->pending_fixes = NULL;
    cJSON_AddItemToObject(obj, "warnings", ctx->pending_warnings ? ctx->pending_warnings : cJSON_CreateArray());
    ctx->pending_warnings = NULL;
    cJSON_AddItemToObject(obj, "platforms", ctx->pending_platforms ? ctx->pending_platforms : cJSON_CreateArray());
    ctx->pending_platforms = NULL;
    cJSON_AddItemToObject(obj, "checks", ctx->pending_checks ? ctx->pending_checks : cJSON_CreateArray());
    ctx->pending_checks = NULL;
    cJSON_AddItemToObject(obj, "references", ctx->pending_references ? ctx->pending_references : cJSON_CreateArray());
    ctx->pending_references = NULL;

    free(ctx->pending_title);
    ctx->pending_title = NULL;
    free(ctx->pending_description);
    ctx->pending_description = NULL;
    free(ctx->pending_rationale);
    ctx->pending_rationale = NULL;
    free(ctx->pending_question);
    ctx->pending_question = NULL;

    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);

    if (json != NULL) scan_context_push(ctx, json);
    return 0;
}

// ---------------------------------------------------------------------
// Le thread producteur
// ---------------------------------------------------------------------


static void *producer_main(void *arg) {
    struct scan_context *ctx = arg;
    pthread_mutex_lock(&g_scan_serialize_mutex);

    struct xccdf_session *session = NULL; // initialisé AVANT tout goto possible

    char ds_path[256];
    int n = snprintf(ds_path, sizeof(ds_path), "../data/%s/ssg-%s-ds.xml", ctx->benchmark_id, ctx->benchmark_id);
    if (n < 0 || (size_t)n >= sizeof(ds_path)) {
        goto fail;
    }

    session = xccdf_session_new(ds_path);
    if (session == NULL) {
        goto fail;
    }

    if (xccdf_session_load_xccdf(session) != 0) {
        goto fail;
    }

    struct xccdf_policy_model *policy_model = xccdf_session_get_policy_model(session);
    if (policy_model == NULL) {
        goto fail;
    }

    struct xccdf_benchmark *benchmark = xccdf_policy_model_get_benchmark(policy_model);
    if (benchmark == NULL) {
        goto fail;
    }

    // ordre aligné sur resolve_profile_context / xccdf_policy_model_create_policy_by_id
    // (openscap, policy.c) : "Tailoring profiles take precedence over Benchmark
    // profiles." — donc le tailoring est vérifié EN PREMIER, le natif seulement
    // en repli si le profil n'y est pas trouvé
    char tailoring_path[256];
    int n2 = snprintf(tailoring_path, sizeof(tailoring_path), "../data/%s/ssg-%s-tailoring.xml", ctx->benchmark_id, ctx->benchmark_id);
    if (n2 < 0 || (size_t)n2 >= sizeof(tailoring_path)) {
        goto fail;
    }

    if (access(tailoring_path, F_OK) == 0) {
        struct oscap_source *check_source = oscap_source_new_from_file(tailoring_path);
        if (check_source == NULL) {
            goto fail;
        }

        struct xccdf_tailoring *check_tailoring = xccdf_tailoring_import_source(check_source, benchmark);
        if (check_tailoring == NULL) {
            oscap_source_free(check_source);
            goto fail;
        }
        oscap_source_free(check_source);

        // capturé AVANT le free — found_in_tailoring survivrait au free (dangling
        // pointer), donc on extrait le résultat de la comparaison en booléen
        // pendant que check_tailoring est encore valide
        bool profile_in_tailoring = (xccdf_tailoring_get_profile_by_id(check_tailoring, ctx->profile_id) != NULL);
        xccdf_tailoring_free(check_tailoring);

        if (profile_in_tailoring) {
            xccdf_session_set_user_tailoring_file(session, tailoring_path);
            
            if (xccdf_session_load_tailoring(session) != 0) {
                goto fail;
            }
        }
    }
        // sinon : le tailoring existe mais ne contient pas ce profil -> on
        // continue sans le charger, repli implicite sur le benchmark natif
        // via xccdf_session_set_profile_id ci-dessous
        // sinon (pas de fichier tailoring du tout) : repli direct sur le natif

    if (!xccdf_session_set_profile_id(session, ctx->profile_id)) {
        goto fail; // profil introuvable, ni dans le tailoring (si chargé), ni dans le natif
    }

    ctx->policy = xccdf_policy_model_get_policy_by_id(policy_model, ctx->profile_id);
    if (ctx->policy == NULL) {
        goto fail;
    }

    int selected_count = xccdf_policy_get_selected_rules_count(ctx->policy);

    scan_context_reserve(ctx, selected_count);

    if (xccdf_session_load_cpe(session) != 0) {
        goto fail;
    }

    if (xccdf_session_load_oval(session) != 0) {
        goto fail;
    }

    xccdf_policy_model_register_start_callback(policy_model, scan_start_callback, ctx);
    xccdf_policy_model_register_output_callback(policy_model, scan_output_callback, ctx);

    if (xccdf_session_evaluate(session) != 0) {
        goto fail;
    }

    double score = xccdf_session_get_base_score(session);

    xccdf_session_free(session);

    pthread_mutex_unlock(&g_scan_serialize_mutex);

    pthread_mutex_lock(&ctx->mutex);
    ctx->final_score = score;
    ctx->state = SCAN_SUCCEEDED;
    pthread_mutex_unlock(&ctx->mutex);

    producer_finish(ctx);
    return NULL;

fail:
    if (session != NULL) {
        xccdf_session_free(session);
    }
    pthread_mutex_unlock(&g_scan_serialize_mutex);

    pthread_mutex_lock(&ctx->mutex);
    ctx->state = SCAN_FAILED;
    pthread_mutex_unlock(&ctx->mutex);

    producer_finish(ctx);
    return NULL;
}
// ---------------------------------------------------------------------
// API publique
// ---------------------------------------------------------------------

struct scan_context *scan_context_new(const char *benchmark_id, const char *profile_id) {
    if (!is_valid_id_component(benchmark_id) || !is_valid_id_component(profile_id)) {
        return NULL;
    }

    struct scan_context *ctx = calloc(1, sizeof(struct scan_context));
    if (ctx == NULL) return NULL;

    // rejette plutôt que tronquer silencieusement (point 6 de l'audit) — un
    // ID plus long que le buffer ne doit jamais faire scanner un profil
    // différent de celui réellement demandé, sans le signaler
    if (strlen(benchmark_id) >= sizeof(ctx->benchmark_id) ||
        strlen(profile_id) >= sizeof(ctx->profile_id)) {
        free(ctx);
        return NULL;
    }

    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }

    ctx->ref_count = 2;
    strncpy(ctx->benchmark_id, benchmark_id, sizeof(ctx->benchmark_id) - 1);
    strncpy(ctx->profile_id, profile_id, sizeof(ctx->profile_id) - 1);
    return ctx;
}

void scan_context_set_connection(struct scan_context *ctx, struct MHD_Connection *connection) {
    ctx->connection = connection;
}

bool scan_context_start(struct scan_context *ctx) {
    int ret = pthread_create(&ctx->producer_thread, NULL, producer_main, ctx);
    if (ret != 0) {
        return false;
    }
    return true;
}

void scan_context_abort(struct scan_context *ctx) {
    scan_context_free(ctx);
}

void consumer_finish(void *cls) {
    struct scan_context *ctx = cls;
    if (ctx == NULL) return;

    pthread_mutex_lock(&ctx->mutex);
    ctx->cancelled = true;
    int remaining = --ctx->ref_count;
    pthread_mutex_unlock(&ctx->mutex);

    if (remaining == 0) {
        pthread_join(ctx->producer_thread, NULL);
        scan_context_free(ctx);
    }
}

ssize_t scan_reader_callback(void *cls, uint64_t pos, char *buf, size_t max) {
    struct scan_context *ctx = cls;

    pthread_mutex_lock(&ctx->mutex);


    if (ctx->read_index < ctx->count) {
        const char *item = ctx->items[ctx->read_index];
        size_t item_len = strlen(item);
        size_t remaining = item_len - ctx->item_offset;
        size_t to_copy = remaining < max ? remaining : max;

        memcpy(buf, item + ctx->item_offset, to_copy);
        ctx->item_offset += to_copy;

        if (ctx->item_offset == item_len) {
            // item entièrement transmis (potentiellement sur plusieurs appels
            // si sa taille dépassait max) : on passe au suivant
            ctx->item_offset = 0;
            ctx->read_index++;
        }

        pthread_mutex_unlock(&ctx->mutex);
        return (ssize_t)to_copy;
    }

    switch (ctx->state) {
        case SCAN_SUCCEEDED: {
            char done_msg[128];
            int n = snprintf(done_msg, sizeof(done_msg), "data: {\"type\":\"done\",\"score\":%f}\n\n", ctx->final_score);
            if (n < 0) {
                pthread_mutex_unlock(&ctx->mutex);
                return MHD_CONTENT_READER_END_WITH_ERROR;
            }
            size_t written = strlen(done_msg); // correct même si snprintf a tronqué,
                                                // contrairement à `n` qui peut dépasser sizeof(done_msg)
            size_t to_copy = (written < max) ? written : max;
            memcpy(buf, done_msg, to_copy);
            ctx->state = SCAN_DONE_SENT;
            pthread_mutex_unlock(&ctx->mutex);
            return (ssize_t)to_copy;
        }
        case SCAN_DONE_SENT:
            pthread_mutex_unlock(&ctx->mutex);
            return MHD_CONTENT_READER_END_OF_STREAM;
        case SCAN_FAILED:
            pthread_mutex_unlock(&ctx->mutex);
            return MHD_CONTENT_READER_END_WITH_ERROR;
        case SCAN_RUNNING:
        default:
            ctx->suspended = true;
            MHD_suspend_connection(ctx->connection);   // <-- appelé AVANT de déverrouiller
            pthread_mutex_unlock(&ctx->mutex);
            return 0;
    }
}