// test_memory.c
//
// Compilation : make memtest   (ASan + UBSan, voir Makefile)
// Exécution   : ./build/test_memory <benchmark_id> <profil_natif_id>
//
// Ce test exerce, dans l'ordre :
//   1. list_profiles_for_distro / free_profiles_for_distro
//   2. list_rules_for_ds / free_rule_info_list
//   3. selected_rules_for_profile sur un profil NATIF
//   4. all_rules_with_selection_for_profile sur un profil NATIF, vérifie
//      la cohérence du compte avec l'étape 3
//   5. create_tailoring_profile FROM SCRATCH (base_profile_id = NULL)
//   6. create_tailoring_profile en ETENDANT le profil natif fourni
//   7. create_tailoring_profile en ETENDANT le profil créé à l'étape 6
//      (chaîne d'héritage à deux niveaux dans le tailoring - le cas
//      qui avait révélé la nécessité de xccdf_policy_model_set_tailoring)
//   8. selected_rules_for_profile / all_rules_with_selection_for_profile
//      sur le profil chaîné de l'étape 7, pour confirmer la résolution
//   9. cas d'erreur : nom dupliqué (doit retourner -2)
//   10. cas d'erreur : profil de base introuvable (doit retourner -3)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../scap_service.h"

static void print_profiles(struct profile_list *profiles, int count, const char *label) {
    printf("--- %s (%d) ---\n", label, count);
    for (int i = 0; i < count; i++) {
        printf("  id=%s title=%s extends=%s\n",
               profiles[i].id,
               profiles[i].title ? profiles[i].title : "(null)",
               profiles[i].extends ? profiles[i].extends : "(none)");
    }
}

