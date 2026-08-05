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


void collect_rules_recursive(struct xccdf_item *benchmark_item,struct xccdf_policy *policy){
    xccdf_type_t benchmark_item_type=xccdf_item_get_type(benchmark_item);
    if(benchmark_item_type==XCCDF_RULE){
        if(xccdf_policy_is_item_selected(policy,xccdf_item_get_id(benchmark_item))){
            struct xccdf_rule *rule=xccdf_item_to_rule(benchmark_item);
            if(rule==NULL){
                printf("Erreur lors de la conversion de l'item vers rule!!!\n");
                oscap_cleanup();
                return ;
            }
            printf("Titre : %s\nID : %s\n\n",xccdf_rule_get_id(rule),get_rule_title(rule));
        }
    }
    else if(benchmark_item_type==XCCDF_GROUP){
        struct xccdf_item_iterator *group_iterator=xccdf_group_get_content(xccdf_item_to_group(benchmark_item));
        if(group_iterator==NULL){
            printf("Erreur dans la création de l'itérateur du groupe!!!!\n");
            oscap_cleanup();
            return ;
        }
        while(xccdf_item_iterator_has_more(group_iterator)){
            struct xccdf_item *group_item=xccdf_item_iterator_next(group_iterator);
            if(group_item==NULL){
                printf("Erreur dans la création de l'itérateur du groupe!!!!\n");
                xccdf_item_iterator_free(group_iterator);
                oscap_cleanup();
                return;
            }
            collect_rules_recursive(group_item,policy);
        }
        xccdf_item_iterator_free(group_iterator);
    }
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

    /*int choice;
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

    choice--;*/

    struct xccdf_profile *profile=profile_list[4];

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

    struct xccdf_item_iterator *benchmark_iterator=xccdf_benchmark_get_content(benchmark);
    if(benchmark_iterator==NULL){
        printf("Erreur dans la création de l'itérateur du benchmark!!!!\n");
        xccdf_policy_free(policy);
        xccdf_policy_model_free(policy_model);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }
    while(xccdf_item_iterator_has_more(benchmark_iterator)){
        struct xccdf_item *benchmark_item=xccdf_item_iterator_next(benchmark_iterator);
        if(benchmark_item==NULL){
            printf("Erreur dans la récupération de l'item!!!!\n");
            xccdf_item_iterator_free(benchmark_iterator);
            xccdf_policy_free(policy);
            xccdf_policy_model_free(policy_model);
            ds_sds_session_free(ds_sds_session);
            oscap_source_free(oscap_ds_source);
            oscap_cleanup();
            return 1;
        }
        collect_rules_recursive(benchmark_item,policy);
    }
    

    xccdf_item_iterator_free(benchmark_iterator);
    xccdf_policy_free(policy);
    xccdf_policy_model_free(policy_model);
    ds_sds_session_free(ds_sds_session);
    oscap_source_free(oscap_ds_source);
    oscap_cleanup();
    return 0;

}