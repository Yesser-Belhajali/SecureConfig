#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <oscap_source.h>
#include <oscap_error.h>
#include "remediate.h"
#include "scap_service.h"

// ---------------------------------------------------------------------
// La file : identique à scan_context_push, adaptée au type remediate_context
// ---------------------------------------------------------------------

static void remediate_context_push(struct remediate_context *ctx, char *json) {
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
        MHD_resume_connection(ctx->connection);
    }
    pthread_mutex_unlock(&ctx->mutex);
}

static void remediate_context_reserve(struct remediate_context *ctx, int capacity) {
    if (capacity <= 0) return;
    char **tmp = malloc(capacity * sizeof(char *));
    if (tmp == NULL) return;
    ctx->items = tmp;
    ctx->capacity = capacity;
}

// ---------------------------------------------------------------------
// Même sérialisation globale que le scan : un seul run OpenSCAP à la fois
// dans tout le processus. Mutex SÉPARÉ de g_scan_serialize_mutex (scan.c) —
// volontaire : un scan classique et une remédiation ne devraient normalement
// jamais s'exécuter en même temps de toute façon (l'utilisateur fait l'un
// puis l'autre dans l'UI), donc ce choix n'introduit pas de concurrence
// nouvelle ; mais si jamais les deux étaient déclenchés en parallèle par
// deux onglets différents, deux mutex séparés éviteraient un blocage
// artificiel entre les deux plutôt que de forcer un unique verrou global.
// À reconsidérer si un jour les deux DOIVENT rester mutuellement exclusifs.
static pthread_mutex_t g_remediate_serialize_mutex = PTHREAD_MUTEX_INITIALIZER;

static void remediate_context_free(struct remediate_context *ctx) {
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
    for (int i = 0; i < ctx->rule_count; i++) free(ctx->rule_ids[i]);
    free(ctx->rule_ids);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
}

static void producer_finish(struct remediate_context *ctx) {
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
        remediate_context_free(ctx);
    }
}

// ---------------------------------------------------------------------
// Callbacks OpenSCAP — dupliqués depuis scan.c à l'identique (mêmes
// helpers build_*_array), volontairement : un fichier commun casserait
// l'indépendance recherchée entre le scan classique et la remédiation
// (cf. discussion précédente sur producer_main dupliqué plutôt que
// complexifié). Toute correction de bug dans l'un doit être répercutée
// manuellement dans l'autre.
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
        const char *text = oscap_reference_get_title(ref);

        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "href", href ? href : "");
        cJSON_AddStringToObject(obj, "text", text ? text : "");
        cJSON_AddItemToArray(arr, obj);
    }
    oscap_reference_iterator_free(it);
    return arr;
}

static int remediate_start_callback(struct xccdf_rule *rule, void *usr) {
    struct remediate_context *ctx = usr;

    pthread_mutex_lock(&ctx->mutex);
    bool cancelled = ctx->cancelled;
    pthread_mutex_unlock(&ctx->mutex);

    if (cancelled) {
        return 1;
    }

    free(ctx->pending_title); ctx->pending_title = NULL;
    free(ctx->pending_description); ctx->pending_description = NULL;
    free(ctx->pending_rationale); ctx->pending_rationale = NULL;
    free(ctx->pending_question); ctx->pending_question = NULL;
    cJSON_Delete(ctx->pending_fixes); ctx->pending_fixes = NULL;
    cJSON_Delete(ctx->pending_warnings); ctx->pending_warnings = NULL;
    cJSON_Delete(ctx->pending_platforms); ctx->pending_platforms = NULL;
    cJSON_Delete(ctx->pending_checks); ctx->pending_checks = NULL;
    cJSON_Delete(ctx->pending_references); ctx->pending_references = NULL;

    const char *rule_id = xccdf_rule_get_id(rule);
    bool selected = xccdf_policy_is_item_selected(ctx->policy, rule_id);

    if (!selected) {
        return 0;
    }

    ctx->pending_title = xccdf_policy_get_readable_item_title(ctx->policy, (struct xccdf_item *)rule, NULL);
    ctx->pending_description = xccdf_policy_get_readable_item_description(ctx->policy, (struct xccdf_item *)rule, NULL);
    ctx->pending_rationale = xccdf_policy_get_readable_item_rationale(ctx->policy, (struct xccdf_item *)rule, NULL);

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

static int remediate_output_callback(struct xccdf_rule_result *rule_result, void *usr) {
    struct remediate_context *ctx = usr;

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
    cJSON_AddStringToObject(obj, "id", xccdf_rule_result_get_idref(rule_result) ? xccdf_rule_result_get_idref(rule_result) : "");
    cJSON_AddStringToObject(obj, "title", ctx->pending_title ? ctx->pending_title : "");
    cJSON_AddStringToObject(obj, "description", ctx->pending_description ? ctx->pending_description : "");
    cJSON_AddStringToObject(obj, "rationale", ctx->pending_rationale ? ctx->pending_rationale : "");
    cJSON_AddStringToObject(obj, "question", ctx->pending_question ? ctx->pending_question : "");
    cJSON_AddStringToObject(obj, "status", status);
    cJSON_AddStringToObject(obj, "severity", severity_to_str(xccdf_rule_result_get_severity(rule_result)));

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

    free(ctx->pending_title); ctx->pending_title = NULL;
    free(ctx->pending_description); ctx->pending_description = NULL;
    free(ctx->pending_rationale); ctx->pending_rationale = NULL;
    free(ctx->pending_question); ctx->pending_question = NULL;

    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);

    if (json != NULL) remediate_context_push(ctx, json);
    return 0;
}

