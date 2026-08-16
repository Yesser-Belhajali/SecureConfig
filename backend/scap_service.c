#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
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

const char *get_rule_question(struct xccdf_rule *rule){
    struct oscap_text_iterator *question_it=xccdf_rule_get_question(rule);
    const char *question=NULL;
    if(question_it!=NULL && oscap_text_iterator_has_more(question_it)){
        struct oscap_text *text=oscap_text_iterator_next(question_it);
        question=oscap_text_get_text(text);
    }
    if(question_it!=NULL){
        oscap_text_iterator_free(question_it);
    }
    return question;
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




void free_profiles_for_distro(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count){
    free_profile_list(profiles, profiles_count);
    free_profile_list(tailoring_profiles, tailoring_count);
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


    struct ds_sds_session *ds_sds_session = NULL;
    struct xccdf_benchmark *benchmark = NULL;
    if(load_benchmark_from_ds(ds_path, &ds_sds_session, &benchmark) != 0){
        return -1;
    }

    struct xccdf_profile_iterator *profile_iterator = xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator == NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    struct profile_list *profiles = NULL;
    int profiles_count = profiles_from_iterator(profile_iterator, &profiles);
    xccdf_profile_iterator_free(profile_iterator);

    if(profiles_count < 0){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
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
            return -1;
        }

        struct xccdf_tailoring *tailoring = xccdf_tailoring_import_source(oscap_tailoring_source, benchmark);
        if(tailoring == NULL){
            oscap_source_free(oscap_tailoring_source);
            free_profile_list(profiles, profiles_count);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }

        struct xccdf_profile_iterator *tailoring_iterator = xccdf_tailoring_get_profiles(tailoring);
        if(tailoring_iterator == NULL){
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
            free_profile_list(profiles, profiles_count);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
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
            return -1;
        }
    }

    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);

    *out_profiles = profiles;
    *out_profiles_count = profiles_count;
    *out_tailoring_profiles = tailoring_profiles;
    *out_tailoring_count = tailoring_count;

    return 0;
}


static void free_references(struct rule_reference *refs, int count){
    if(refs==NULL) return;
    for(int i=0;i<count;i++){
        free(refs[i].href);
        free(refs[i].text);
    }
    free(refs);
}

static int collect_references(struct xccdf_rule *rule, struct rule_reference **out_refs, int *out_count){
    *out_refs=NULL;
    *out_count=0;

    struct oscap_reference_iterator *it=xccdf_rule_get_references(rule);
    if(it==NULL){
        return 0;
    }

    struct rule_reference *refs=NULL;
    int count=0;

    while(oscap_reference_iterator_has_more(it)){
        struct oscap_reference *ref=oscap_reference_iterator_next(it);
        const char *href=oscap_reference_get_href(ref);
        // pour une référence non-dublincore, le contenu texte brut est stocké
        // dans "title" côté openscap (confirmé dans reference.c) — c'est bien
        // le texte affiché entre les balises <xccdf:reference>...</xccdf:reference>
        const char *text=oscap_reference_get_title(ref);

        struct rule_reference *tmp=realloc(refs,(count+1)*sizeof(struct rule_reference));
        if(tmp==NULL){
            free_references(refs,count);
            oscap_reference_iterator_free(it);
            return -1;
        }
        refs=tmp;

        refs[count].href = href ? strdup(href) : NULL;
        refs[count].text = text ? strdup(text) : NULL;

        if((href!=NULL && refs[count].href==NULL) || (text!=NULL && refs[count].text==NULL)){
            free(refs[count].href);
            free(refs[count].text);
            free_references(refs,count);
            oscap_reference_iterator_free(it);
            return -1;
        }
        count++;
    }

    oscap_reference_iterator_free(it);
    *out_refs=refs;
    *out_count=count;
    return 0;
}

static void free_fixes(struct rule_fix *fixes, int count){
    if(fixes==NULL) return;
    for(int i=0;i<count;i++){
        free(fixes[i].system);
        free(fixes[i].content);
    }
    free(fixes);
}

