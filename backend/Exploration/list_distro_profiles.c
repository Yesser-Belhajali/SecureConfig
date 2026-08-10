#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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


static int build_paths(const char *id,char *ds_path, size_t ds_size,char *tailoring_path, size_t tailoring_size) {
    int n1 = snprintf(ds_path, ds_size, "../data/%s/ssg-%s-ds.xml", id, id);
    int n2 = snprintf(tailoring_path, tailoring_size, "../data/%s/ssg-%s-tailoring.xml", id, id);

    if (n1 < 0 || (size_t)n1 >= ds_size) return 0;
    if (n2 < 0 || (size_t)n2 >= tailoring_size) return 0;
    return 1;
}

// Vérifie si le fichier de tailoring existe (chemin déjà construit par build_paths).
static int has_tailoring(const char *tailoring_path) {
    return access(tailoring_path, F_OK) == 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de 2\n", argc - 1);
        return 1;
    }

    const char *id = argv[1];

    char ds_path[256];
    char tailoring_path[256];
    if (!build_paths(id, ds_path, sizeof(ds_path), tailoring_path, sizeof(tailoring_path))) {
        printf("Erreur : id trop long ou chemin tronqué\n");
        return 1;
    }

    oscap_init();

    struct oscap_source *oscap_ds_source = oscap_source_new_from_file(ds_path);
    if (oscap_ds_source == NULL) {
        printf("Erreur dans l'initialisation de la source!!!!\n");
        oscap_cleanup();
        return 1;
    }

    struct ds_sds_session *ds_sds_session = ds_sds_session_new_from_source(oscap_ds_source);
    if (ds_sds_session == NULL) {
        printf("Erreur dans la conversion de la source vers une Data Stream Source!!!!\n");
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct oscap_source *oscap_xccdf_source = ds_sds_session_select_checklist(ds_sds_session, NULL, NULL, NULL);
    if (oscap_xccdf_source == NULL) {
        printf("Erreur dans la création de la source XCCDF depuis la Data Stream source!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    oscap_source_free(oscap_ds_source);

    struct xccdf_benchmark *benchmark = xccdf_benchmark_import_source(oscap_xccdf_source);
    if (benchmark == NULL) {
        printf("Erreur dans la création du benchmark!!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile_iterator *profile_iterator = xccdf_benchmark_get_profiles(benchmark);
    if (profile_iterator == NULL) {
        printf("Echec dans la récupération de l'itérateur!!!!\n");
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_profile **profile_list = NULL;
    int count = 0;

    while (xccdf_profile_iterator_has_more(profile_iterator)) {
        struct xccdf_profile *profile = xccdf_profile_iterator_next(profile_iterator);
        if (profile == NULL) {
            printf("Erreur dans l'extraction du profil!!!!\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        struct xccdf_profile **tmp = realloc(profile_list, (count + 1) * sizeof(struct xccdf_profile *));
        if (tmp == NULL) {
            printf("Erreur allocation mémoire\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }
        profile_list = tmp;
        profile_list[count] = profile;

        printf("%d) ID : %s\nTitre : %s\n", count + 1,xccdf_profile_get_id(profile_list[count]), get_profile_title(profile_list[count]));
        count++;
    }


    if (has_tailoring(tailoring_path)) {

        printf("Les Profils de tailoring sont : \n");

        struct oscap_source *oscap_tailoring_source = oscap_source_new_from_file(tailoring_path);
        if (oscap_tailoring_source == NULL) {
            printf("Erreur dans l'initialisation de la source de tailoring!!!!\n");
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        struct xccdf_tailoring *tailoring = xccdf_tailoring_import_source(oscap_tailoring_source, benchmark);
        if (tailoring == NULL) {
            printf("Erreur lors du chargement du tailoring!!!\n");
            oscap_source_free(oscap_tailoring_source);
            xccdf_profile_iterator_free(profile_iterator);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }


        xccdf_profile_iterator_free(profile_iterator);

        profile_iterator = xccdf_tailoring_get_profiles(tailoring);
        if (profile_iterator == NULL) {
            printf("Erreur lors de l'initialisation de l'itérateur des profils de tailoring!!!\n");
            xccdf_tailoring_free(tailoring);
            oscap_source_free(oscap_tailoring_source);
            free(profile_list);
            xccdf_benchmark_free(benchmark);
            ds_sds_session_free(ds_sds_session);
            oscap_cleanup();
            return 1;
        }

        while (xccdf_profile_iterator_has_more(profile_iterator)) {
            struct xccdf_profile *profile = xccdf_profile_iterator_next(profile_iterator);
            if (profile == NULL) {
                printf("Erreur lors de l'extraction du profil de tailoring!!!!\n");
                xccdf_profile_iterator_free(profile_iterator);
                xccdf_tailoring_free(tailoring);
                oscap_source_free(oscap_tailoring_source);
                free(profile_list);
                xccdf_benchmark_free(benchmark);
                ds_sds_session_free(ds_sds_session);
                oscap_cleanup();
                return 1;
            }

            struct xccdf_profile **tmp = realloc(profile_list, (count + 1) * sizeof(struct xccdf_profile *));
            if (tmp == NULL) {
                printf("Erreur allocation mémoire\n");
                xccdf_profile_iterator_free(profile_iterator);
                xccdf_tailoring_free(tailoring);
                oscap_source_free(oscap_tailoring_source);
                free(profile_list);
                xccdf_benchmark_free(benchmark);
                ds_sds_session_free(ds_sds_session);
                oscap_cleanup();
                return 1;
            }
            profile_list = tmp;
            profile_list[count] = profile;

            printf("%d) ID : %s\nTitre : %s\n", count + 1,xccdf_profile_get_id(profile_list[count]), get_profile_title(profile_list[count]));
            count++;
        }

        xccdf_tailoring_free(tailoring);
        oscap_source_free(oscap_tailoring_source);
        
    }

    xccdf_profile_iterator_free(profile_iterator);


    free(profile_list);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_cleanup();
    return 0;
}