// ---------------------------------------------------------------------
// Le thread producteur
// ---------------------------------------------------------------------

static void *producer_main(void *arg) {
    struct remediate_context *ctx = arg;
    pthread_mutex_lock(&g_remediate_serialize_mutex);

    struct xccdf_session *session = NULL;

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
    // en repli si le profil n'y est pas trouvé. Remplace profile_is_tailoring
    // (préfixe hardcodé, supprimée) par une vérification réelle dans le fichier.
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

        // capturé en booléen AVANT le free — le pointeur retourné par
        // get_profile_by_id devient invalide dès que check_tailoring est libéré
        bool profile_in_tailoring = (xccdf_tailoring_get_profile_by_id(check_tailoring, ctx->profile_id) != NULL);
        xccdf_tailoring_free(check_tailoring);

        if (profile_in_tailoring) {
            xccdf_session_set_user_tailoring_file(session, tailoring_path);

            if (xccdf_session_load_tailoring(session) != 0) {
                goto fail;
            }
        }
        // sinon : tailoring existe mais ne contient pas ce profil -> repli
        // implicite sur le natif via set_profile_id ci-dessous
    }
    // sinon (pas de fichier tailoring) : repli direct sur le natif

    if (!xccdf_session_set_profile_id(session, ctx->profile_id)) {
        goto fail;
    }

    ctx->policy = xccdf_policy_model_get_policy_by_id(policy_model, ctx->profile_id);
    if (ctx->policy == NULL) {
        goto fail;
    }

    // restreint l'évaluation ET la remédiation aux seules règles choisies —
    // cf. discussion : dès qu'add_rule est appelé au moins une fois, seules
    // les règles ajoutées sont évaluées (_user_specified_rule_mode > 0 dans
    // libopenscap), donc le TestResult produit ne contiendra QUE ces règles,
    // et xccdf_session_remediate() qui opère sur ce TestResult sera de facto
    // borné aux mêmes règles, sans logique de filtrage supplémentaire à écrire
    for (int i = 0; i < ctx->rule_count; i++) {
        xccdf_session_add_rule(session, ctx->rule_ids[i]);
    }

    remediate_context_reserve(ctx, ctx->rule_count);

    if (xccdf_session_load_cpe(session) != 0) {
        goto fail;
    }
    if (xccdf_session_load_oval(session) != 0) {
        goto fail;
    }

    xccdf_policy_model_register_start_callback(policy_model, remediate_start_callback, ctx);
    xccdf_policy_model_register_output_callback(policy_model, remediate_output_callback, ctx);

    if (xccdf_session_evaluate(session) != 0) {
        goto fail;
    }

    // les callbacks se redéclenchent ici (xccdf_policy_rule_result_remediate
    // appelle XCCDF_POLICY_OUTCB_START/END exactement comme l'évaluation
    // normale) : un second event SSE "FIXED"/"ERROR" par règle corrigée,
    // en plus du "FAIL" déjà envoyé par evaluate() ci-dessus
    if (xccdf_session_remediate(session) != 0) {
        goto fail;
    }

    // appelé APRÈS remediate (qui recalcule le score en interne via
    // xccdf_policy_recalculate_score) — reflète donc l'état post-remédiation
    double score = xccdf_session_get_base_score(session);

    xccdf_session_free(session);
    pthread_mutex_unlock(&g_remediate_serialize_mutex);

    pthread_mutex_lock(&ctx->mutex);
    ctx->final_score = score;
    ctx->state = REMEDIATE_SUCCEEDED;
    pthread_mutex_unlock(&ctx->mutex);

    producer_finish(ctx);
    return NULL;


