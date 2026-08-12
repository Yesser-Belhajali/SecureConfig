#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>

#define MAX_LEN 100


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

    printf("Result_ID : %s\nTime : %s\nSeverity : %s\nStatus :  %s\n\n",xccdf_rule_result_get_idref(rule_result),rule_result_time,rule_result_severity ,rule_result_type);
    return 0;
}

int main(int argc, char** argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de seulement 1",argc-1);
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

    free(profile_list);

    struct xccdf_policy *policy=xccdf_session_get_xccdf_policy(session);

    xccdf_policy_model_register_start_callback(policy_model, mon_callback_start, policy);
    xccdf_policy_model_register_output_callback(policy_model, mon_callback_output, NULL);


    
    
    if (xccdf_session_evaluate(session) != 0) {
        printf("Échec de l'évaluation\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }


    printf("Votre score de conformité est = %f%%\n",xccdf_session_get_base_score(session));

    xccdf_session_free(session);
    oscap_cleanup();
    return 0;
}