static void print_rules_summary(struct rule_list *rules, int count, const char *label) {
    int selected_true = 0;
    for (int i = 0; i < count; i++) {
        if (rules[i].selected) selected_true++;
    }
    printf("--- %s : %d règles, %d selected=true ---\n", label, count, selected_true);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <benchmark_id> <profil_natif_id>\n", argv[0]);
        return 1;
    }

    const char *benchmark_id = argv[1];
    const char *native_profile_id = argv[2];

    char ds_path[256];
    snprintf(ds_path, sizeof(ds_path), "../data/%s/ssg-%s-ds.xml", benchmark_id, benchmark_id);

    int failures = 0;

    // --- 1. list_profiles_for_distro ---
    struct profile_list *profiles = NULL;
    int profiles_count = 0;
    struct profile_list *tailoring_profiles = NULL;
    int tailoring_count = 0;

    if (list_profiles_for_distro(benchmark_id, &profiles, &profiles_count,
                                   &tailoring_profiles, &tailoring_count) != 0) {
        printf("ECHEC list_profiles_for_distro\n");
        return 1;
    }
    print_profiles(profiles, profiles_count, "Profils natifs");
    print_profiles(tailoring_profiles, tailoring_count, "Profils tailoring (avant création)");
    free_profiles_for_distro(profiles, profiles_count, tailoring_profiles, tailoring_count);

    // --- 2. list_rules_for_ds ---
    struct rule_list *all_rules = NULL;
    int all_rules_count = list_rules_for_ds(ds_path, &all_rules);
    if (all_rules_count < 0) {
        printf("ECHEC list_rules_for_ds\n");
        return 1;
    }
    print_rules_summary(all_rules, all_rules_count, "list_rules_for_ds");
    if (all_rules_count < 2) {
        printf("ECHEC : besoin d'au moins 2 règles dans le benchmark pour la suite du test\n");
        free_rule_info_list(all_rules, all_rules_count);
        return 1;
    }
    // on garde 2 IDs de règles réelles pour construire des profils de test valides
    char rule_id_a[256], rule_id_b[256];
    strncpy(rule_id_a, all_rules[0].id, sizeof(rule_id_a) - 1);
    rule_id_a[sizeof(rule_id_a) - 1] = '\0';
    strncpy(rule_id_b, all_rules[1].id, sizeof(rule_id_b) - 1);
    rule_id_b[sizeof(rule_id_b) - 1] = '\0';
    free_rule_info_list(all_rules, all_rules_count);

    // --- 3. selected_rules_for_profile sur un profil NATIF ---
    struct rule_list *native_selected = NULL;
    int native_selected_count = selected_rules_for_profile(benchmark_id, native_profile_id, &native_selected);
    if (native_selected_count < 0) {
        printf("ECHEC selected_rules_for_profile (natif: %s)\n", native_profile_id);
        return 1;
    }
    print_rules_summary(native_selected, native_selected_count, "selected_rules_for_profile (natif)");
    free_rule_info_list(native_selected, native_selected_count);

    // --- 4. all_rules_with_selection_for_profile sur un profil NATIF ---
    struct rule_list *native_all = NULL;
    int native_all_count = all_rules_with_selection_for_profile(benchmark_id, native_profile_id, &native_all);
    if (native_all_count < 0) {
        printf("ECHEC all_rules_with_selection_for_profile (natif: %s)\n", native_profile_id);
        return 1;
    }
    int native_all_selected = 0;
    for (int i = 0; i < native_all_count; i++) if (native_all[i].selected) native_all_selected++;
    print_rules_summary(native_all, native_all_count, "all_rules_with_selection_for_profile (natif)");
    free_rule_info_list(native_all, native_all_count);

    if (native_all_selected != native_selected_count) {
        printf("INCOHERENCE (natif) : selected_rules_for_profile=%d vs all_rules selected=%d\n",
               native_selected_count, native_all_selected);
        failures++;
    } else {
        printf("OK : cohérence natif (%d règles sélectionnées)\n", native_selected_count);
    }

    // --- noms uniques basés sur l'horodatage, pour ne pas collisionner
    //     avec des runs précédents (le tailoring persiste sur disque) ---
    long ts = (long)time(NULL);
    char name_scratch[128], name_extend_native[128], name_extend_chain[128];
    snprintf(name_scratch, sizeof(name_scratch), "Test From Scratch %ld", ts);
    snprintf(name_extend_native, sizeof(name_extend_native), "Test Extend Native %ld", ts);
    snprintf(name_extend_chain, sizeof(name_extend_chain), "Test Extend Chain %ld", ts);

    // --- 5. create_tailoring_profile FROM SCRATCH ---
    char id_scratch[256] = {0};
    const char *added_scratch[] = { rule_id_a };
    int ret_scratch = create_tailoring_profile(
        benchmark_id, name_scratch,
        NULL, // from scratch
        added_scratch, 1,
        NULL, 0,
        id_scratch, sizeof(id_scratch)
    );
    if (ret_scratch != 0) {
        printf("ECHEC create_tailoring_profile (from scratch), code=%d\n", ret_scratch);
        failures++;
    } else {
        printf("OK : profil from-scratch créé, id=%s\n", id_scratch);
    }

    // --- 6. create_tailoring_profile en étendant le profil NATIF ---
    char id_extend_native[256] = {0};
    const char *removed_native[] = { rule_id_b };
    int ret_extend_native = create_tailoring_profile(
        benchmark_id, name_extend_native,
        native_profile_id,
        NULL, 0,
        removed_native, 1,
        id_extend_native, sizeof(id_extend_native)
    );
    if (ret_extend_native != 0) {
        printf("ECHEC create_tailoring_profile (extend natif), code=%d\n", ret_extend_native);
        failures++;
    } else {
        printf("OK : profil étendant le natif créé, id=%s\n", id_extend_native);
    }

    // --- 7. create_tailoring_profile en étendant le profil de l'étape 6
    //        (chaîne d'héritage à deux niveaux dans le tailoring) ---
    char id_extend_chain[256] = {0};
    int ret_extend_chain = 0;
    if (ret_extend_native == 0) {
        ret_extend_chain = create_tailoring_profile(
            benchmark_id, name_extend_chain,
            id_extend_native, // étend le profil tailoring créé juste avant
            NULL, 0,
            NULL, 0,
            id_extend_chain, sizeof(id_extend_chain)
        );
        if (ret_extend_chain != 0) {
            printf("ECHEC create_tailoring_profile (chaîne tailoring->tailoring), code=%d\n", ret_extend_chain);
            failures++;
        } else {
            printf("OK : profil chaîné (tailoring étend tailoring) créé, id=%s\n", id_extend_chain);
        }
    } else {
        printf("SAUTE : étape 7 dépend du succès de l'étape 6\n");
    }

    // --- 8. vérifier la résolution du profil chaîné ---
    if (ret_extend_chain == 0) {
        struct rule_list *chain_selected = NULL;
        int chain_selected_count = selected_rules_for_profile(benchmark_id, id_extend_chain, &chain_selected);
        if (chain_selected_count < 0) {
            printf("ECHEC selected_rules_for_profile sur le profil chaîné\n");
            failures++;
        } else {
            print_rules_summary(chain_selected, chain_selected_count, "selected_rules_for_profile (chaîné)");
            free_rule_info_list(chain_selected, chain_selected_count);
        }

        struct rule_list *chain_all = NULL;
        int chain_all_count = all_rules_with_selection_for_profile(benchmark_id, id_extend_chain, &chain_all);
        if (chain_all_count < 0) {
            printf("ECHEC all_rules_with_selection_for_profile sur le profil chaîné\n");
            failures++;
        } else {
            int chain_all_selected = 0;
            for (int i = 0; i < chain_all_count; i++) if (chain_all[i].selected) chain_all_selected++;
            print_rules_summary(chain_all, chain_all_count, "all_rules_with_selection_for_profile (chaîné)");
            free_rule_info_list(chain_all, chain_all_count);

            if (chain_all_selected != chain_selected_count) {
                printf("INCOHERENCE (chaîné) : %d vs %d\n", chain_selected_count, chain_all_selected);
                failures++;
            } else if (chain_selected_count == native_selected_count) {
                printf("SUSPECT : le profil chaîné a le même nombre de règles que le natif "
                       "(%d) - l'héritage tailoring->tailoring pourrait ne pas être résolu\n",
                       chain_selected_count);
                failures++;
            } else {
                printf("OK : le profil chaîné a %d règles sélectionnées (natif en a %d) "
                       "- héritage cohérent\n", chain_selected_count, native_selected_count);
            }
        }
    }

    // --- 9. cas d'erreur : nom dupliqué (doit retourner -2) ---
    char id_dup[256] = {0};
    int ret_dup = create_tailoring_profile(
        benchmark_id, name_scratch, // même nom que l'étape 5
        NULL, NULL, 0, NULL, 0,
        id_dup, sizeof(id_dup)
    );
    if (ret_dup != -2) {
        printf("ECHEC test nom dupliqué : attendu -2, obtenu %d\n", ret_dup);
        failures++;
    } else {
        printf("OK : nom dupliqué correctement rejeté (-2)\n");
    }

    // --- 10. cas d'erreur : profil de base introuvable (doit retourner -3) ---
    char id_bad_base[256] = {0};
    char name_bad_base[128];
    snprintf(name_bad_base, sizeof(name_bad_base), "Test Bad Base %ld", ts);
    int ret_bad_base = create_tailoring_profile(
        benchmark_id, name_bad_base,
        "ce_profil_n_existe_vraiment_pas_du_tout",
        NULL, 0, NULL, 0,
        id_bad_base, sizeof(id_bad_base)
    );
    if (ret_bad_base != -3) {
        printf("ECHEC test profil de base invalide : attendu -3, obtenu %d\n", ret_bad_base);
        failures++;
    } else {
        printf("OK : profil de base invalide correctement rejeté (-3)\n");
    }

    // --- 11. test profil inexistant sur les fonctions de lecture (doit retourner -1) ---
    struct rule_list *bogus = NULL;
    int bogus_count = selected_rules_for_profile(benchmark_id, "ce_profil_n_existe_pas", &bogus);
    if (bogus_count != -1) {
        printf("ECHEC test profil inexistant (lecture) : attendu -1, obtenu %d\n", bogus_count);
        failures++;
    } else {
        printf("OK : profil inexistant correctement rejeté en lecture (-1)\n");
    }
    if (bogus_count >= 0) free_rule_info_list(bogus, bogus_count);

    printf("\n=== RESUME : %d échec(s) fonctionnel(s) ===\n", failures);
    printf("(le rapport ASan/UBSan, s'il y a une erreur mémoire, apparaîtra séparément ci-dessus ou ci-dessous)\n");

    return failures > 0 ? 1 : 0;
}