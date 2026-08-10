#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "scap_service.h"

char *profiles_and_tailoring_to_json(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count);

char *rules_to_json(struct rule_list *rules, int count);

#endif