fail:
    if (session != NULL) {
        if (oscap_err()) {
            const char *err_desc = oscap_err_desc();
            if (err_desc != NULL) {
                cJSON *err_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(err_obj, "type", "error");
                cJSON_AddStringToObject(err_obj, "message", err_desc);
                char *err_json = cJSON_PrintUnformatted(err_obj);
                cJSON_Delete(err_obj);
                if (err_json != NULL) {
                    remediate_context_push(ctx, err_json);
                }
            }
            oscap_clearerr();
        }
        xccdf_session_free(session);
    }
    pthread_mutex_unlock(&g_remediate_serialize_mutex);

    pthread_mutex_lock(&ctx->mutex);
    ctx->state = REMEDIATE_FAILED;
    pthread_mutex_unlock(&ctx->mutex);

    producer_finish(ctx);
    return NULL;
}

// ---------------------------------------------------------------------
// API publique
// ---------------------------------------------------------------------

struct remediate_context *remediate_context_new(const char *benchmark_id, const char *profile_id,
                                                   const char **rule_ids, int rule_count) {
    if (rule_count <= 0) return NULL;

    struct remediate_context *ctx = calloc(1, sizeof(struct remediate_context));
    if (ctx == NULL) return NULL;

    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }

    char **dup_ids = calloc(rule_count, sizeof(char *));
    if (dup_ids == NULL) {
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        return NULL;
    }

    for (int i = 0; i < rule_count; i++) {
        dup_ids[i] = rule_ids[i] ? strdup(rule_ids[i]) : NULL;
        if (rule_ids[i] != NULL && dup_ids[i] == NULL) {
            // échec d'allocation en cours de copie : nettoyage de ce qui a déjà été dupliqué
            for (int j = 0; j < i; j++) free(dup_ids[j]);
            free(dup_ids);
            pthread_mutex_destroy(&ctx->mutex);
            free(ctx);
            return NULL;
        }
    }

    ctx->rule_ids = dup_ids;
    ctx->rule_count = rule_count;
    ctx->ref_count = 2;
    strncpy(ctx->benchmark_id, benchmark_id, sizeof(ctx->benchmark_id) - 1);
    strncpy(ctx->profile_id, profile_id, sizeof(ctx->profile_id) - 1);
    return ctx;
}

void remediate_context_set_connection(struct remediate_context *ctx, struct MHD_Connection *connection) {
    ctx->connection = connection;
}

bool remediate_context_start(struct remediate_context *ctx) {
    int ret = pthread_create(&ctx->producer_thread, NULL, producer_main, ctx);
    return ret == 0;
}

void remediate_context_abort(struct remediate_context *ctx) {
    remediate_context_free(ctx);
}

void remediate_consumer_finish(void *cls) {
    struct remediate_context *ctx = cls;
    if (ctx == NULL) return;

    pthread_mutex_lock(&ctx->mutex);
    ctx->cancelled = true;
    int remaining = --ctx->ref_count;
    pthread_mutex_unlock(&ctx->mutex);

    if (remaining == 0) {
        pthread_join(ctx->producer_thread, NULL);
        remediate_context_free(ctx);
    }
}

ssize_t remediate_reader_callback(void *cls, uint64_t pos, char *buf, size_t max) {
    struct remediate_context *ctx = cls;

    pthread_mutex_lock(&ctx->mutex);

    if (ctx->read_index < ctx->count) {
        const char *item = ctx->items[ctx->read_index];
        size_t item_len = strlen(item);
        size_t remaining = item_len - ctx->item_offset;
        size_t to_copy = remaining < max ? remaining : max;

        memcpy(buf, item + ctx->item_offset, to_copy);
        ctx->item_offset += to_copy;

        if (ctx->item_offset == item_len) {
            ctx->item_offset = 0;
            ctx->read_index++;
        }

        pthread_mutex_unlock(&ctx->mutex);
        return (ssize_t)to_copy;
    }

    switch (ctx->state) {
        case REMEDIATE_SUCCEEDED: {
            char done_msg[128];
            int n = snprintf(done_msg, sizeof(done_msg), "data: {\"type\":\"done\",\"score\":%f}\n\n", ctx->final_score);
            if (n < 0) {
                pthread_mutex_unlock(&ctx->mutex);
                return MHD_CONTENT_READER_END_WITH_ERROR;
            }
            size_t to_copy = ((size_t)n < max) ? (size_t)n : max;
            memcpy(buf, done_msg, to_copy);
            ctx->state = REMEDIATE_DONE_SENT;
            pthread_mutex_unlock(&ctx->mutex);
            return (ssize_t)to_copy;
        }
        case REMEDIATE_DONE_SENT:
            pthread_mutex_unlock(&ctx->mutex);
            return MHD_CONTENT_READER_END_OF_STREAM;
        case REMEDIATE_FAILED:
            pthread_mutex_unlock(&ctx->mutex);
            return MHD_CONTENT_READER_END_WITH_ERROR;
        case REMEDIATE_RUNNING:
        default:
            ctx->suspended = true;
            MHD_suspend_connection(ctx->connection);
            pthread_mutex_unlock(&ctx->mutex);
            return 0;
    }
}


