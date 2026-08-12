#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <oscap_error.h>
#include "scan_stream.h"
#include "scap_service.h"



static void push_event(struct scan_context *ctx, char *json) {
    pthread_mutex_lock(&ctx->mutex);
    if (ctx->count == ctx->capacity) {
        int new_cap = ctx->capacity == 0 ? 16 : ctx->capacity * 2;
        char **tmp = realloc(ctx->items, new_cap * sizeof(char *));
        if (tmp == NULL) {
            pthread_mutex_unlock(&ctx->mutex);
            free(json);
            return; // événement perdu plutôt que crash ; le score final reste correct
        }
        ctx->items = tmp;
        ctx->capacity = new_cap;
    }
    ctx->items[ctx->count++] = json;
    pthread_mutex_unlock(&ctx->mutex);

    MHD_resume_connection(ctx->connection);
}

static int scan_start_callback(struct xccdf_rule *rule, void *usr) {
    struct scan_context *ctx = usr;

    const char *rule_id = xccdf_rule_get_id(rule);
    bool selected = xccdf_policy_is_item_selected(ctx->policy, rule_id);
    if (!selected) {
        return 0;
    }

    char *title = xccdf_policy_get_readable_item_title(ctx->policy, (struct xccdf_item *)rule, NULL);

    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "type", "start");
    cJSON_AddStringToObject(obj, "rule_id", rule_id ? rule_id : "");
    cJSON_AddStringToObject(obj, "title", title ? title : "");
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);

    free(title); // xccdf_policy_get_readable_item_title alloue, contrairement à get_rule_title

    if (json != NULL) push_event(ctx, json);
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
    cJSON_AddStringToObject(obj, "rule_id", xccdf_rule_result_get_idref(rule_result));
    cJSON_AddStringToObject(obj, "status", status);
    cJSON_AddStringToObject(obj, "severity", severity_to_str(xccdf_rule_result_get_severity(rule_result)));
    char *json = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);

    if (json != NULL) push_event(ctx, json);
    return 0;
}

static void *producer_main(void *arg) {
    struct scan_context *ctx = arg;

    oscap_init();

    char ds_path[256];
    snprintf(ds_path, sizeof(ds_path), "../data/%s/ssg-%s-ds.xml", ctx->benchmark_id, ctx->benchmark_id);

    char tailoring_path[256];
    snprintf(tailoring_path, sizeof(tailoring_path), "../data/%s/ssg-%s-tailoring.xml", ctx->benchmark_id, ctx->benchmark_id);

    fprintf(stderr, "[scan] ds_path=%s\n", ds_path);
    fprintf(stderr, "[scan] tailoring_path=%s (exists=%d)\n", tailoring_path, access(tailoring_path, F_OK) == 0);

    struct xccdf_session *session = xccdf_session_new(ds_path);
    if (session == NULL) {
        fprintf(stderr, "[scan] FAIL: xccdf_session_new returned NULL\n");
        goto fail;
    }

    if (access(tailoring_path, F_OK) == 0) {
        xccdf_session_set_user_tailoring_file(session, tailoring_path);
    }

    int load_ret;

    load_ret = xccdf_session_load_xccdf(session);
    fprintf(stderr, "[scan] xccdf_session_load_xccdf returned %d\n", load_ret);
    if (load_ret != 0) {
        fprintf(stderr, "[scan] FAIL: load_xccdf failed\n");
        xccdf_session_free(session);
        goto fail;
    }

    load_ret = xccdf_session_load_tailoring(session);
    fprintf(stderr, "[scan] xccdf_session_load_tailoring returned %d\n", load_ret);
    if (load_ret != 0) {
        const char *err_desc = oscap_err_desc();
        fprintf(stderr, "[scan] FAIL: load_tailoring failed - oscap error: %s\n", err_desc ? err_desc : "(none)");
        xccdf_session_free(session);
        goto fail;
    }

    struct xccdf_policy_model *policy_model = xccdf_session_get_policy_model(session);

    bool set_ret = xccdf_session_set_profile_id(session, ctx->profile_id);
    fprintf(stderr, "[scan] xccdf_session_set_profile_id(%s) returned %d\n", ctx->profile_id, set_ret);
    if (!set_ret) {
        fprintf(stderr, "[scan] FAIL: set_profile_id failed\n");
        xccdf_session_free(session);
        goto fail;
    }

    ctx->policy = xccdf_policy_model_get_policy_by_id(policy_model, ctx->profile_id);
    if (ctx->policy == NULL) {
        fprintf(stderr, "[scan] FAIL: no policy found for profile_id=%s\n", ctx->profile_id);
        xccdf_session_free(session);
        goto fail;
    }

    // profil validé : on ne charge CPE/OVAL que maintenant, pour ne pas
    // payer ce coût si le profile_id était invalide
    load_ret = xccdf_session_load_cpe(session);
    fprintf(stderr, "[scan] xccdf_session_load_cpe returned %d\n", load_ret);
    if (load_ret != 0) {
        fprintf(stderr, "[scan] FAIL: load_cpe failed\n");
        xccdf_session_free(session);
        goto fail;
    }

    load_ret = xccdf_session_load_oval(session);
    fprintf(stderr, "[scan] xccdf_session_load_oval returned %d\n", load_ret);
    if (load_ret != 0) {
        fprintf(stderr, "[scan] FAIL: load_oval failed\n");
        xccdf_session_free(session);
        goto fail;
    }

    xccdf_policy_model_register_start_callback(policy_model, scan_start_callback, ctx);
    xccdf_policy_model_register_output_callback(policy_model, scan_output_callback, ctx);

    int eval_ret = xccdf_session_evaluate(session);
    fprintf(stderr, "[scan] xccdf_session_evaluate returned %d\n", eval_ret);
    if (eval_ret != 0) {
        fprintf(stderr, "[scan] FAIL: evaluate failed\n");
        xccdf_session_free(session);
        goto fail;
    }

    pthread_mutex_lock(&ctx->mutex);
    ctx->final_score = xccdf_session_get_base_score(session);
    ctx->finished = true;
    pthread_mutex_unlock(&ctx->mutex);

    fprintf(stderr, "[scan] SUCCESS: %d events pushed\n", ctx->count);

    xccdf_session_free(session);
    oscap_cleanup();
    MHD_resume_connection(ctx->connection);
    return NULL;

fail:
    pthread_mutex_lock(&ctx->mutex);
    ctx->failed = true;
    pthread_mutex_unlock(&ctx->mutex);
    oscap_cleanup();
    MHD_resume_connection(ctx->connection);
    return NULL;
}

