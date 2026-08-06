#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <ds_sds_session.h>
#include <oscap_source.h>


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


struct rule_node{
    struct xccdf_rule *rule;
    bool selected;
    struct rule_node *next;
};

void free_rule_list(struct rule_node *head){
    while(head!=NULL){
        struct rule_node *new_rule_node=head;
        head=head->next;
        free(new_rule_node);
    }
}

struct rule_node *push_front(struct rule_node *head,struct xccdf_rule *rule,bool selected,bool *error){
    struct rule_node *new_rule_node=malloc(sizeof(struct rule_node));
    if(new_rule_node==NULL){
        printf("Echec dans la création d'un noeud rule_node!!!!!\n");
        *error=true;
        return head;
    }
    new_rule_node->rule=rule;
    new_rule_node->next=head;
    new_rule_node->selected=selected;
    return new_rule_node;
}

struct rule_node *collect_rules_recursive(struct xccdf_item *benchmark_item,struct rule_node *head,struct xccdf_policy *policy,bool *error){
    if(*error){
        return head;
    }
    xccdf_type_t benchmark_item_type=xccdf_item_get_type(benchmark_item);
    if(benchmark_item_type==XCCDF_RULE){
        bool selected=xccdf_policy_is_item_selected(policy,xccdf_item_get_id(benchmark_item));
        head=push_front(head,xccdf_item_to_rule(benchmark_item),selected,error);
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
            head=collect_rules_recursive(group_item,head,policy,error);
        }
        xccdf_item_iterator_free(group_iterator);
    }
    return head;
}

struct rule_node *get_benchmark_rules(struct xccdf_benchmark *benchmark,struct xccdf_policy *policy,bool *error){
    struct rule_node *head=NULL;
    struct xccdf_item_iterator *benchmark_iterator=xccdf_benchmark_get_content(benchmark);
    if(benchmark_iterator==NULL){
        printf("Erreur dans la création de l'itérateur du benchmark!!!!\n");
        *error=true;
        return NULL;
    }
    while(xccdf_item_iterator_has_more(benchmark_iterator) && *error==false){
        struct xccdf_item *benchmark_item=xccdf_item_iterator_next(benchmark_iterator);
        head=collect_rules_recursive(benchmark_item,head,policy,error);
    }
    xccdf_item_iterator_free(benchmark_iterator);
    return head;
}

struct rule_node *get_benchmark_rules_or_null(struct xccdf_benchmark *benchmark,struct xccdf_policy *policy,bool *error){
    struct rule_node *head=get_benchmark_rules(benchmark,policy,error);
    if(error!=NULL && *error){
        free_rule_list(head);
        return NULL;
    }
    return head;
}


int main(int argc,char **argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de 2\n",argc-1);
        return 1;
    }

    oscap_init();

    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(argv[1]);
    if(oscap_ds_source==NULL){
        printf("Erreur dans l'initialisationde la source!!!!\n");
        oscap_cleanup();
        return 1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        printf("Erreur dans la conversion de la source vers une Data Stream Source!!!!\n");
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        printf("Erreur dans la création de la source XCCDF depuis la Data Stream source!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        printf("Erreur dans la création du benchmark!!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    bool error=false;

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur!!!!\n");
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile **profile_list=NULL;

    int count=0;

    while(xccdf_profile_iterator_has_more(profile_iterator)){

        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            printf("Erreur dans l'extraction du profil!!!!\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        struct xccdf_profile **tmp=realloc(profile_list, (count+1)* sizeof(struct xccdf_profile *));
        if(tmp==NULL){
            printf("Erreur allocation mémoire\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        profile_list=tmp;

        profile_list[count]=profile;
        
        printf("%d) ID : %s\nTitre : %s\n",count+1,xccdf_profile_get_id(profile_list[count]),get_profile_title(profile_list[count]));
        count++;
    }

    xccdf_profile_iterator_free(profile_iterator);

    int choice;
    printf("Choisissez un profil: ");
    scanf("%d",&choice);
    if(choice<1 || choice>count){
        printf("Choix invalide\n");
        free(profile_list);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    choice--;

    struct xccdf_profile *profile=profile_list[choice];
    if(profile==NULL){
        printf("Erreur dans la récupération du profil!!!\n");
        free(profile_list);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    free(profile_list);

    struct xccdf_policy_model *policy_model=xccdf_policy_model_new(benchmark);
    if(policy_model==NULL){
        printf("Erreur lors de la création du policy_model!!!\n");
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(policy_model,profile);
    if(policy==NULL){
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct rule_node *head=get_benchmark_rules_or_null(benchmark,policy,&error);

    if(error){
        printf("Erreur: échec lors de la collecte des règles (allocation mémoire ou itérateur invalide)\n");
        free_rule_list(head);
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct rule_node *iterator=head;

    while(iterator!=NULL){
        printf("Selected : %s\nTitre : %s\nID : %s\n",iterator->selected ? "true" : "false",get_rule_title(iterator->rule),xccdf_rule_get_id(iterator->rule));
        iterator=iterator->next;
    }

    free_rule_list(head);
    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    return 0;
}