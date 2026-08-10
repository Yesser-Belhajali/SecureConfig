#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <ds_sds_session.h>
#include <oscap_source.h>
#include "scap_service.h"



const char *get_profile_title(struct xccdf_profile *profile) {
    struct oscap_text_iterator *title_it = xccdf_profile_get_title(profile);
    const char *title = NULL;
    if (title_it != NULL && oscap_text_iterator_has_more(title_it)) {
        struct oscap_text *text = oscap_text_iterator_next(title_it);
        title = oscap_text_get_text(text);
    }
    if (title_it != NULL) {
        oscap_text_iterator_free(title_it);
    }
    return title;
}

const char *get_profile_description(struct xccdf_profile *profile) {
    struct oscap_text_iterator *description_it = xccdf_profile_get_description(profile);
    const char *description = NULL;
    if (description_it != NULL && oscap_text_iterator_has_more(description_it)) {
        struct oscap_text *text = oscap_text_iterator_next(description_it);
        description = oscap_text_get_text(text);
    }
    if (description_it != NULL) {
        oscap_text_iterator_free(description_it);
    }
    return description;
}

const char *get_rule_title(struct xccdf_rule *rule) {
    struct oscap_text_iterator *title_it = xccdf_rule_get_title(rule);
    const char *title = NULL;
    if (title_it!=NULL && oscap_text_iterator_has_more(title_it)) {
        struct oscap_text *text = oscap_text_iterator_next(title_it);
        title = oscap_text_get_text(text);
    }
    if(title_it!=NULL){
        oscap_text_iterator_free(title_it);
    }
    return title;
}

const char *get_rule_description(struct xccdf_rule *rule){
    struct oscap_text_iterator *description_it=xccdf_rule_get_description(rule);
    const char*description=NULL;
    if(description_it!=NULL && oscap_text_iterator_has_more(description_it)){
        struct oscap_text *text=oscap_text_iterator_next(description_it);
        description=oscap_text_get_text(text);
    }
    if(description_it!=NULL){
        oscap_text_iterator_free(description_it);
    }
    return description;
}

const char *get_rule_rationale(struct xccdf_rule *rule){
    struct oscap_text_iterator *rationale_it=xccdf_rule_get_rationale(rule);
    const char *rationale=NULL;
    if(rationale_it!=NULL && oscap_text_iterator_has_more(rationale_it)){
        struct oscap_text *text=oscap_text_iterator_next(rationale_it);
        rationale=oscap_text_get_text(text);
    }
    if(rationale_it!=NULL){
        oscap_text_iterator_free(rationale_it);
    }
    return rationale;
}

const char *get_rule_severity(struct xccdf_rule *rule){
    const char *rule_severity="Not Defined";
    xccdf_level_t severity_type=xccdf_rule_get_severity(rule);

    switch (severity_type) {
        case XCCDF_LEVEL_NOT_DEFINED:
            rule_severity = "Not Defined";
            break;

        case XCCDF_UNKNOWN:
            rule_severity = "Unknown";
            break;

        case XCCDF_INFO:
            rule_severity = "Info";
            break;

        case XCCDF_LOW:
            rule_severity = "Low";
            break;

        case XCCDF_MEDIUM:
            rule_severity = "Medium";
            break;

        case XCCDF_HIGH:
            rule_severity = "High";
            break;
    }
    return rule_severity;
}


struct rule_node{
    struct xccdf_rule *rule;
    struct rule_node *next;
};

struct rule_node *push_front(struct rule_node *head,struct xccdf_rule *rule,bool *error){
    struct rule_node *new_rule_node=malloc(sizeof(struct rule_node));
    if(new_rule_node==NULL){
        printf("Echec dans la création d'un noeud rule_node!!!!!\n");
        *error=true;
        return head;
    }
    new_rule_node->rule=rule;
    new_rule_node->next=head;
    return new_rule_node;
}

void free_rule_list(struct rule_node *head){
    while(head!=NULL){
        struct rule_node *new_rule_node=head;
        head=head->next;
        free(new_rule_node);
    }
}

