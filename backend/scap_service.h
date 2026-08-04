#ifndef SCAP_SERVICE_H
#define SCAP_SERVICE_H

struct profile_list{
    char *id;
    char *title;
};

int list_profiles_for_ds(const char *ds_path,struct profile_list **out_profiles);
void free_profile_list(struct profile_list *profiles, int count);

#endif