static int collect_fixes(struct xccdf_rule *rule, struct rule_fix **out_fixes, int *out_count){
    *out_fixes=NULL;
    *out_count=0;

    struct xccdf_fix_iterator *it=xccdf_rule_get_fixes(rule);
    if(it==NULL){
        return 0;
    }

    struct rule_fix *fixes=NULL;
    int count=0;

    while(xccdf_fix_iterator_has_more(it)){
        struct xccdf_fix *fix=xccdf_fix_iterator_next(it);
        const char *system=xccdf_fix_get_system(fix);
        const char *content=xccdf_fix_get_content(fix); // const char* direct, pas d'oscap_text ici

        struct rule_fix *tmp=realloc(fixes,(count+1)*sizeof(struct rule_fix));
        if(tmp==NULL){
            free_fixes(fixes,count);
            xccdf_fix_iterator_free(it);
            return -1;
        }
        fixes=tmp;

        fixes[count].system = system ? strdup(system) : NULL;
        fixes[count].content = content ? strdup(content) : NULL;

        if((system!=NULL && fixes[count].system==NULL) || (content!=NULL && fixes[count].content==NULL)){
            free(fixes[count].system);
            free(fixes[count].content);
            free_fixes(fixes,count);
            xccdf_fix_iterator_free(it);
            return -1;
        }
        count++;
    }

    xccdf_fix_iterator_free(it);
    *out_fixes=fixes;
    *out_count=count;
    return 0;
}