struct rule_node *collect_rules_recursive(struct xccdf_item *benchmark_item,struct rule_node *head,bool *error){
    if(*error){
        return head;
    }
    xccdf_type_t benchmark_item_type=xccdf_item_get_type(benchmark_item);
    if(benchmark_item_type==XCCDF_RULE){
        head=push_front(head,xccdf_item_to_rule(benchmark_item),error);
    }
    else if(benchmark_item_type==XCCDF_GROUP){
        struct xccdf_item_iterator *group_iterator=xccdf_group_get_content(xccdf_item_to_group(benchmark_item));
        if(group_iterator==NULL){
            printf("Erreur dans la création de l'itérateur du groupe!!!!\n");
            *error=true;
            return head;
        }
        while(xccdf_item_iterator_has_more(group_iterator) && *error==false){
            struct xccdf_item *group_item=xccdf_item_iterator_next(group_iterator);
            head=collect_rules_recursive(group_item,head,error);
        }
        xccdf_item_iterator_free(group_iterator);
    }
    return head;
}

struct rule_node *get_benchmark_rules(struct xccdf_benchmark *benchmark,bool *error){
    struct rule_node *head=NULL;
    struct xccdf_item_iterator *benchmark_iterator=xccdf_benchmark_get_content(benchmark);
    if(benchmark_iterator==NULL){
        printf("Erreur dans la création de l'itérateur du benchmark!!!!\n");
        *error=true;
        return NULL;
    }
    while(xccdf_item_iterator_has_more(benchmark_iterator) && *error==false){
        struct xccdf_item *benchmark_item=xccdf_item_iterator_next(benchmark_iterator);
        head=collect_rules_recursive(benchmark_item,head,error);
    }
    xccdf_item_iterator_free(benchmark_iterator);
    return head;
}

struct rule_node *get_benchmark_rules_or_null(struct xccdf_benchmark *benchmark){
    bool error=false;
    struct rule_node *head=get_benchmark_rules(benchmark,&error);
    if(error){
        free_rule_list(head);
        return NULL;
    }
    return head;
}

void free_profile_list(struct profile_list *profiles,int count){
    if(profiles==NULL){
        return;
    }
    for(int i=0;i<count;i++){
        free(profiles[i].id);
        free(profiles[i].title);
        free(profiles[i].description);
        free(profiles[i].extends);
    }
    free(profiles);
}


static int load_benchmark_from_ds(const char *ds_path, struct ds_sds_session **out_session, struct xccdf_benchmark **out_benchmark){
    *out_session = NULL;
    *out_benchmark = NULL;

    struct oscap_source *oscap_ds_source = oscap_source_new_from_file(ds_path);
    if(oscap_ds_source == NULL){
        return -1;
    }

    struct ds_sds_session *ds_sds_session = ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session == NULL){
        oscap_source_free(oscap_ds_source);
        return -1;
    }

    struct oscap_source *oscap_xccdf_source = ds_sds_session_select_checklist(ds_sds_session, NULL, NULL, NULL);
    if(oscap_xccdf_source == NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        return -1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark = xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark == NULL){
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    *out_session = ds_sds_session;
    *out_benchmark = benchmark;
    return 0;
}

static struct xccdf_profile *find_profile_by_id(struct xccdf_profile_iterator *it, const char *profile_id) {
    if (it == NULL) return NULL;
    while (xccdf_profile_iterator_has_more(it)) {
        struct xccdf_profile *p = xccdf_profile_iterator_next(it);
        const char *id = xccdf_profile_get_id(p);
        if (id != NULL && strcmp(id, profile_id) == 0) {
            return p;
        }
    }
    return NULL;
}

