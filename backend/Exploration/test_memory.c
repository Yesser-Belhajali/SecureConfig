// test_memory.c
// Compilation :
//   gcc -Wall -g -I/usr/local/include/openscap -o build/test_memory \
//       test_memory.c scap_service.c json_utils.c \
//       -L/usr/local/lib -lopenscap -lcjson
//
// Exécution :
//   valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
//       ./build/test_memory <benchmark_id> <profil_natif_id> <profil_tailoring_id>
//
// Le troisième argument (profil de tailoring) est optionnel si vous n'avez
// pas encore de fichier tailoring pour ce benchmark - dans ce cas le test
// de résolution tailoring sera juste sauté (message affiché).

#include <stdio.h>
#include <stdlib.h>
#include "../scap_service.h"

static void print_profiles(struct profile_list *profiles, int count, const char *label) {
    printf("--- %s (%d) ---\n", label, count);
    for (int i = 0; i < count; i++) {
        printf("  id=%s title=%s\n", profiles[i].id, profiles[i].title ? profiles[i].title : "(null)");
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
    if (argc < 3) {
        printf("Usage: %s <benchmark_id> <profil_natif_id> [profil_tailoring_id]\n", argv[0]);
        return 1;
    }

    const char *benchmark_id = argv[1];
    const char *native_profile_id = argv[2];
    const char *tailoring_profile_id = (argc >= 4) ? argv[3] : NULL;

    char ds_path[256];
    snprintf(ds_path, sizeof(ds_path), "../data/%s/ssg-%s-ds.xml", benchmark_id, benchmark_id);

    // --- 1. list_profiles_for_distro (profils natifs + tailoring) ---
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
    print_profiles(tailoring_profiles, tailoring_count, "Profils tailoring");
    free_profiles_for_distro(profiles, profiles_count, tailoring_profiles, tailoring_count);

    // --- 2. list_rules_for_ds (toutes les règles, sans notion de profil) ---
    struct rule_list *all_rules = NULL;
    int all_rules_count = list_rules_for_ds(ds_path, &all_rules);
    if (all_rules_count < 0) {
        printf("ECHEC list_rules_for_ds\n");
        return 1;
    }
    print_rules_summary(all_rules, all_rules_count, "list_rules_for_ds");
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
    print_rules_summary(native_all, native_all_count, "all_rules_with_selection_for_profile (natif)");
    free_rule_info_list(native_all, native_all_count);

    // --- 5. Idem sur un profil TAILORING, si fourni ---
    if (tailoring_profile_id != NULL) {
        struct rule_list *tailoring_selected = NULL;
        int tailoring_selected_count = selected_rules_for_profile(benchmark_id, tailoring_profile_id, &tailoring_selected);
        if (tailoring_selected_count < 0) {
            printf("ECHEC selected_rules_for_profile (tailoring: %s)\n", tailoring_profile_id);
            return 1;
        }
        print_rules_summary(tailoring_selected, tailoring_selected_count, "selected_rules_for_profile (tailoring)");
        free_rule_info_list(tailoring_selected, tailoring_selected_count);

        struct rule_list *tailoring_all = NULL;
        int tailoring_all_count = all_rules_with_selection_for_profile(benchmark_id, tailoring_profile_id, &tailoring_all);
        if (tailoring_all_count < 0) {
            printf("ECHEC all_rules_with_selection_for_profile (tailoring: %s)\n", tailoring_profile_id);
            return 1;
        }
        print_rules_summary(tailoring_all, tailoring_all_count, "all_rules_with_selection_for_profile (tailoring)");

        // vérification cohérence : le compte "selected=true" doit être identique
        // entre les deux fonctions pour le même profil
        int selected_in_all = 0;
        for (int i = 0; i < tailoring_all_count; i++) {
            if (tailoring_all[i].selected) selected_in_all++;
        }
        if (selected_in_all != tailoring_selected_count) {
            printf("INCOHERENCE : selected_rules_for_profile=%d vs all_rules count selected=%d\n",
                   tailoring_selected_count, selected_in_all);
        } else {
            printf("OK : cohérence entre les deux fonctions (%d règles sélectionnées)\n", selected_in_all);
        }

        free_rule_info_list(tailoring_all, tailoring_all_count);
    } else {
        printf("(profil tailoring non fourni, test sauté)\n");
    }

    // --- 6. Test d'un profil INEXISTANT : doit retourner -1 proprement, sans crash ---
    struct rule_list *bogus = NULL;
    int bogus_count = selected_rules_for_profile(benchmark_id, "ce_profil_n_existe_pas", &bogus);
    printf("Test profil inexistant : count=%d (attendu -1)\n", bogus_count);
    if (bogus_count >= 0) {
        free_rule_info_list(bogus, bogus_count);
    }

    printf("\nTous les tests ont tourné sans crash. Vérifiez le rapport valgrind ci-dessus.\n");
    return 0;
}