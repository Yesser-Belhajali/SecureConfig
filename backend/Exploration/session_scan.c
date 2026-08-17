#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xccdf_benchmark.h>
#include <oscap.h>
#include <ds_sds_session.h>



int main(int argc,char **argv){

    if(argc!=4){
        printf("Nous avons besoin de 3 arguments!!!!!\n");
        return 1;
    }

    if(access(argv[2], F_OK) != 0){
        printf("Le fichier de tailoring n'existe pas!!!!!\n");
        return 1;
    }

    oscap_init();

    struct oscap_source *oscap_ds_source = oscap_source_new_from_file(argv[1]);
    if(oscap_ds_source == NULL){
        printf("Erreur dans la création de la DS oscap_source!!!!\n");
        oscap_cleanup();
        return 1;
    }

    struct ds_sds_session *ds_sds_session = ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session == NULL){
        printf("Erreur dans le conversion du DS oscap_source vers ds_sds_session!!!\n");
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct oscap_source *oscap_xccdf_source = ds_sds_session_select_checklist(ds_sds_session, NULL, NULL, NULL);
    if(oscap_xccdf_source == NULL){
        printf("Erreur dans l'extraction du benchmark depuis la ds_sds_session!!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark = xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark == NULL){
        printf("Erreur dans l'import du benhcmark depuis la source!!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    ds_sds_session_free(ds_sds_session);

    struct oscap_source *oscap_tailoring_source=oscap_source_new_from_file(argv[2]);
    if(oscap_tailoring_source==NULL){
        printf("Erreur dans la création de la tailoring oscap_source!!!!!\n");
        xccdf_benchmark_free(benchmark);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_tailoring *tailoring=xccdf_tailoring_import_source(oscap_tailoring_source,benchmark);
    if(tailoring==NULL){
        printf("Erreur dans l'import du tailoring depuis la source!!!!\n");
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        oscap_cleanup();
        return 1;
    }

    oscap_source_free(oscap_tailoring_source);

    struct xccdf_profile *profile=xccdf_tailoring_get_profile_by_id(tailoring,argv[3]);
    if(profile==NULL){
        printf("Profil non existant!!!\n");
        xccdf_tailoring_free(tailoring);
        xccdf_benchmark_free(benchmark);
        oscap_cleanup();
        return 1;
    }

    if(!xccdf_tailoring_remove_profile(tailoring,profile)){
        printf("Erreur dans la suppression du profil!!!!!\n");
        xccdf_tailoring_free(tailoring);
        xccdf_benchmark_free(benchmark);
        oscap_cleanup();
        return 1;
    }


    printf("Suppression effectuée avec succes!!!!\n");

    xccdf_tailoring_export(tailoring);


    xccdf_tailoring_free(tailoring);
    xccdf_benchmark_free(benchmark);
    oscap_cleanup();
    return 0;
}