static int profiles_from_iterator(struct xccdf_profile_iterator *profile_iterator, struct profile_list **out_profiles){
    struct profile_list *profiles = NULL;
    int count = 0;

    while(xccdf_profile_iterator_has_more(profile_iterator)){
        struct xccdf_profile *profile = xccdf_profile_iterator_next(profile_iterator);
        if(profile == NULL){
            free_profile_list(profiles, count);
            return -1;
        }

        struct profile_list *tmp = realloc(profiles, (count+1) * sizeof(struct profile_list));
        if(tmp == NULL){
            free_profile_list(profiles, count);
            return -1;
        }
        profiles = tmp;

        profiles[count].id = NULL;
        profiles[count].title = NULL;
        profiles[count].description = NULL;
        profiles[count].extends = NULL;

        const char *id = xccdf_profile_get_id(profile);
        const char *title = get_profile_title(profile);
        const char *description = get_profile_description(profile);
        const char *extends = xccdf_profile_get_extends(profile);

        profiles[count].id = id ? strdup(id) : NULL;
        profiles[count].title = title ? strdup(title) : NULL;
        profiles[count].description = description ? strdup(description) : NULL;
        profiles[count].extends = extends ? strdup(extends) : NULL;

        if((id!=NULL && profiles[count].id==NULL) || (title!=NULL && profiles[count].title==NULL) || (description!=NULL && profiles[count].description==NULL) || (extends!=NULL && profiles[count].extends==NULL)){
            free(profiles[count].id);
            free(profiles[count].title);
            free(profiles[count].description);
            free(profiles[count].extends);
            free_profile_list(profiles, count);
            return -1;
        }
        count++;
    }

    *out_profiles = profiles;
    return count;
}

