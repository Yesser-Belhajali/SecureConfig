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

    struct xccdf_profile_iterator *profile_iterator=xccdf_benchmark_get_profiles(benchmark);
    if(profile_iterator==NULL){
        printf("Echec dans la récupération de l'itérateur!!!!\n");
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
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
            printf("Erreur dans l'extraction du profil!!!!\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
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
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
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

    profile_iterator=xccdf_tailoring_get_profiles(tailoring);
    if(profile_iterator==NULL){
        printf("Erreur lors de l'initialisation de l'itérateur des profils!!!\n");
        free(profile_list);
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    while(xccdf_profile_iterator_has_more(profile_iterator)){
        struct xccdf_profile *profile=xccdf_profile_iterator_next(profile_iterator);
        if(profile==NULL){
            printf("Erreur lors de l'extraction du profil!!!!\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
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
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
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
    printf("Choisissez le profil que vous voulez étendre: ");
    scanf("%d",&choice);

    // IMPORTANT : vider le \n restant dans le buffer stdin après scanf,
    // sinon le fgets suivant lira immédiatement ce \n au lieu d'attendre
    // une vraie saisie de l'utilisateur
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

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

    struct xccdf_profile *profile=profile_list[choice];
    if(profile==NULL){
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


    struct xccdf_profile *tailoring_profile=xccdf_profile_new();

    char id[100];
    printf("Donner l'ID du nouvel profil de tailoring : ");
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = '\0'; // retire le \n final laissé par fgets

    if (strlen(id) == 0) {
        printf("ID vide, abandon.\n");
        xccdf_profile_free(xccdf_profile_to_item(tailoring_profile));
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    char nom[100];
    printf("Donner le nom du nouvel profil de tailoring : ");
    fgets(nom, sizeof(nom), stdin);
    nom[strcspn(nom, "\n")] = '\0'; // retire le \n final laissé par fgets

    if (strlen(nom) == 0) {
        printf("nom vide, abandon.\n");
        xccdf_profile_free(xccdf_profile_to_item(tailoring_profile));
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    xccdf_profile_set_id(tailoring_profile,id);
    

    struct oscap_text *title = oscap_text_new();

    if (title == NULL) {
        printf("Erreur lors de la création du titre\n");

        xccdf_profile_free(xccdf_profile_to_item(tailoring_profile));
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();

        return 1;
    }

    if (!oscap_text_set_text(title, nom)) {
        printf("Erreur lors de la définition du texte du titre\n");

        oscap_text_free(title);
        xccdf_profile_free(xccdf_profile_to_item(tailoring_profile));
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();

        return 1;
    }

    if (!oscap_text_set_lang(title, "fr")) {
        printf("Erreur lors de la définition de la langue du titre\n");

        oscap_text_free(title);
        xccdf_profile_free(xccdf_profile_to_item(tailoring_profile));
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();

        return 1;
    }

    if (!xccdf_profile_add_title(tailoring_profile, title)) {
        printf("Erreur lors de l'ajout du titre\n");

        oscap_text_free(title);

        xccdf_profile_free(xccdf_profile_to_item(tailoring_profile));
        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();

        return 1;
    }
    xccdf_profile_set_tailoring(tailoring_profile, true);
    xccdf_profile_set_extends(tailoring_profile,xccdf_profile_get_id(profile));

    struct xccdf_select *profile_select=xccdf_select_new();
    xccdf_select_set_item(profile_select,"xccdf_org.ssgproject.content_rule_systemd_journal_upload_url");
    xccdf_select_set_selected(profile_select,false);
    xccdf_profile_add_select(tailoring_profile,profile_select);

    xccdf_tailoring_add_profile(tailoring,tailoring_profile);
    xccdf_tailoring_export(tailoring,"/home/yesser-belhajali/Desktop/SecureConfig/data/ubuntu2404/ssg-ubuntu2404-tailoring.xml",xccdf_benchmark_get_schema_version(benchmark));


    xccdf_tailoring_free(tailoring);
    oscap_source_free(oscap_tailoring_source);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    return 0;
}

/*struct xccdf_profile *tailoring_profile=xccdf_profile_new();
    xccdf_profile_set_id(tailoring_profile,"xccdf_org.example_profile_custom");
    struct oscap_text *title = oscap_text_new();

    xccdf_profile_set_tailoring(tailoring_profile, true);
    xccdf_profile_set_extends(tailoring_profile,xccdf_profile_get_id(profile));

    struct xccdf_select *profile_select=xccdf_select_new();
    xccdf_select_set_item(profile_select,"xccdf_org.ssgproject.content_rule_accounts_password_pam_minlen");
    xccdf_select_set_selected(profile_select,false);
    xccdf_profile_add_select(tailoring_profile,profile_select);

    xccdf_tailoring_add_profile(tailoring,tailoring_profile);*/

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

    free(profile_list);*/