static void free_warnings(struct rule_warning *warnings, int count) {
    if (warnings == NULL) return;
    for (int i = 0; i < count; i++) {
        free(warnings[i].category);
        free(warnings[i].text);
    }
    free(warnings);
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

static int collect_warnings(struct xccdf_rule *rule, struct rule_warning **out_warnings, int *out_count){
    *out_warnings=NULL;
    *out_count=0;

    struct xccdf_warning_iterator *it=xccdf_rule_get_warnings(rule);
    if(it==NULL){
        return 0;
    }

    struct rule_warning *warnings=NULL;
    int count=0;

    while(xccdf_warning_iterator_has_more(it)){
        struct xccdf_warning *w=xccdf_warning_iterator_next(it);
        const char *category=warning_category_to_str(xccdf_warning_get_category(w));

        struct oscap_text *text_obj=xccdf_warning_get_text(w);
        const char *text=text_obj ? oscap_text_get_text(text_obj) : NULL;

        struct rule_warning *tmp=realloc(warnings,(count+1)*sizeof(struct rule_warning));
        if(tmp==NULL){
            free_warnings(warnings,count);
            xccdf_warning_iterator_free(it);
            return -1;
        }
        warnings=tmp;

        warnings[count].category = category ? strdup(category) : NULL;
        warnings[count].text = text ? strdup(text) : NULL;

        if((category!=NULL && warnings[count].category==NULL) || (text!=NULL && warnings[count].text==NULL)){
            free(warnings[count].category);
            free(warnings[count].text);
            free_warnings(warnings,count);
            xccdf_warning_iterator_free(it);
            return -1;
        }
        count++;
    }

    xccdf_warning_iterator_free(it);
    *out_warnings=warnings;
    *out_count=count;
    return 0;
}

static void free_platforms(char **platforms, int count) {
    if (platforms == NULL) return;
    for (int i = 0; i < count; i++) free(platforms[i]);
    free(platforms);
}

static int collect_platforms(struct xccdf_rule *rule, char ***out_platforms, int *out_count){
    *out_platforms=NULL;
    *out_count=0;

    struct oscap_string_iterator *it=xccdf_rule_get_platforms(rule);
    if(it==NULL){
        return 0;
    }

    char **platforms=NULL;
    int count=0;

    while(oscap_string_iterator_has_more(it)){
        const char *platform=oscap_string_iterator_next(it);

        char **tmp=realloc(platforms,(count+1)*sizeof(char *));
        if(tmp==NULL){
            free_platforms(platforms,count);
            oscap_string_iterator_free(it);
            return -1;
        }
        platforms=tmp;

        platforms[count] = platform ? strdup(platform) : NULL;
        if(platform!=NULL && platforms[count]==NULL){
            free_platforms(platforms,count);
            oscap_string_iterator_free(it);
            return -1;
        }
        count++;
    }

    oscap_string_iterator_free(it);
    *out_platforms=platforms;
    *out_count=count;
    return 0;
}

static void free_checks(struct rule_check *checks, int count) {
    if (checks == NULL) return;
    for (int i = 0; i < count; i++) {
        free(checks[i].system);
        free(checks[i].selector);
        free(checks[i].content);
    }
    free(checks);
}

static int collect_checks(struct xccdf_rule *rule, struct rule_check **out_checks, int *out_count){
    *out_checks=NULL;
    *out_count=0;

    struct xccdf_check_iterator *it=xccdf_rule_get_checks(rule);
    if(it==NULL){
        return 0;
    }

    struct rule_check *checks=NULL;
    int count=0;

    while(xccdf_check_iterator_has_more(it)){
        struct xccdf_check *check=xccdf_check_iterator_next(it);
        const char *system=xccdf_check_get_system(check);
        const char *selector=xccdf_check_get_selector(check);
        const char *content=xccdf_check_get_content(check);

        struct rule_check *tmp=realloc(checks,(count+1)*sizeof(struct rule_check));
        if(tmp==NULL){
            free_checks(checks,count);
            xccdf_check_iterator_free(it);
            return -1;
        }
        checks=tmp;

        checks[count].system = system ? strdup(system) : NULL;
        checks[count].selector = selector ? strdup(selector) : NULL;
        checks[count].content = content ? strdup(content) : NULL;

        if((system!=NULL && checks[count].system==NULL) || (selector!=NULL && checks[count].selector==NULL) || (content!=NULL && checks[count].content==NULL)){
            free(checks[count].system);
            free(checks[count].selector);
            free(checks[count].content);
            free_checks(checks,count);
            xccdf_check_iterator_free(it);
            return -1;
        }
        count++;
    }

    xccdf_check_iterator_free(it);
    *out_checks=checks;
    *out_count=count;
    return 0;
}

// libère tous les champs d'UNE entrée rule_list (mais pas le pointeur lui-même,
// qui vit dans un tableau géré par realloc côté appelant)
static void free_rule_entry_fields(struct rule_list *r){
    free(r->id);
    free(r->title);
    free(r->description);
    free(r->rationale);
    free(r->severity);
    free(r->question);
    free_references(r->references, r->references_count);
    free_fixes(r->fixes, r->fixes_count);
    free_warnings(r->warnings, r->warnings_count);
    free_platforms(r->platforms, r->platforms_count);
    free_checks(r->checks, r->checks_count);
}



void free_rule_info_list(struct rule_list *rules,int count){
    if(rules==NULL){
        return;
    }
    for(int i=0;i<count;i++){
        free_rule_entry_fields(&rules[i]);
    }
    free(rules);
}


struct rule_node *push_front(struct rule_node *head,struct xccdf_rule *rule,bool *error){
    struct rule_node *new_rule_node=malloc(sizeof(struct rule_node));
    if(new_rule_node==NULL){
        *error=true;
        return head;
    }
    new_rule_node->rule=rule;
    new_rule_node->next=head;
    return new_rule_node;
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

void free_rule_list(struct rule_node *head){
    while(head!=NULL){
        struct rule_node *new_rule_node=head;
        head=head->next;
        free(new_rule_node);
    }
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

// remplit une entrée rule_list à partir d'une xccdf_rule; sur échec, *out est
// nettoyé et remis à zéro par free_rule_entry_fields, donc l'appelant peut
// simplement traiter l'entrée comme jamais remplie et ne PAS incrémenter count
static int fill_rule_entry(struct xccdf_rule *rule, bool selected, struct rule_list *out){
    memset(out, 0, sizeof(*out));

    const char *id=xccdf_rule_get_id(rule);
    const char *title=get_rule_title(rule);
    const char *description=get_rule_description(rule);
    const char *rationale=get_rule_rationale(rule);
    const char *severity=get_rule_severity(rule);
    const char *question=get_rule_question(rule);

    out->id = id ? strdup(id) : NULL;
    out->title = title ? strdup(title) : NULL;
    out->description = description ? strdup(description) : NULL;
    out->rationale = rationale ? strdup(rationale) : NULL;
    out->severity = severity ? strdup(severity) : NULL;
    out->question = question ? strdup(question) : NULL;
    out->selected = selected;

    if((id!=NULL && out->id==NULL) || (title!=NULL && out->title==NULL) ||
       (description!=NULL && out->description==NULL) || (rationale!=NULL && out->rationale==NULL) ||
       (severity!=NULL && out->severity==NULL) || (question!=NULL && out->question==NULL)){
        free_rule_entry_fields(out);
        return -1;
    }

    if(collect_references(rule, &out->references, &out->references_count) != 0){
        free_rule_entry_fields(out);
        return -1;
    }

    if(collect_fixes(rule, &out->fixes, &out->fixes_count) != 0){
        free_rule_entry_fields(out);
        return -1;
    }

    if(collect_warnings(rule, &out->warnings, &out->warnings_count) != 0){
        free_rule_entry_fields(out);
        return -1;
    }

    if(collect_platforms(rule, &out->platforms, &out->platforms_count) != 0){
        free_rule_entry_fields(out);
        return -1;
    }

    if(collect_checks(rule, &out->checks, &out->checks_count) != 0){
        free_rule_entry_fields(out);
        return -1;
    }

    return 0;
}


int list_rules_for_ds(const char *ds_path,struct rule_list **out_rules){
    if(ds_path==NULL){
        return -1;
    }
    *out_rules=NULL;


    struct ds_sds_session *ds_sds_session = NULL;
    struct xccdf_benchmark *benchmark = NULL;
    if(load_benchmark_from_ds(ds_path, &ds_sds_session, &benchmark) != 0){
        return -1;
    }

    struct rule_node *head=get_benchmark_rules_or_null(benchmark);
    if(head==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
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
            return -1;
        }
        rules=tmp;

        if(fill_rule_entry(iter->rule, false, &rules[count]) != 0){
            free_rule_info_list(rules,count);
            free_rule_list(head);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
        count++;
        iter=iter->next;
    }

    free_rule_list(head);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);

    *out_rules=rules;
    return count;
}

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
    struct xccdf_profile *profile = xccdf_benchmark_get_profile_by_id(benchmark, profile_id);

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

        profile = xccdf_tailoring_get_profile_by_id(tailoring, profile_id);
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

int profile_selected_rules(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules){
    if(out_rules==NULL){
        return -1;
    }
    *out_rules=NULL;


    struct resolved_profile_context ctx;
    if (resolve_profile_context(benchmark_id, profile_id, &ctx) != 0) {
        return -1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(ctx.policy_model,ctx.profile);
    if(policy==NULL){
        free_profile_context(&ctx);
        return -1;
    }

    struct xccdf_select_iterator *select_iterator=xccdf_policy_get_selected_rules(policy);
    if(select_iterator==NULL){
        xccdf_policy_free(policy);
        free_profile_context(&ctx);
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
            return -1;
        }

        struct xccdf_rule *rule=xccdf_item_to_rule(item);

        struct rule_list *tmp = realloc(rules, (count + 1) * sizeof(struct rule_list));
        if (tmp == NULL) {
            free_rule_info_list(rules, count);
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            return -1;
        }
        rules = tmp;

        if(fill_rule_entry(rule, true, &rules[count]) != 0){
            free_rule_info_list(rules, count);
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            free_profile_context(&ctx);
            return -1;
        }
        count++;
    }

    xccdf_select_iterator_free(select_iterator);
    xccdf_policy_free(policy);
    free_profile_context(&ctx);

    *out_rules=rules;
    return count;
}

int profile_all_rules(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules){
    if(out_rules==NULL){
        return -1;
    }
    *out_rules=NULL;


    struct resolved_profile_context ctx;
    if (resolve_profile_context(benchmark_id, profile_id, &ctx) != 0) {
        return -1;
    }

    struct rule_node *head=get_benchmark_rules_or_null(ctx.benchmark);
    if(head==NULL){
        free_profile_context(&ctx);
        return -1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(ctx.policy_model,ctx.profile);
    if(policy==NULL){
        free_profile_context(&ctx);
        free_rule_list(head);
        return -1;
    }

    

    struct rule_list *rules=NULL;
    int count=0;
    struct rule_node *iter=head;

    while(iter!=NULL){
        struct rule_list *tmp = realloc(rules, (count + 1) * sizeof(struct rule_list));
        if(tmp==NULL){
            free_rule_info_list(rules,count);
            xccdf_policy_free(policy);
            free_rule_list(head);
            free_profile_context(&ctx);
            return -1;
        }
        rules=tmp;

        const char *id=xccdf_rule_get_id(iter->rule);
        bool selected=xccdf_policy_is_item_selected(policy,id);

        if(fill_rule_entry(iter->rule, selected, &rules[count]) != 0){
            free_rule_info_list(rules,count);
            xccdf_policy_free(policy);
            free_rule_list(head);
            free_profile_context(&ctx);
            return -1;
        }
        count++;
        iter=iter->next;
    }

    xccdf_policy_free(policy);
    free_rule_list(head);
    free_profile_context(&ctx);

    *out_rules=rules;
    return count;
}


static void slugify(const char *name, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; name[i] != '\0' && j + 1 < out_size; i++) {
        char c = name[i];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out[j++] = c;
        } else if (c >= 'A' && c <= 'Z') {
            out[j++] = c - 'A' + 'a';
        } else if (j > 0 && out[j-1] != '_') {
            out[j++] = '_';
        }
    }
    while (j > 0 && out[j-1] == '_') j--;
    out[j] = '\0';
}

static bool id_exists(const char *id, struct profile_list *profiles, int count) {
    for (int i = 0; i < count; i++) {
        if (profiles[i].id != NULL && strcmp(profiles[i].id, id) == 0) return true;
    }
    return false;
}

static bool title_exists(const char *title, struct profile_list *profiles, int count) {
    for (int i = 0; i < count; i++) {
        if (profiles[i].title != NULL && strcmp(profiles[i].title, title) == 0) return true;
    }
    return false;
}

static int generate_unique_id(const char *name, struct profile_list *native, int native_count,
                               struct profile_list *tailoring, int tailoring_count,
                               char *out_id, size_t out_size){
    char slug[192];
    slugify(name, slug, sizeof(slug));
    if (slug[0] == '\0') {
        strcpy(slug, "profile");
    }

    for (int suffix = 0; suffix < 1000; suffix++) {
        int n = (suffix == 0)
            ? snprintf(out_id, out_size, "xccdf_org.secureconfig_profile_%s", slug)
            : snprintf(out_id, out_size, "xccdf_org.secureconfig_profile_%s_%d", slug, suffix + 1);
        if (n < 0 || (size_t)n >= out_size) return -1;

        if (!id_exists(out_id, native, native_count) && !id_exists(out_id, tailoring, tailoring_count)) {
            return 0;
        }
    }
    return -1;
}


int create_tailoring_profile(const char *benchmark_id, const char *name, const char *description,
                              const char *base_profile_id, // NULL = from-scratch
                              const char **added_ids, int added_count,
                              const char **removed_ids, int removed_count,
                              char *out_new_id, size_t out_id_size){

    if(benchmark_id==NULL || name==NULL || out_new_id==NULL){
        return -1;
    }

    // 1. unicité du nom + génération de l'id
    struct profile_list *native_profiles=NULL;
    int native_count=0;
    struct profile_list *tailoring_profiles_list=NULL;
    int tailoring_list_count=0;

    if(list_profiles_for_distro(benchmark_id,&native_profiles,&native_count,&tailoring_profiles_list,&tailoring_list_count)!=0){
        return -1;
    }

    if(title_exists(name, native_profiles, native_count) || title_exists(name, tailoring_profiles_list, tailoring_list_count)){
        free_profiles_for_distro(native_profiles, native_count, tailoring_profiles_list, tailoring_list_count);
        return -2;
    }

    char new_id[256];
    if(generate_unique_id(name, native_profiles, native_count, tailoring_profiles_list, tailoring_list_count, new_id, sizeof(new_id))!=0){
        free_profiles_for_distro(native_profiles, native_count, tailoring_profiles_list, tailoring_list_count);
        return -1;
    }

    free_profiles_for_distro(native_profiles, native_count, tailoring_profiles_list, tailoring_list_count);

    // 2. chemins + chargement du DS
    char ds_path[256], tailoring_path[256];
    int n1=snprintf(ds_path,sizeof(ds_path),"../data/%s/ssg-%s-ds.xml",benchmark_id,benchmark_id);
    int n2=snprintf(tailoring_path,sizeof(tailoring_path),"../data/%s/ssg-%s-tailoring.xml",benchmark_id,benchmark_id);
    if(n1<0 || (size_t)n1>=sizeof(ds_path) || n2<0 || (size_t)n2>=sizeof(tailoring_path)){
        return -1;
    }


    struct ds_sds_session *ds_sds_session=NULL;
    struct xccdf_benchmark *benchmark=NULL;
    if(load_benchmark_from_ds(ds_path,&ds_sds_session,&benchmark)!=0){
        return -1;
    }

    // 3. charge le tailoring existant, ou en crée un nouveau
    bool tailoring_existed = (access(tailoring_path, F_OK) == 0);
    struct oscap_source *tailoring_source = NULL;
    struct xccdf_tailoring *tailoring = NULL;

    if(tailoring_existed){
        tailoring_source = oscap_source_new_from_file(tailoring_path);
        if(tailoring_source == NULL){
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
        tailoring = xccdf_tailoring_import_source(tailoring_source, benchmark);
        if(tailoring == NULL){
            oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
    } 
    else {
        tailoring = xccdf_tailoring_new();
        if(tailoring == NULL){
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
        const char *bench_id = xccdf_benchmark_get_id(benchmark);

        char tailoring_doc_id[300];
        snprintf(tailoring_doc_id, sizeof(tailoring_doc_id), "xccdf_org.secureconfig_tailoring_%s", benchmark_id);

        // format ISO 8601, cohérent avec xccdf_rule_result_get_time() vu dans les tests de scan
        char version_time[32];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(version_time, sizeof(version_time), "%Y-%m-%dT%H:%M:%S", tm_info);

        if(!xccdf_tailoring_set_id(tailoring, tailoring_doc_id)
        || !xccdf_tailoring_set_benchmark_ref(tailoring, bench_id)
        || !xccdf_tailoring_set_version(tailoring, "1")
        || !xccdf_tailoring_set_version_time(tailoring, version_time)){
            xccdf_tailoring_free(tailoring);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
    }

    // 4. valide le profil de base, s'il est fourni (natif ou déjà dans le tailoring)
    if(base_profile_id != NULL){
        struct xccdf_profile *base_profile = xccdf_benchmark_get_profile_by_id(benchmark, base_profile_id);

        if(base_profile == NULL && tailoring_existed){
            base_profile = xccdf_tailoring_get_profile_by_id(tailoring, base_profile_id);
        }

        if(base_profile == NULL){
            xccdf_tailoring_free(tailoring);
            if(tailoring_source != NULL) oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -3;
        }
    }

    // 5. crée le profil
    struct xccdf_profile *profile = xccdf_profile_new();
    if(profile == NULL){
        xccdf_tailoring_free(tailoring);
        if(tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    if(!xccdf_profile_set_id(profile, new_id) || !xccdf_profile_set_tailoring(profile, true)){
        xccdf_profile_free(xccdf_profile_to_item(profile));
        xccdf_tailoring_free(tailoring);
        if(tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    if(base_profile_id != NULL && !xccdf_profile_set_extends(profile, base_profile_id)){
        xccdf_profile_free(xccdf_profile_to_item(profile));
        xccdf_tailoring_free(tailoring);
        if(tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    struct oscap_text *title = oscap_text_new();
    bool title_ok = (title != NULL) && oscap_text_set_text(title, name) && oscap_text_set_lang(title, "fr") && xccdf_profile_add_title(profile, title);
    if(!title_ok){
        if(title != NULL) oscap_text_free(title);
        xccdf_profile_free(xccdf_profile_to_item(profile));
        xccdf_tailoring_free(tailoring);
        if(tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }

    // 5bis. description, optionnelle : même pattern que le titre, mais on ne
    // fait rien si description est NULL ou vide (pas d'élément <description>
    // ajouté au profil dans ce cas)
    if(description != NULL && description[0] != '\0'){
        struct oscap_text *desc = oscap_text_new();
        bool desc_ok = (desc != NULL) && oscap_text_set_text(desc, description) && oscap_text_set_lang(desc, "fr") && xccdf_profile_add_description(profile, desc);
        if(!desc_ok){
            if(desc != NULL) oscap_text_free(desc);
            xccdf_profile_free(xccdf_profile_to_item(profile));
            xccdf_tailoring_free(tailoring);
            if(tailoring_source != NULL) oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
    }

    // 6. selects : added -> selected=true, removed -> selected=false
    for(int i=0; i<added_count; i++){
        struct xccdf_select *sel = xccdf_select_new();
        bool ok = (sel != NULL) && xccdf_select_set_item(sel, added_ids[i]) && xccdf_select_set_selected(sel, true) && xccdf_profile_add_select(profile, sel);
        if(!ok){
            if(sel != NULL) xccdf_select_free(sel);
            xccdf_profile_free(xccdf_profile_to_item(profile));
            xccdf_tailoring_free(tailoring);
            if(tailoring_source != NULL) oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
    }

    for(int i=0; i<removed_count; i++){
        struct xccdf_select *sel = xccdf_select_new();
        bool ok = (sel != NULL) && xccdf_select_set_item(sel, removed_ids[i]) && xccdf_select_set_selected(sel, false) && xccdf_profile_add_select(profile, sel);
        if(!ok){
            if(sel != NULL) xccdf_select_free(sel);
            xccdf_profile_free(xccdf_profile_to_item(profile));
            xccdf_tailoring_free(tailoring);
            if(tailoring_source != NULL) oscap_source_free(tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            return -1;
        }
    }

    // 7. attache le profil au tailoring et exporte
    if(!xccdf_tailoring_add_profile(tailoring, profile)){
        xccdf_profile_free(xccdf_profile_to_item(profile));
        xccdf_tailoring_free(tailoring);
        if(tailoring_source != NULL) oscap_source_free(tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        return -1;
    }
    // à partir d'ici, tailoring possède profile - ne plus le free séparément

    const struct xccdf_version_info *version_info = xccdf_benchmark_get_schema_version(benchmark);
    int export_ret = xccdf_tailoring_export(tailoring, tailoring_path, version_info);

    xccdf_tailoring_free(tailoring); // libère aussi le profil qu'il possède désormais
    if(tailoring_source != NULL) oscap_source_free(tailoring_source);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);

    if(export_ret < 0){
        return -1;
    }

    if(out_id_size > 0){
        strncpy(out_new_id, new_id, out_id_size - 1);
        out_new_id[out_id_size - 1] = '\0';
    }

    return 0;
}