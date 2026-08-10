#ifndef SCAP_SERVICE_H
#define SCAP_SERVICE_H

#include <stdbool.h>

struct profile_list{
    char *id;
    char *title;
    char *description;
    char *extends;
};

struct rule_list{
    char *id;
    char *title;
    char *description;
    char *rationale;
    char *severity;
    bool selected;
};


int list_profiles_for_distro(const char *id,struct profile_list **out_profiles, int *out_profiles_count,struct profile_list **out_tailoring_profiles, int *out_tailoring_count);
void free_profile_list(struct profile_list *profiles, int count);
void free_profiles_for_distro(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count);

int list_rules_for_ds(const char *ds_path,struct rule_list **out_rules);
void free_rule_info_list(struct rule_list *rules,int count);

// benchmark_id (pas ds_path) : ces fonctions résolvent en interne si le profil
// vient du benchmark natif ou du tailoring associé, aucune indication à fournir
int selected_rules_for_profile(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules);
int all_rules_with_selection_for_profile(const char *benchmark_id, const char *profile_id, struct rule_list **out_rules);

#endif