struct scan_context *scan_context_new(const char *benchmark_id, const char *profile_id) {
    struct scan_context *ctx = calloc(1, sizeof(struct scan_context));
    if (ctx == NULL) return NULL;
    pthread_mutex_init(&ctx->mutex, NULL);
    strncpy(ctx->benchmark_id, benchmark_id, sizeof(ctx->benchmark_id) - 1);
    strncpy(ctx->profile_id, profile_id, sizeof(ctx->profile_id) - 1);
    return ctx;
}

void scan_context_set_connection(struct scan_context *ctx, struct MHD_Connection *connection) {
    ctx->connection = connection;
}

void scan_context_start(struct scan_context *ctx) {
    pthread_create(&ctx->producer_thread, NULL, producer_main, ctx);
}

void scan_context_free(void *cls) {
    struct scan_context *ctx = cls;
    if (ctx == NULL) return;

    pthread_join(ctx->producer_thread, NULL);
    for (int i = 0; i < ctx->count; i++) free(ctx->items[i]);
    free(ctx->items);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
}

ssize_t scan_reader_callback(void *cls, uint64_t pos, char *buf, size_t max) {
    struct scan_context *ctx = cls;

    pthread_mutex_lock(&ctx->mutex);

    if (ctx->read_index < ctx->count) {
        int n = snprintf(buf, max, "data: %s\n\n", ctx->items[ctx->read_index]);
        ctx->read_index++;
        pthread_mutex_unlock(&ctx->mutex);
        return (n < 0) ? MHD_CONTENT_READER_END_WITH_ERROR : n;
    }

    if (ctx->finished) {
        if (!ctx->done_sent) {
            int n = snprintf(buf, max, "data: {\"type\":\"done\",\"score\":%f}\n\n", ctx->final_score);
            ctx->done_sent = true;
            pthread_mutex_unlock(&ctx->mutex);
            return (n < 0) ? MHD_CONTENT_READER_END_WITH_ERROR : n;
        }
        pthread_mutex_unlock(&ctx->mutex);
        return MHD_CONTENT_READER_END_OF_STREAM;
    }

    if (ctx->failed) {
        pthread_mutex_unlock(&ctx->mutex);
        return MHD_CONTENT_READER_END_WITH_ERROR;
    }

    pthread_mutex_unlock(&ctx->mutex);
    MHD_suspend_connection(ctx->connection);
    return 0;
}