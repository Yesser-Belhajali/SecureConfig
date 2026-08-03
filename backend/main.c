#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <ds_sds_session.h>
#include <oscap_source.h>


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
            return NULL;
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

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        printf("Erreur dans la création du benchmark!!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    bool error=false;

    struct rule_node *head=get_benchmark_rules(benchmark,&error);

    if(error){
        printf("Erreur: échec lors de la collecte des règles (allocation mémoire ou itérateur invalide)\n");
        free_rule_list(head);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct rule_node *iterator=head;

    while(iterator!=NULL){
        printf("Titre : %s\nID : %s\n",get_rule_title(iterator->rule),xccdf_rule_get_id(iterator->rule));
        iterator=iterator->next;
    }

    free_rule_list(head);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_source_free(oscap_ds_source);
    oscap_cleanup();
    return 0;
}









/*

int mon_callback_start(struct xccdf_rule *rule, void *usr);
int mon_callback_output(struct xccdf_rule_result *result, void *usr);




int main(int argc, char** argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de 2\n",argc-1);
        return 1;
    }
    oscap_init();

    struct xccdf_session* session=xccdf_session_new(argv[1]);

    if(session==NULL){
        printf("Echec dans l'initialisation de la session\n");
        oscap_cleanup();
        return 1;
    }

    if(xccdf_session_load(session)!=0){
        printf("Le chargement des composants a échoué!!!\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_policy_model *policy_model=xccdf_session_get_policy_model(session);
    if(policy_model==NULL){
        printf("Echech dans la récupération de la policy_model!!!!\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_benchmark *benchmark=xccdf_policy_model_get_benchmark(policy_model);
    if(benchmark==NULL){
        printf("Echec dans la récupération du benchmark!!!!\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    bool error=false;

    struct rule_node *all_rules=get_all_rules(benchmark,&error);

    if(error){
        fprintf(stderr, "Erreur: échec lors de la collecte des règles (allocation mémoire ou itérateur invalide)\n");
        free_rule_list(all_rules);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }


    for (struct rule_node *cur = all_rules; cur != NULL; cur = cur->next) {
        const char *id = xccdf_rule_get_id(cur->rule);
        if(id == NULL){
            fprintf(stderr, "Erreur: une règle sans ID a été rencontrée\n");
            free_rule_list(all_rules);
            xccdf_session_free(session);
            oscap_cleanup();
            return 1;
        }
        const char *title = get_first_title(cur->rule);
        printf("ID: %s\nTitre: %s\n\n", id, title ? title : "N/A");
    }

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur!!!!\n");
        xccdf_session_free(session);
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
            xccdf_session_free(session);
            free(profile_list);
            oscap_cleanup();
            return 1;
        }
        struct xccdf_profile **tmp=realloc(profile_list, (count+1)* sizeof(struct xccdf_profile *));

        if(tmp==NULL){
            printf("Erreur allocation mémoire\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            free(tmp);
            xccdf_session_free(session);
            oscap_cleanup();
            return 1;
        }

        profile_list=tmp;

        profile_list[count]=profile;
        if(profile_list[count]==NULL){
            printf("Erreur allocation mémoire\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            free(tmp);
            xccdf_session_free(session);
            oscap_cleanup();
            return 1;
        }
        printf("%d)%s\n",count+1,xccdf_profile_get_id(profile_list[count]));
        count++;
    }

    xccdf_profile_iterator_free(profile_iterator);

    int choice;
    printf("Choisissez un profil: ");
    scanf("%d",&choice);
    if(choice<1 || choice>count){
        printf("Choix invalide\n");
        free(profile_list);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    choice--;

    printf("Vous avez choisi : %s\n",xccdf_profile_get_id(profile_list[choice]));

    if(!xccdf_session_set_profile_id(session,xccdf_profile_get_id(profile_list[choice]))){
        printf("Erreur dans l'initialisation du profil de la session\n");
        free(profile_list);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_policy *policy=xccdf_session_get_xccdf_policy(session);

    xccdf_policy_model_register_start_callback(policy_model, mon_callback_start, policy);
    xccdf_policy_model_register_output_callback(policy_model, mon_callback_output, NULL);
    
    
    if (xccdf_session_evaluate(session) != 0) {
        printf("Échec de l'évaluation\n");
        free(profile_list);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    
    printf("Evaluation terminée avec succes!!!!!!YYAAAAYYY\n");

    printf("Votre score de conformité est = %f%%\n",xccdf_session_get_base_score(session));




    
    free_rule_list(all_rules);
    xccdf_session_free(session);
    oscap_cleanup();
    return 0;
}



int mon_callback_start(struct xccdf_rule *rule, void *usr) {

    struct xccdf_policy *policy =(struct xccdf_policy *)usr;

    const char *rule_id =xccdf_rule_get_id(rule);

    const bool rule_selected=xccdf_policy_is_item_selected(policy,rule_id);

    if(!rule_selected){
        return 0;
    }

    char *rule_title =xccdf_policy_get_readable_item_title(policy,(struct xccdf_item *)rule,NULL);
    
    printf("Title : %s\n", rule_title);
    printf("Rule : %s\n", rule_id);
    free(rule_title);
    return 0;
}

int mon_callback_output(struct xccdf_rule_result *rule_result, void *usr) {

    xccdf_test_result_type_t result_type = xccdf_rule_result_get_result(rule_result);

    if(result_type==XCCDF_RESULT_NOT_SELECTED){
        return 0;
    }

    const char *rule_result_type="UNKNOWN";

    switch(result_type){
        case XCCDF_RESULT_PASS:
            rule_result_type = "PASS";
            break;

        case XCCDF_RESULT_FAIL:
            rule_result_type = "FAIL";
            break;

        case XCCDF_RESULT_ERROR:
            rule_result_type = "ERROR";
            break;

        case XCCDF_RESULT_UNKNOWN:
            rule_result_type = "UNKNOWN";
            break;

        case XCCDF_RESULT_NOT_APPLICABLE:
            rule_result_type = "NOT_APPLICABLE";
            break;

        case XCCDF_RESULT_NOT_CHECKED:
            rule_result_type = "NOT_CHECKED";
            break;

        case XCCDF_RESULT_NOT_SELECTED:
            rule_result_type = "NOT_SELECTED";
            break;

        case XCCDF_RESULT_INFORMATIONAL:
            rule_result_type = "INFORMATIONAL";
            break;

        case XCCDF_RESULT_FIXED:
            rule_result_type = "FIXED";
            break;
    }



    const char *rule_result_time=xccdf_rule_result_get_time(rule_result);

    const char *rule_result_severity="Not Defined";

    xccdf_level_t severity_type=xccdf_rule_result_get_severity(rule_result);

    switch (severity_type) {
        case XCCDF_LEVEL_NOT_DEFINED:
            rule_result_severity = "Not Defined";
            break;

        case XCCDF_UNKNOWN:
            rule_result_severity = "Unknown";
            break;

        case XCCDF_INFO:
            rule_result_severity = "Info";
            break;

        case XCCDF_LOW:
            rule_result_severity = "Low";
            break;

        case XCCDF_MEDIUM:
            rule_result_severity = "Medium";
            break;

        case XCCDF_HIGH:
            rule_result_severity = "High";
            break;
    }

    printf("Time : %s\nSeverity : %s\nStatus :  %s\n\n",rule_result_time,rule_result_severity ,rule_result_type);
    return 0;
}*/