int list_profiles_for_distro(const char *id,struct profile_list **out_profiles, int *out_profiles_count,struct profile_list **out_tailoring_profiles, int *out_tailoring_count){

    if(out_profiles==NULL || out_profiles_count==NULL || out_tailoring_profiles==NULL || out_tailoring_count==NULL){
        return -1;
    }

    *out_profiles=NULL;
    *out_profiles_count=0;
    *out_tailoring_profiles=NULL;
    *out_tailoring_count=0;

    char ds_path[256];
    char tailoring_path[256];

    int n1=snprintf(ds_path,sizeof(ds_path),"../data/%s/ssg-%s-ds.xml",id,id);
    int n2=snprintf(tailoring_path,sizeof(tailoring_path),"../data/%s/ssg-%s-tailoring.xml",id,id);
    if(n1<0 || (size_t)n1>=sizeof(ds_path) || n2<0 || (size_t)n2>=sizeof(tailoring_path)){
        return -1;
    }

    oscap_init();

    struct ds_sds_session *ds_sds_session = NULL;
    struct xccdf_benchmark *benchmark = NULL;
    if(load_benchmark_from_ds(ds_path, &ds_sds_session, &benchmark) != 0){
        oscap_cleanup();
        return -1;
    }

    struct xccdf_profile_iterator *profile_iterator = xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator == NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct profile_list *profiles = NULL;
    int profiles_count = profiles_from_iterator(profile_iterator, &profiles);
    xccdf_profile_iterator_free(profile_iterator);

    if(profiles_count < 0){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct profile_list *tailoring_profiles = NULL;
    int tailoring_count = 0;

    if(access(tailoring_path, F_OK) == 0){
        struct oscap_source *oscap_tailoring_source = oscap_source_new_from_file(tailoring_path);
        if(oscap_tailoring_source == NULL){
            free_profile_list(profiles, profiles_count);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        struct xccdf_tailoring *tailoring = xccdf_tailoring_import_source(oscap_tailoring_source, benchmark);
        if(tailoring == NULL){
            oscap_source_free(oscap_tailoring_source);
            free_profile_list(profiles, profiles_count);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        struct xccdf_profile_iterator *tailoring_iterator = xccdf_tailoring_get_profiles(tailoring);
        if(tailoring_iterator == NULL){
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
            free_profile_list(profiles, profiles_count);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        tailoring_count = profiles_from_iterator(tailoring_iterator, &tailoring_profiles);
        xccdf_profile_iterator_free(tailoring_iterator);
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);

        if(tailoring_count < 0){
            free_profile_list(profiles, profiles_count);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
    }

    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();

    *out_profiles = profiles;
    *out_profiles_count = profiles_count;
    *out_tailoring_profiles = tailoring_profiles;
    *out_tailoring_count = tailoring_count;

    return 0;
}

void free_profiles_for_distro(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count){
    free_profile_list(profiles, profiles_count);
    free_profile_list(tailoring_profiles, tailoring_count);
}




void free_rule_info_list(struct rule_list *rules,int count){
    if(rules==NULL){
        return;
    }
    for(int i=0;i<count;i++){
        free(rules[i].id);
        free(rules[i].title);
        free(rules[i].description);
        free(rules[i].rationale);
        free(rules[i].severity);
    }
    free(rules);
}


int list_rules_for_ds(const char *ds_path,struct rule_list **out_rules){
    if(ds_path==NULL){
        return -1;
    }
    *out_rules=NULL;

    oscap_init();

    struct ds_sds_session *ds_sds_session = NULL;
    struct xccdf_benchmark *benchmark = NULL;
    if(load_benchmark_from_ds(ds_path, &ds_sds_session, &benchmark) != 0){
        oscap_cleanup();
        return -1;
    }

    struct rule_node *head=get_benchmark_rules_or_null(benchmark);
    if(head==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct rule_list *rules=NULL;
    int count=0;
    struct rule_node *iter=head;

    while(iter!=NULL){
        struct rule_list *tmp = realloc(rules, (count + 1) * sizeof(struct rule_list));
        if(tmp==NULL){
            free_rule_info_list(rules,count);
            free_rule_list(head);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
        rules=tmp;

        const char *id=xccdf_rule_get_id(iter->rule);
        const char *title=get_rule_title(iter->rule);
        const char *description=get_rule_description(iter->rule);
        const char *rationale=get_rule_rationale(iter->rule);
        const char *severity=get_rule_severity(iter->rule);

        rules[count].id = id ? strdup(id) : NULL;
        rules[count].title = title ? strdup(title) : NULL;
        rules[count].description = description ? strdup(description) : NULL;
        rules[count].rationale = rationale ? strdup(rationale) : NULL;
        rules[count].severity = severity ? strdup(severity) : NULL;
        rules[count].selected = false;

        if((id !=NULL && rules[count].id==NULL) || (title!=NULL && rules[count].title==NULL) || (description!=NULL && rules[count].description==NULL) || (rationale!=NULL && rules[count].rationale==NULL) || (severity!=NULL && rules[count].severity==NULL)){
            free(rules[count].id);
            free(rules[count].title);
            free(rules[count].description);
            free(rules[count].rationale);
            free(rules[count].severity);
            free_rule_info_list(rules,count);
            free_rule_list(head);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
        count++;
        iter=iter->next;
    }

    free_rule_list(head);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();

    *out_rules=rules;
    return count;
}


struct resolved_profile_context {
    struct ds_sds_session *ds_sds_session;
    struct xccdf_benchmark *benchmark;       // possédé par policy_model après création
    struct xccdf_policy_model *policy_model;
    struct oscap_source *tailoring_source;   // NULL si aucun tailoring utilisé
    struct xccdf_profile *profile;           // profil trouvé (natif ou tailoring)
};

// cherche profile_id d'abord dans le benchmark natif, puis dans le tailoring
// associé à benchmark_id s'il existe et si le profil n'a pas été trouvé avant
static int resolve_profile_context(const char *benchmark_id, const char *profile_id,struct resolved_profile_context *ctx) {
    
    memset(ctx, 0, sizeof(*ctx));

    char ds_path[256], tailoring_path[256];
    int n1 = snprintf(ds_path, sizeof(ds_path), "../data/%s/ssg-%s-ds.xml", benchmark_id, benchmark_id);
    int n2 = snprintf(tailoring_path, sizeof(tailoring_path), "../data/%s/ssg-%s-tailoring.xml", benchmark_id, benchmark_id);
    if (n1 < 0 || (size_t)n1 >= sizeof(ds_path) || n2 < 0 || (size_t)n2 >= sizeof(tailoring_path)) {
        return -1;
    }

    struct ds_sds_session *ds_sds_session = NULL;
    struct xccdf_benchmark *benchmark = NULL;
    if(load_benchmark_from_ds(ds_path, &ds_sds_session, &benchmark) != 0){
        return -1;
    }

    // 1. recherche dans les profils natifs
    struct xccdf_profile_iterator *pit = xccdf_benchmark_get_profiles(benchmark);
    struct xccdf_profile *profile = find_profile_by_id(pit, profile_id);
    if (pit != NULL) {
        xccdf_profile_iterator_free(pit);
    }

    // 2. si pas trouvé, recherche dans le tailoring (s'il existe)
    struct xccdf_tailoring *tailoring = NULL;
    struct oscap_source *tailoring_source = NULL;

    if (profile == NULL && access(tailoring_path, F_OK) == 0) {
        tailoring_source = oscap_source_new_from_file(tailoring_path);
        if (tailoring_source == NULL) {
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }

        tailoring = xccdf_tailoring_import_source(tailoring_source, benchmark);
        if (tailoring == NULL) {
            oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }

        struct xccdf_profile_iterator *tit = xccdf_tailoring_get_profiles(tailoring);
        profile = find_profile_by_id(tit, profile_id);
        if (tit != NULL) {
            xccdf_profile_iterator_free(tit);
        }
    }

    if (profile == NULL) {
        if (tailoring != NULL) xccdf_tailoring_free(tailoring);
        if (tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    // 3. policy_model prend possession du benchmark
    struct xccdf_policy_model *policy_model = xccdf_policy_model_new(benchmark);
    if (policy_model == NULL) {
        if (tailoring != NULL) xccdf_tailoring_free(tailoring);
        if (tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    // 4. si le profil vient du tailoring, l'attacher AVANT de créer la policy
    // (obligatoire pour que l'héritage extends soit résolu correctement,
    // confirmé empiriquement - cf. tests précédents)
    if (tailoring != NULL) {
        xccdf_policy_model_set_tailoring(policy_model, tailoring);
        // à partir d'ici, policy_model possède tailoring - ne plus le free séparément
    }

    ctx->ds_sds_session = ds_sds_session;
    ctx->benchmark = benchmark;
    ctx->policy_model = policy_model;
    ctx->tailoring_source = tailoring_source;
    ctx->profile = profile;
    return 0;
}

static void free_profile_context(struct resolved_profile_context *ctx) {
    if (ctx->policy_model != NULL) {
        xccdf_policy_model_free(ctx->policy_model); // libère benchmark + tailoring en interne
    }
    if (ctx->tailoring_source != NULL) {
        oscap_source_free(ctx->tailoring_source);
    }
    if (ctx->ds_sds_session != NULL) {
        ds_sds_session_free(ctx->ds_sds_session);
    }
}

int selected_rules_for_profile(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules){
    if(out_rules==NULL){
        return -1;
    }
    *out_rules=NULL;

    oscap_init();

    struct resolved_profile_context ctx;
    if (resolve_profile_context(benchmark_id, profile_id, &ctx) != 0) {
        oscap_cleanup();
        return -1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(ctx.policy_model,ctx.profile);
    if(policy==NULL){
        free_profile_context(&ctx);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_select_iterator *select_iterator=xccdf_policy_get_selected_rules(policy);
    if(select_iterator==NULL){
        xccdf_policy_free(policy);
        free_profile_context(&ctx);
        oscap_cleanup();
        return -1;
    }

    struct rule_list *rules=NULL;
    int count=0;

    while(xccdf_select_iterator_has_more(select_iterator)){
        struct xccdf_select *select=xccdf_select_iterator_next(select_iterator);

        struct xccdf_item *item=xccdf_benchmark_get_item(ctx.benchmark,xccdf_select_get_item(select));
        if(item==NULL || xccdf_item_get_type(item)!=XCCDF_RULE){
            free_rule_info_list(rules,count);
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            oscap_cleanup();
            return -1;
        }

        struct xccdf_rule *rule=xccdf_item_to_rule(item);
        const char *id=xccdf_rule_get_id(rule);
        const char *title=get_rule_title(rule);
        const char *description=get_rule_description(rule);
        const char *rationale=get_rule_rationale(rule);
        const char *severity=get_rule_severity(rule);

        struct rule_list *tmp = realloc(rules, (count + 1) * sizeof(struct rule_list));
        if (tmp == NULL) {
            free_rule_info_list(rules, count);
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            oscap_cleanup();
            return -1;
        }
        rules = tmp;

        rules[count].id = id ? strdup(id) : NULL;
        rules[count].title = title ? strdup(title) : NULL;
        rules[count].description = description ? strdup(description) : NULL;
        rules[count].rationale = rationale ? strdup(rationale) : NULL;
        rules[count].severity = severity ? strdup(severity) : NULL;
        rules[count].selected = true;

        if((id !=NULL && rules[count].id==NULL) || (title!=NULL && rules[count].title==NULL) || (description!=NULL && rules[count].description==NULL) || (rationale!=NULL && rules[count].rationale==NULL) || (severity!=NULL && rules[count].severity==NULL)){
            free(rules[count].id);
            free(rules[count].title);
            free(rules[count].description);
            free(rules[count].rationale);
            free(rules[count].severity);
            free_rule_info_list(rules,count);
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            oscap_cleanup();
            return -1;
        }
        count++;
    }

    xccdf_select_iterator_free(select_iterator);
    xccdf_policy_free(policy);
    free_profile_context(&ctx);
    oscap_cleanup();

    *out_rules=rules;
    return count;
}

int all_rules_with_selection_for_profile(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules){
    if(out_rules==NULL){
        return -1;
    }
    *out_rules=NULL;

    oscap_init();

    struct resolved_profile_context ctx;
    if (resolve_profile_context(benchmark_id, profile_id, &ctx) != 0) {
        oscap_cleanup();
        return -1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(ctx.policy_model,ctx.profile);
    if(policy==NULL){
        free_profile_context(&ctx);
        oscap_cleanup();
        return -1;
    }

    struct rule_node *head=get_benchmark_rules_or_null(ctx.benchmark);
    if(head==NULL){
        xccdf_policy_free(policy);
        free_profile_context(&ctx);
        oscap_cleanup();
        return -1;
    }

    struct rule_list *rules=NULL;
    int count=0;
    struct rule_node *iter=head;

    while(iter!=NULL){
        struct rule_list *tmp = realloc(rules, (count + 1) * sizeof(struct rule_list));
        if(tmp==NULL){
            free_rule_info_list(rules,count);
            free_rule_list(head);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            oscap_cleanup();
            return -1;
        }
        rules=tmp;

        const char *id=xccdf_rule_get_id(iter->rule);
        const char *title=get_rule_title(iter->rule);
        const char *description=get_rule_description(iter->rule);
        const char *rationale=get_rule_rationale(iter->rule);
        const char *severity=get_rule_severity(iter->rule);
        bool selected=xccdf_policy_is_item_selected(policy,id);

        rules[count].id = id ? strdup(id) : NULL;
        rules[count].title = title ? strdup(title) : NULL;
        rules[count].description = description ? strdup(description) : NULL;
        rules[count].rationale = rationale ? strdup(rationale) : NULL;
        rules[count].severity = severity ? strdup(severity) : NULL;
        rules[count].selected = selected;

        if((id !=NULL && rules[count].id==NULL) || (title!=NULL && rules[count].title==NULL) || (description!=NULL && rules[count].description==NULL) || (rationale!=NULL && rules[count].rationale==NULL) || (severity!=NULL && rules[count].severity==NULL)){
            free(rules[count].id);
            free(rules[count].title);
            free(rules[count].description);
            free(rules[count].rationale);
            free(rules[count].severity);
            free_rule_info_list(rules,count);
            free_rule_list(head);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            oscap_cleanup();
            return -1;
        }
        count++;
        iter=iter->next;
    }

    free_rule_list(head);
    xccdf_policy_free(policy);
    free_profile_context(&ctx);
    oscap_cleanup();

    *out_rules=rules;
    return count;
}