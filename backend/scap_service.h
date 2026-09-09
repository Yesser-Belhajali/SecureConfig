#ifndef SCAP_SERVICE_H
#define SCAP_SERVICE_H

#include <stdbool.h>
#include <stddef.h>




struct profile_list{
    char *id;
    char *title;
    char *description;
    char *extends;
};

struct rule_node{
    struct xccdf_rule *rule;
    struct rule_node *next;
};

struct rule_reference{
    char *href;   // peut être NULL
    char *text;
};

struct rule_fix{
    char *system;   // ex: "urn:xccdf:fix:script:sh", "urn:xccdf:fix:script:ansible"
    char *content;  // le script/contenu de remédiation
};

struct rule_warning{
    char *category;   // traduit depuis xccdf_warning_category_t en string
    char *text;
};

struct rule_check{
    char *system;
    char *selector;   // peut être NULL
    char *content;    // peut être NULL
};

struct rule_list{
    char *id;
    char *title;
    char *description;
    char *rationale;
    char *severity;
    char *question;
    bool selected;

    struct rule_reference *references;
    int references_count;

    struct rule_fix *fixes;
    int fixes_count;

    struct rule_warning *warnings;
    int warnings_count;

    char **platforms;      // simples chaînes CPE, pas de struct dédiée nécessaire
    int platforms_count;

    struct rule_check *checks;
    int checks_count;
};

struct resolved_profile_context {
    struct xccdf_benchmark *benchmark;       // possédé par policy_model après création
    struct xccdf_policy_model *policy_model;
    struct xccdf_profile *profile;           // profil trouvé (natif ou tailoring)
};

const char *get_rule_title(struct xccdf_rule *rule);
const char *get_rule_question(struct xccdf_rule *rule);




// n'accepte que le jeu de caractères attendu pour un segment d'URL déjà
// filtré par sscanf %[^/] côté http_server.c (donc jamais de '/'), en repli
// défensif si ces fonctions sont un jour appelées depuis un autre point
// d'entrée. Bloque aussi toute séquence ".." qui permettrait de sortir du
// dossier data/ via les snprintf de construction de chemin.
bool is_valid_id_component(const char *s);


// charge le benchmark XCCDF depuis un datastream SCAP (.xml). Utilisée par
// plusieurs modules (scap_service.c en interne, remediate.c pour sa propre
// résolution de profil) — rendue publique plutôt que dupliquée, contrairement
// aux petits helpers de formatage comme report_invalid_id
int load_benchmark_from_ds(const char *ds_path, struct xccdf_benchmark **out_benchmark);


// vérifie que chaque rule_ids[i] existe dans benchmark ET que c'est bien une
// XCCDF_RULE (pas un groupe, un profil, ou un idref qui ne correspond à rien).
// Retourne l'index du premier ID invalide, ou -1 si tous sont valides.
int find_invalid_rule_id(struct xccdf_benchmark *benchmark, const char **rule_ids, int count);

// copie l'ID invalide (s'il existe) dans out_invalid_id, en tronquant proprement.
void report_invalid_id(const char *invalid, char *out_invalid_id, size_t out_invalid_id_size);


int list_profiles_for_distro(const char *id,struct profile_list **out_profiles, int *out_profiles_count,struct profile_list **out_tailoring_profiles, int *out_tailoring_count);
void free_profile_list(struct profile_list *profiles, int count);
void free_profiles_for_distro(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count);

int list_rules_for_ds(const char *ds_path,struct rule_list **out_rules);
void free_rule_info_list(struct rule_list *rules,int count);

// benchmark_id (pas ds_path) : ces fonctions résolvent en interne si le profil
// vient du benchmark natif ou du tailoring associé, aucune indication à fournir
int profile_selected_rules(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules);
int profile_all_rules(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules);



// -2 = nom déjà utilisé ; -3 = profil de base introuvable ;
// -4 = profil from-scratch sans aucune règle ajoutée (check rapide) ;
// -5 = profil résolu vide après héritage (check complet) ;
// -6 = extends un profil de base sans aucun added/removed -> duplication
// exacte, refusée pour l'instant ;
// -8 = un ID dans added_ids/removed_ids n'existe pas dans le benchmark (ou
// n'est pas une XCCDF_RULE) — out_invalid_id est rempli avec cet ID dans ce
// cas uniquement, sinon laissé inchangé
int create_tailoring_profile(const char *benchmark_id, const char *name, const char *description,
                             const char *base_profile_id ,
                             const char **added_ids, int added_count,
                             const char **removed_ids, int removed_count,
                             char *out_new_id, size_t out_id_size,
                             char *out_invalid_id, size_t out_invalid_id_size);



// -1 = erreur générique ; -2 = pas de fichier tailoring pour ce benchmark ;
// -3 = profil introuvable dans le tailoring ; -4 = suppression refusée,
// un autre profil du tailoring hérite (extends) de celui-ci
int delete_tailoring_profile(const char *benchmark_id, const char *profile_id);



// -1 = erreur générique ; -2 = pas de fichier tailoring ; -3 = profil introuvable ;
// -5 = le profil édité deviendrait vide ; -6 = un autre profil du tailoring
// deviendrait vide ; -7 = aucune modification demandée (added et removed vides) ;
// -8 = un ID dans added_ids/removed_ids n'existe pas dans le benchmark —
// out_invalid_id est rempli avec cet ID dans ce cas uniquement
int update_tailoring_profile(const char *benchmark_id, const char *profile_id,
                              const char **added_ids, int added_count,
                              const char **removed_ids, int removed_count,
                              char *out_invalid_id, size_t out_invalid_id_size);








#endif