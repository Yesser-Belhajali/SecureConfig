#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    }
    free(profiles);
}

int list_profiles_for_ds(const char *ds_path,struct profile_list **out_profiles){

    if(out_profiles==NULL){
        return -1;
    }

    *out_profiles=NULL;

    oscap_init();

    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(ds_path);
    if(oscap_ds_source==NULL){
        oscap_cleanup();
        return -1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct profile_list *profiles=NULL;

    int count=0;

    while(xccdf_profile_iterator_has_more(profile_iterator)){

        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            free_profile_list(profiles,count);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        struct profile_list *tmp=realloc(profiles, (count+1)* sizeof(struct profile_list));
        if(tmp==NULL){
            free_profile_list(profiles,count);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        profiles=tmp;

        profiles[count].id = NULL;
        profiles[count].title = NULL;

        const char *id=xccdf_profile_get_id(profile);
        const char *title=get_profile_title(profile);

        profiles[count].id = id ? strdup(id) : NULL;
        profiles[count].title = title ? strdup(title) : NULL;

        if((id!=NULL && profiles[count].id==NULL) || (title!=NULL && profiles[count].title==NULL)){
            free(profiles[count].id);
            free(profiles[count].title);
            free_profile_list(profiles,count);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
        count++;
    }


    xccdf_profile_iterator_free(profile_iterator);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();

    *out_profiles=profiles;
    return count;
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

    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(ds_path);
    if(oscap_ds_source==NULL){
        oscap_cleanup();
        return -1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        ds_sds_session_free(ds_sds_session);
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



int selected_rules_for_profile(const char *ds_path,const char *profile_id,struct rule_list **out_rules){
    if(out_rules==NULL){
        return -1;
    }

    *out_rules=NULL;

    oscap_init();


    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(ds_path);
    if(oscap_ds_source==NULL){
        oscap_cleanup();
        return -1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }
    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }
    
    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_profile *profile = NULL;
    while (xccdf_profile_iterator_has_more(profile_iterator)) {
        struct xccdf_profile *p = xccdf_profile_iterator_next(profile_iterator);
        if (strcmp(xccdf_profile_get_id(p), profile_id) == 0) {
            profile = p;
            break;
        }
    }

    xccdf_profile_iterator_free(profile_iterator);

    if (profile == NULL) {
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_policy_model *policy_model=xccdf_policy_model_new(benchmark);
    if(policy_model==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(policy_model,profile);
    if(policy==NULL){
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_select_iterator *select_iterator=xccdf_policy_get_selected_rules(policy);
    if(select_iterator==NULL){
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct rule_list *rules=NULL;
    int count=0;

    while(xccdf_select_iterator_has_more(select_iterator)){
        struct xccdf_select *select=xccdf_select_iterator_next(select_iterator);

        struct xccdf_item *item=xccdf_benchmark_get_item(benchmark,xccdf_select_get_item(select));
        if(item==NULL || xccdf_item_get_type(item)!=XCCDF_RULE){
            free_rule_info_list(rules,count);
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
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
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
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
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
        count++;
    }

    xccdf_select_iterator_free(select_iterator);
    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    
    *out_rules=rules;
    return count;
}

int all_rules_with_selection_for_profile(const char *ds_path,const char *profile_id,struct rule_list **out_rules){
    if(out_rules==NULL){
        return -1;
    }

    *out_rules=NULL;

    oscap_init();

    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(ds_path);
    if(oscap_ds_source==NULL){
        oscap_cleanup();
        return -1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_profile *profile = NULL;
    while (xccdf_profile_iterator_has_more(profile_iterator)) {
        struct xccdf_profile *p = xccdf_profile_iterator_next(profile_iterator);
        if (strcmp(xccdf_profile_get_id(p), profile_id) == 0) {
            profile = p;
            break;
        }
    }

    xccdf_profile_iterator_free(profile_iterator);

    if (profile == NULL) {
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_policy_model *policy_model=xccdf_policy_model_new(benchmark);
    if(policy_model==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(policy_model,profile);
    if(policy==NULL){
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct rule_node *head=get_benchmark_rules_or_null(benchmark);
    if(head==NULL){
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
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
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
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
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
        count++;
        iter=iter->next;
    }

    free_rule_list(head);
    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();

    *out_rules=rules;
    return count;
}