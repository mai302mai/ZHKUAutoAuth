#include "online_info.h"

#include <cJSON.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string_field(const cJSON *object, const char *name)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    size_t length;
    char *copy;

    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        fprintf(stderr, "Online information is missing string field \"%s\".\n",
                name);
        return NULL;
    }

    length = strlen(item->valuestring);
    if (length == SIZE_MAX) {
        fprintf(stderr, "Online information field \"%s\" is too large.\n", name);
        return NULL;
    }

    copy = malloc(length + 1);
    if (copy == NULL) {
        fprintf(stderr, "Could not allocate memory for field \"%s\".\n", name);
        return NULL;
    }
    memcpy(copy, item->valuestring, length + 1);
    return copy;
}

void online_info_destroy(struct online_info *info)
{
    if (info == NULL) {
        return;
    }

    free(info->online_time);
    free(info->online_ip);
    free(info->uplink_bytes);
    free(info->downlink_bytes);
    free(info);
}

struct online_info *online_info_parse(const char *json, size_t json_size)
{
    char *json_copy;
    cJSON *root;
    const cJSON *result;
    const cJSON *list;
    const cJSON *first_item;
    struct online_info *info;

    if (json == NULL || json_size == 0 || json_size == SIZE_MAX ||
        memchr(json, '\0', json_size) != NULL) {
        fprintf(stderr, "Online information response is empty or invalid.\n");
        return NULL;
    }

    json_copy = malloc(json_size + 1);
    if (json_copy == NULL) {
        fprintf(stderr, "Could not allocate memory to parse online information.\n");
        return NULL;
    }
    memcpy(json_copy, json, json_size);
    json_copy[json_size] = '\0';

    root = cJSON_ParseWithLengthOpts(json_copy, json_size + 1, NULL, 1);
    free(json_copy);
    if (!cJSON_IsObject(root)) {
        fprintf(stderr, "Could not parse online information JSON object.\n");
        cJSON_Delete(root);
        return NULL;
    }

    result = cJSON_GetObjectItemCaseSensitive(root, "result");
    list = cJSON_GetObjectItemCaseSensitive(root, "list");
    first_item = cJSON_GetArrayItem(list, 0);
    if (!cJSON_IsNumber(result) || result->valuedouble != 1.0 ||
        !cJSON_IsArray(list) || !cJSON_IsObject(first_item)) {
        fprintf(stderr, "Online information JSON has no successful list item.\n");
        cJSON_Delete(root);
        return NULL;
    }

    info = calloc(1, sizeof(*info));
    if (info == NULL) {
        fprintf(stderr, "Could not allocate online information structure.\n");
        cJSON_Delete(root);
        return NULL;
    }

    info->online_time = copy_string_field(first_item, "online_time");
    info->online_ip = copy_string_field(first_item, "online_ip");
    info->uplink_bytes = copy_string_field(first_item, "uplink_bytes");
    info->downlink_bytes = copy_string_field(first_item, "downlink_bytes");
    cJSON_Delete(root);

    if (info->online_time == NULL || info->online_ip == NULL ||
        info->uplink_bytes == NULL || info->downlink_bytes == NULL) {
        online_info_destroy(info);
        return NULL;
    }

    return info;
}
