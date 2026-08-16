#ifndef SCAP_SERVICE_H
#define SCAP_SERVICE_H

#include <stdbool.h>




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
    struct ds_sds_session *ds_sds_session;
    struct xccdf_benchmark *benchmark;       // possédé par policy_model après création
    struct xccdf_policy_model *policy_model;
    struct oscap_source *tailoring_source;   // NULL si aucun tailoring utilisé
    struct xccdf_profile *profile;           // profil trouvé (natif ou tailoring)
};

const char *get_rule_title(struct xccdf_rule *rule);
const char *get_rule_question(struct xccdf_rule *rule);


int list_profiles_for_distro(const char *id,struct profile_list **out_profiles, int *out_profiles_count,struct profile_list **out_tailoring_profiles, int *out_tailoring_count);
void free_profile_list(struct profile_list *profiles, int count);
void free_profiles_for_distro(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count);

int list_rules_for_ds(const char *ds_path,struct rule_list **out_rules);
void free_rule_info_list(struct rule_list *rules,int count);

// benchmark_id (pas ds_path) : ces fonctions résolvent en interne si le profil
// vient du benchmark natif ou du tailoring associé, aucune indication à fournir
int profile_selected_rules(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules);
int profile_all_rules(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules);


int create_tailoring_profile(const char *benchmark_id, const char *name, const char *description,
                              const char *base_profile_id, // NULL = from-scratch
                              const char **added_ids, int added_count,
                              const char **removed_ids, int removed_count,
                              char *out_new_id, size_t out_id_size);

#endif