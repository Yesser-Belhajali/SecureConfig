#include <stdio.h>
#include <stdlib.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>


int main(int argc, char** argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de seulement 1",argc-1);
        return 1;
    }

    struct xccdf_session* session=xccdf_session_new(argv[1]);

    if(session==NULL){
        printf("Echec dans l'initialisation de la session");
        return 1;
    }

    if(xccdf_session_load(session)!=0){
        printf("Le chargement des composants a échoué!!!");
        xccdf_session_free(session);
        return 1;
    }

    struct xccdf_policy_model *policy_model=xccdf_session_get_policy_model(session);
    if(policy_model==NULL){
        printf("Echech dans la récupération de la policy_model!!!!");
        xccdf_session_free(session);
        return 1;
    }

    struct xccdf_benchmark *benchmark=xccdf_policy_model_get_benchmark(policy_model);
    if(benchmark==NULL){
        printf("Echec dans la récupération du benchmark!!!!");
        xccdf_session_free(session);
        return 1;
    }

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur!!!!");
        xccdf_session_free(session);
        return 1;
    }

    while(xccdf_profile_iterator_has_more(profile_iterator)){
        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            printf("Erreur dans l'extraction du profil!!!!");
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_session_free(session);
            return 1;
        }
        printf("  - %s\n", xccdf_profile_get_id(profile));
    }
    xccdf_profile_iterator_free(profile_iterator);
    xccdf_session_free(session);
    return 0;
}