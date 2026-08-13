// json_utils.c
#include <cjson/cJSON.h>
#include "json_utils.h"

static void append_profiles_array(cJSON *array, struct profile_list *profiles, int count) {
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "id", profiles[i].id ? profiles[i].id : "");
        cJSON_AddStringToObject(item, "title", profiles[i].title ? profiles[i].title : "");
        cJSON_AddStringToObject(item, "description", profiles[i].description ? profiles[i].description : "");
        if (profiles[i].extends != NULL) {
            cJSON_AddStringToObject(item, "extends", profiles[i].extends);
        }
        cJSON_AddItemToArray(array, item);
    }
}

char *profiles_and_tailoring_to_json(struct profile_list *profiles, int profiles_count,struct profile_list *tailoring_profiles, int tailoring_count) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return NULL;

    cJSON *profiles_array = cJSON_CreateArray();
    if (profiles_array == NULL) { cJSON_Delete(root); return NULL; }
    append_profiles_array(profiles_array, profiles, profiles_count);
    cJSON_AddItemToObject(root, "profiles", profiles_array);

    cJSON *tailoring_array = cJSON_CreateArray();
    if (tailoring_array == NULL) { cJSON_Delete(root); return NULL; }
    append_profiles_array(tailoring_array, tailoring_profiles, tailoring_count);
    cJSON_AddItemToObject(root, "tailoring_profiles", tailoring_array);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    return json_str;
}

static void append_references_array(cJSON *item, struct rule_reference *refs, int count){
    cJSON *array = cJSON_CreateArray();
    for(int i=0;i<count;i++){
        cJSON *ref_item = cJSON_CreateObject();
        cJSON_AddStringToObject(ref_item, "href", refs[i].href ? refs[i].href : "");
        cJSON_AddStringToObject(ref_item, "text", refs[i].text ? refs[i].text : "");
        cJSON_AddItemToArray(array, ref_item);
    }
    cJSON_AddItemToObject(item, "references", array);
}

static void append_fixes_array(cJSON *item, struct rule_fix *fixes, int count){
    cJSON *array = cJSON_CreateArray();
    for(int i=0;i<count;i++){
        cJSON *fix_item = cJSON_CreateObject();
        cJSON_AddStringToObject(fix_item, "system", fixes[i].system ? fixes[i].system : "");
        cJSON_AddStringToObject(fix_item, "content", fixes[i].content ? fixes[i].content : "");
        cJSON_AddItemToArray(array, fix_item);
    }
    cJSON_AddItemToObject(item, "fixes", array);
}

char *rules_to_json(struct rule_list *rules, int count) {
    cJSON *root = cJSON_CreateArray();
    if (root == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (item == NULL) { cJSON_Delete(root); return NULL; }

        cJSON_AddStringToObject(item, "id", rules[i].id ? rules[i].id : "");
        cJSON_AddStringToObject(item, "title", rules[i].title ? rules[i].title : "");
        cJSON_AddStringToObject(item, "description", rules[i].description ? rules[i].description : "");
        cJSON_AddStringToObject(item, "rationale", rules[i].rationale ? rules[i].rationale : "");
        cJSON_AddStringToObject(item, "severity", rules[i].severity ? rules[i].severity : "");
        cJSON_AddBoolToObject(item, "selected", rules[i].selected);
        cJSON_AddStringToObject(item, "question", rules[i].question ? rules[i].question : "");
        append_references_array(item, rules[i].references, rules[i].references_count);
        append_fixes_array(item, rules[i].fixes, rules[i].fixes_count);

        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    return json_str;
}