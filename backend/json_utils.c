// json_utils.c
#include <cjson/cJSON.h>
#include "json_utils.h"

char *profiles_to_json(struct profile_list *profiles, int count) {
    cJSON *root = cJSON_CreateArray();
    if (root == NULL) return NULL;

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (item == NULL) {
            cJSON_Delete(root);
            return NULL;
        }

        cJSON_AddStringToObject(item, "id", profiles[i].id ? profiles[i].id : "");
        cJSON_AddStringToObject(item, "title", profiles[i].title ? profiles[i].title : "");

        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}