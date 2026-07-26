#include <stdio.h>
#include <stdlib.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>


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
    printf("Le retour =%d",xccdf_session_evaluate(session));
    /*struct xccdf_policy_model * policy_model=xccdf_session_get_policy_model(session);

    if(policy_model==NULL){
        fprintf("Erreur dans l'extraction du modele de policy!!!");
        xccdf_session_free(session);
        return 1;
    }*/

    xccdf_session_free(session);
    return 0;
}