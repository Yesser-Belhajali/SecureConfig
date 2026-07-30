#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>

#define MAX_LEN 100

int mon_callback_start(struct xccdf_rule *rule, void *usr);
int mon_callback_output(struct xccdf_rule_result *result, void *usr);


int main(int argc, char** argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de seulement 1",argc-1);
        return 1;
    }
    oscap_init();

    struct xccdf_session* session=xccdf_session_new(argv[1]);

    if(session==NULL){
        printf("Echec dans l'initialisation de la session");
        oscap_cleanup();
        return 1;
    }

    if(xccdf_session_load(session)!=0){
        printf("Le chargement des composants a échoué!!!");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_policy_model *policy_model=xccdf_session_get_policy_model(session);
    if(policy_model==NULL){
        printf("Echech dans la récupération de la policy_model!!!!");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_benchmark *benchmark=xccdf_policy_model_get_benchmark(policy_model);
    if(benchmark==NULL){
        printf("Echec dans la récupération du benchmark!!!!");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur!!!!");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile **profile_list=NULL;
    int count=0;

    while(xccdf_profile_iterator_has_more(profile_iterator)){
        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            printf("Erreur dans l'extraction du profil!!!!");
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

    xccdf_policy_model_set_show_rule_details(policy_model,true);

    struct xccdf_policy *policy=xccdf_policy_new(policy_model,profile_list[choice]);
    if(policy==NULL){
        printf("Echec de la création de la policy!!!!");
        free(profile_list);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    if(xccdf_policy_model_add_policy(policy_model,policy)==false){
        printf("Echec lors de l'ajout de la policy dans la liste des policies de policy_model!!!!");
        free(profile_list);
        xccdf_policy_free(policy);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    if(!xccdf_session_set_profile_id(session,xccdf_profile_get_id(profile_list[choice]))){
        printf("Erreur dans l'initialisation du profil de la session");
        free(profile_list);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    };

    xccdf_policy_model_register_start_callback(policy_model, mon_callback_start, NULL);
    xccdf_policy_model_register_output_callback(policy_model, mon_callback_output, NULL);
    
    
    if (xccdf_session_evaluate(session) != 0) {
        printf("Échec de l'évaluation\n");
        free(profile_list);
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }
    
    printf("Evaluation terminée avec succes!!!!!!YYAAAAYYY\n");



    free(profile_list);
    xccdf_session_free(session);
    oscap_cleanup();
    return 0;
}

int mon_callback_start(struct xccdf_rule *rule, void *usr) {
    const char *rule_id = xccdf_rule_get_id(rule);
    printf("[START] Évaluation en cours de la règle : %s...\n", rule_id);
    return 0;
}

int mon_callback_output(struct xccdf_rule_result *result, void *usr) {
    const char *rule_id = xccdf_rule_result_get_idref(result);
    xccdf_test_result_type_t res_type = xccdf_rule_result_get_result(result);
    printf("[OUTPUT] Règle %s terminée avec le statut #%d\n", rule_id, res_type);
    return 0;
}