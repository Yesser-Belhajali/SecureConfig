#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <stdio.h>
#include <stdlib.h>



int main(int argc, char **argv){
    if(argc<2){
        fprintf(stderr, "Usage: %s <chemin_ds.xml>\n",argv[0]);
        return 1;
    }
    struct xccdf_session *session = xccdf_session_new(argv[1]);
    if (session==NULL){
        fprintf(stderr, "Erreur: ompossible d'ouvrir %s\n",argv[1]);
        return 1;
    }
    if(xccdf_session_load(session)!=0){
        fprintf(stderr, "Erreur: xccdf_session_load a échoué\n");
        xccdf_session_free(session);
        return 1;
    }
    struct xccdf_policy_model *policy_model = xccdf_session_get_policy_model(session);
    if (policy_model == NULL) {
        fprintf(stderr, "Erreur: impossible de récupérer le policy_model\n");
        xccdf_session_free(session);
        return 1;
    }

    struct xccdf_benchmark *benchmark = xccdf_policy_model_get_benchmark(policy_model);

    printf("Benchmark : %s\n\n", xccdf_benchmark_get_id(benchmark));
    printf("Profils disponibles :\n");

    struct xccdf_profile_iterator *it = xccdf_benchmark_get_profiles(benchmark);
    while (xccdf_profile_iterator_has_more(it)) {
        struct xccdf_profile *profile = xccdf_profile_iterator_next(it);
        printf("  - %s\n", xccdf_profile_get_id(profile));
    }
    xccdf_profile_iterator_free(it);

    xccdf_session_free(session);
    return 0;
}