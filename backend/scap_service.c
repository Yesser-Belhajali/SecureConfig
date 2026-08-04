#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <ds_sds_session.h>
#include <oscap_source.h>
#include "scap_service.h"



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

void free_profile_list(struct profile_list *profiles,int count){
    if(profiles==NULL){
        return;
    }
    for(int i=0;i<count;i++){
        free(profiles[i].id);
        free(profiles[i].title);
    }
    free(profiles);
}


int list_profiles_for_ds(const char *ds_path,struct profile_list **out_profiles){

    if(out_profiles==NULL){
        return -1;
    }

    *out_profiles=NULL;

    oscap_init();

    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(ds_path);
    if(oscap_ds_source==NULL){
        oscap_cleanup();
        return -1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return -1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return -1;
    }

    struct profile_list *profiles=NULL;

    int count=0;

    while(xccdf_profile_iterator_has_more(profile_iterator)){

        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            free_profile_list(profiles,count);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        struct profile_list *tmp=realloc(profiles, (count+1)* sizeof(struct profile_list));
        if(tmp==NULL){
            free_profile_list(profiles,count);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }

        profiles=tmp;

        profiles[count].id = NULL;
        profiles[count].title = NULL;

        const char *id=xccdf_profile_get_id(profile);
        const char *title=get_profile_title(profile);

        profiles[count].id = id ? strdup(id) : NULL;
        profiles[count].title = title ? strdup(title) : NULL;

        if((id!=NULL && profiles[count].id==NULL) || (title!=NULL && profiles[count].title==NULL)){
            free(profiles[count].id);
            free(profiles[count].title);
            free_profile_list(profiles,count);
            xccdf_profile_iterator_free(profile_iterator);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return -1;
        }
        count++;
    }


    xccdf_profile_iterator_free(profile_iterator);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();

    *out_profiles=profiles;
    return count;
}