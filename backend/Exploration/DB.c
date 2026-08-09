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
}




int main(int argc,char **argv){
    if(argc!=3){
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

    struct oscap_source *oscap_tailoring_source=oscap_source_new_from_file(argv[2]);
    if(oscap_tailoring_source==NULL){
        printf("Erreur dans l'initialisationde la source!!!!\n");
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_tailoring *tailoring=xccdf_tailoring_import_source(oscap_tailoring_source,benchmark);
    if(tailoring==NULL){
        printf("Erreur lors du chargement du tailoring!!!\n");
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile_iterator *tailoring_profiles_iterator=xccdf_tailoring_get_profiles(tailoring);
    if(tailoring_profiles_iterator==NULL){
        printf("Erreur lors de l'initialisation de l'itérateur des profils!!!\n");
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile **profile_list=NULL;

    int count=0;

    while(xccdf_profile_iterator_has_more(tailoring_profiles_iterator)){
        struct xccdf_profile *tailoring_profile=xccdf_profile_iterator_next(tailoring_profiles_iterator);
        if(tailoring_profile==NULL){
            printf("Erreur lors de l'extraction du profil!!!!\n");
            free(profile_list);
            xccdf_profile_iterator_free(tailoring_profiles_iterator);
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        struct xccdf_profile **tmp=realloc(profile_list, (count+1)* sizeof(struct xccdf_profile *));
        if(tmp==NULL){
            printf("Erreur allocation mémoire\n");
            free(profile_list);
            xccdf_profile_iterator_free(tailoring_profiles_iterator);
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        profile_list=tmp;

        profile_list[count]=tailoring_profile;
        
        printf("%d) ID : %s\nTitre : %s\n",count+1,xccdf_profile_get_id(profile_list[count]),get_profile_title(profile_list[count]));
        count++;
    }

    xccdf_profile_iterator_free(tailoring_profiles_iterator);

    int choice;
    printf("Choisissez un profil: ");
    scanf("%d",&choice);
    if(choice<1 || choice>count){
        printf("Choix invalide\n");
        free(profile_list);
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    choice--;

    struct xccdf_profile *tailoring_profile=profile_list[choice];
    if(tailoring_profile==NULL){
        printf("Erreur dans la récupération du profil!!!\n");
        free(profile_list);
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    free(profile_list);


    xccdf_tailoring_resolve(tailoring,benchmark);


    struct xccdf_select_iterator *select_iterator=xccdf_profile_get_selects(tailoring_profile);
    if(select_iterator==NULL){
        printf("Erreur lors de la création de l'itérateur des selects!!!\n");
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    while(xccdf_select_iterator_has_more(select_iterator)){
        struct xccdf_select *select=xccdf_select_iterator_next(select_iterator);
        printf("Selected : %s\nID : %s\n",xccdf_select_get_selected(select)?"true":"false",xccdf_select_get_item(select));
    }

    xccdf_select_iterator_free(select_iterator);

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");

    


    /*struct xccdf_policy_model *policy_model = xccdf_policy_model_new(benchmark);
    if(policy_model==NULL){
        printf("Erreur lors de la création du policy model!!!!\n");
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    xccdf_policy_model_set_tailoring(policy_model,tailoring);


    struct xccdf_policy *policy=xccdf_policy_new(policy_model,tailoring_profile);
    if(policy==NULL){
        printf("Erreur lors de la création de la policy!!!\n");
        xccdf_policy_model_free(policy_model);
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    select_iterator=xccdf_policy_get_selects(policy);
        if(select_iterator==NULL){
        printf("Erreur dans la création de l'itérateur de select!!!!\n");
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
        oscap_source_free(oscap_tailoring_source);
        ds_sds_session_free(ds_sds_session);
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
            oscap_source_free(oscap_tailoring_source);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }
        printf("Selected : %s\nID : %s\n",xccdf_select_get_selected(select)?"true":"false",xccdf_select_get_item(select));
    }

    xccdf_select_iterator_free(select_iterator);

    printf("Nombre de rules = %d",xccdf_policy_get_selected_rules_count(policy));

    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");

    select_iterator=xccdf_policy_get_selected_rules(policy);
    if(select_iterator==NULL){
        printf("Erreur dans la création de l'itérateur de select!!!!\n");
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
        oscap_source_free(oscap_tailoring_source);
        ds_sds_session_free(ds_sds_session);
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
            oscap_source_free(oscap_tailoring_source);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }
        printf("Selected : %s\nID : %s\n",xccdf_select_get_selected(select)?"true":"false",xccdf_select_get_item(select));
    }

    printf("Nombre de rules = %d",xccdf_policy_get_selected_rules_count(policy));


    xccdf_select_iterator_free(select_iterator);
    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model);*/
    oscap_source_free(oscap_tailoring_source);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    return 0;    
}

    /*struct oscap_source *oscap_ds_source=oscap_source_new_from_file(argv[1]);
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


    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur du benchmark!!!!\n");
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
            printf("Erreur lors de l'extraction du profil!!!!\n");
            free(profile_list);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
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


    struct xccdf_tailoring *tailoring = xccdf_tailoring_new();
    xccdf_tailoring_set_id(tailoring, "xccdf_org.example_tailoring_custom");
    xccdf_tailoring_set_benchmark_ref(tailoring,xccdf_benchmark_get_id(benchmark));

    struct xccdf_profile *tailoring_profile=xccdf_profile_new();
    xccdf_profile_set_id(tailoring_profile,"xccdf_org.example_profile_custom");
    xccdf_profile_set_tailoring(tailoring_profile, true);
    xccdf_profile_set_extends(tailoring_profile,xccdf_profile_get_id(profile));

    struct xccdf_select *profile_select=xccdf_select_new();
    xccdf_select_set_item(profile_select,"xccdf_org.ssgproject.content_rule_accounts_password_pam_minlen");
    xccdf_select_set_selected(profile_select,false);
    xccdf_profile_add_select(tailoring_profile,profile_select);

    xccdf_tailoring_add_profile(tailoring,tailoring_profile);
    xccdf_tailoring_export(tailoring,"/home/yesser-belhajali/Desktop/SecureConfig/data/custom-tailoring.xml",xccdf_benchmark_get_schema_version(benchmark));

    xccdf_tailoring_free(tailoring);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    return 0;
}*/









/*




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