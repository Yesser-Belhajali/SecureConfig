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
    
    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur du benchmark!!!!\n");
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile **profile_list=NULL;

    int count=0;

    while(xccdf_profile_iterator_has_more(profile_iterator)){
        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            printf("Erreur lors de l'extraction du profil!!!!\n");
            free(profile_list);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }

        struct xccdf_profile **tmp=realloc(profile_list, (count+1)* sizeof(struct xccdf_profile *));
        if(tmp==NULL){
            printf("Erreur allocation mémoire\n");
            free(profile_list);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }

        profile_list=tmp;

        profile_list[count]=profile;
        if(profile_list[count]==NULL){
            printf("Erreur allocation mémoire\n");
            free(profile_list);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }
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
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    choice--;

    struct xccdf_profile *profile=profile_list[choice];

    free(profile_list);

    struct xccdf_policy_model *policy_model=xccdf_policy_model_new(benchmark);
    if(policy_model==NULL){
        printf("Erreur lors de la création du policy_model!!!\n");
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_policy *policy=xccdf_policy_new(policy_model,profile);
    if(policy==NULL){
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_select_iterator *select_iterator=xccdf_policy_get_selected_rules(policy);
    if(select_iterator==NULL){
        printf("Erreur dans la création de l'itérateur de select!!!!\n");
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    while(xccdf_select_iterator_has_more(select_iterator)){
        struct xccdf_select *select=xccdf_select_iterator_next(select_iterator);
        if(select==NULL){
            printf("Erreur lors de l'extraction du select!!!!\n");
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }
        struct xccdf_item *item=xccdf_benchmark_get_item(benchmark,xccdf_select_get_item(select));
        if(item==NULL){
            printf("Erreur lors de la récupération de la règle du benchmark!!!\n");
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }
        xccdf_type_t item_type=xccdf_item_get_type(item);
        if(item_type!=XCCDF_RULE){
            printf("L'item récupéré n'est pas une regle!!!!\n");
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }
        struct xccdf_rule *rule=xccdf_item_to_rule(item);
        if(rule==NULL){
            printf("Erreur lors de la conversion de l'item vers regle!!!!\n");
            xccdf_select_iterator_free(select_iterator);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }
        printf("Titre : %s\nID : %s\n\n",get_rule_title(rule),xccdf_rule_get_id(rule));
    }

    xccdf_select_iterator_free(select_iterator);
    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model);
    ds_sds_session_free(ds_sds_session);
    oscap_source_free(oscap_ds_source);
    oscap_cleanup();

}









/*#define MAX_LEN 100

int mon_callback_start(struct xccdf_rule *rule, void *usr);
int mon_callback_output(struct xccdf_rule_result *result, void *usr);

struct rule_node{
    struct xccdf_rule *rule;
    struct rule_node *next;
};

struct rule_node *push_front(struct rule_node *head, struct xccdf_rule *rule,bool *error){
    struct rule_node *rule_node=malloc(sizeof(struct rule_node));
    if(rule_node==NULL){
        *error=true;
        return head;
    }
    rule_node->rule=rule;
    rule_node->next=head;
    return rule_node;
}

void free_rule_list(struct rule_node *head){
    while(head!=NULL){
        struct rule_node *next=head->next;
        free(head);
        head=next;
    }
}


struct rule_node *collect_rules_recursive(struct xccdf_item *item, struct rule_node *head,bool *error){
    if(*error){
        return head;
    }
    xccdf_type_t item_type=xccdf_item_get_type(item);
    if(item_type==XCCDF_RULE){
        head=push_front(head,(struct xccdf_rule *)item,error);
    }
    else if(item_type==XCCDF_GROUP){
        struct xccdf_item_iterator *child_it=xccdf_group_get_content((struct xccdf_group *)item);
        if(child_it==NULL){
            *error=true;
            return head;
        }
        while(xccdf_item_iterator_has_more(child_it) && !*error){
            struct xccdf_item *child=xccdf_item_iterator_next(child_it);
            head=collect_rules_recursive(child,head,error);
        }
        xccdf_item_iterator_free(child_it);
    }
    return head;
}


struct rule_node *get_all_rules(struct xccdf_benchmark *benchmark,bool *error) {
    struct rule_node *head = NULL;
    struct xccdf_item_iterator *root_it = xccdf_benchmark_get_content(benchmark);
    if(root_it==NULL){
        *error=true;
        return NULL;
    }
    while (xccdf_item_iterator_has_more(root_it) && !*error) {
        struct xccdf_item *item = xccdf_item_iterator_next(root_it);
        head = collect_rules_recursive(item, head,error);
    }
    xccdf_item_iterator_free(root_it);
    return head;
}


const char *get_first_title(struct xccdf_rule *rule) {
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