int validate_remediation_request(const char *benchmark_id, const char *profile_id,
                                   const char **rule_ids, int rule_count,
                                   char *out_invalid_id, size_t out_invalid_id_size){

    if(benchmark_id==NULL || profile_id==NULL || rule_ids==NULL || rule_count<=0){
        return -1;
    }

    char ds_path[256], tailoring_path[256];
    int n1 = snprintf(ds_path, sizeof(ds_path), "../data/%s/ssg-%s-ds.xml", benchmark_id, benchmark_id);
    int n2 = snprintf(tailoring_path, sizeof(tailoring_path), "../data/%s/ssg-%s-tailoring.xml", benchmark_id, benchmark_id);
    if(n1 < 0 || (size_t)n1 >= sizeof(ds_path) || n2 < 0 || (size_t)n2 >= sizeof(tailoring_path)){
        return -1;
    }

    struct xccdf_benchmark *benchmark = NULL;
    if(load_benchmark_from_ds(ds_path, &benchmark) != 0){
        return -1;
    }

    // rule_ids validés d'abord — indépendant du profil, ne nécessite que le
    // benchmark brut (pas encore de policy_model/tailoring à ce stade)
    int bad = find_invalid_rule_id(benchmark, rule_ids, rule_count);
    if(bad >= 0){
        report_invalid_id(rule_ids[bad], out_invalid_id, out_invalid_id_size);
        xccdf_benchmark_free(benchmark);
        return -8;
    }

    // résolution du profil, dupliquée volontairement plutôt que de réutiliser
    // resolve_profile_context : cette fonction a une responsabilité différente
    // (construire un contexte réutilisable pour lire/écrire un tailoring),
    // alors qu'ici on a seulement besoin, ponctuellement, d'un xccdf_policy
    // pour vérifier l'appartenance des règles — même ordre tailoring-first
    struct xccdf_tailoring *tailoring = NULL;
    struct xccdf_profile *profile = NULL;

    if(access(tailoring_path, F_OK) == 0){
        struct oscap_source *tailoring_source = oscap_source_new_from_file(tailoring_path);
        if(tailoring_source == NULL){
            xccdf_benchmark_free(benchmark);
            return -1;
        }

        tailoring = xccdf_tailoring_import_source(tailoring_source, benchmark);
        if(tailoring == NULL){
            oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            return -1;
        }
        oscap_source_free(tailoring_source);

        profile = xccdf_tailoring_get_profile_by_id(tailoring, profile_id);
    }

    if(profile == NULL){
        profile = xccdf_benchmark_get_profile_by_id(benchmark, profile_id);
    }

    if(profile == NULL){
        if(tailoring != NULL) xccdf_tailoring_free(tailoring);
        xccdf_benchmark_free(benchmark);
        return -3;
    }

    struct xccdf_policy_model *policy_model = xccdf_policy_model_new(benchmark);
    if(policy_model == NULL){
        if(tailoring != NULL) xccdf_tailoring_free(tailoring);
        xccdf_benchmark_free(benchmark);
        return -1;
    }

    if(tailoring != NULL){
        xccdf_policy_model_set_tailoring(policy_model, tailoring);
    }

    struct xccdf_policy *policy = xccdf_policy_new(policy_model, profile);
    if(policy == NULL){
        xccdf_policy_model_free(policy_model); // libère benchmark + tailoring
        return -1;
    }

    // vérifie l'appartenance de chaque règle à LA SÉLECTION de ce profil précis
    // — find_invalid_rule_id garantit déjà l'existence dans le benchmark,
    // ici on vérifie en plus policy->selected_final via l'API publique dédiée
    for(int i = 0; i < rule_count; i++){
        if(!xccdf_policy_is_item_selected(policy, rule_ids[i])){
            report_invalid_id(rule_ids[i], out_invalid_id, out_invalid_id_size);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            return -9;
        }
    }

    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model); // libère benchmark + tailoring en interne

    return 0;
}