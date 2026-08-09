#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <ds_sds_session.h>
#include <oscap_source.h>



int main(int argc,char **argv){
    if(argc!=2){
        printf("Nous avons besoin de 2 parametres!!!\n");
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


    struct xccdf_tailoring *tailoring = xccdf_tailoring_new();

    char nom[50];
    printf("Donner l'ID du fichier de tailoring : ");
    fgets(nom, sizeof(nom), stdin);

    xccdf_tailoring_set_id(tailoring, nom);

    xccdf_tailoring_set_benchmark_ref(tailoring,xccdf_benchmark_get_id(benchmark));

    xccdf_tailoring_export(tailoring, "/home/yesser-belhajali/Desktop/SecureConfig/data/ubuntu2404/ssg-ubuntu2404-tailoring.xml", xccdf_benchmark_get_schema_version(benchmark));


    xccdf_tailoring_free(tailoring);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    return 0;
}