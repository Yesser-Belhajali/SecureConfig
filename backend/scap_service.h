#ifndef SCAP_SERVICE_H
#define SCAP_SERVICE_H

struct profile_list{
    char *id;
    char *title;
};

struct rule_list{
    char *id;
    char *title;
    char *description;
    char *rationale;
    char *severity;
};

int list_profiles_for_ds(const char *ds_path,struct profile_list **out_profiles);
void free_profile_list(struct profile_list *profiles, int count);

int list_rules_for_ds(const char *ds_path,struct rule_list **out_rules);
void free_rule_info_list(struct rule_list *rules,int count);


int selected_rules_for_profile(const char *ds_path,const char *profile_id,struct rule_list **out_rules);

#endif