#include "user_config.h"

#include <cJSON.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_required_string(const cJSON *object, const char *name)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    size_t length;
    char *copy;

    if (!cJSON_IsString(item) || item->valuestring == NULL ||
        item->valuestring[0] == '\0') {
        fprintf(stderr, "Configuration is missing non-empty string field "
                        "\"%s\".\n",
                name);
        return NULL;
    }

    length = strlen(item->valuestring);
    if (length == SIZE_MAX) {
        fprintf(stderr, "Configuration field \"%s\" is too large.\n", name);
        return NULL;
    }

    copy = malloc(length + 1);
    if (copy == NULL) {
        fprintf(stderr, "Could not allocate memory for configuration field "
                        "\"%s\".\n",
                name);
        return NULL;
    }
    memcpy(copy, item->valuestring, length + 1);
    return copy;
}

void user_config_destroy(struct user_config *config)
{
    if (config == NULL) {
        return;
    }

    free(config->account);
    free(config->password);
#if defined(__linux__)
    free(config->interface_name);
#endif
    free(config);
}

static struct user_config *parse_config(const char *data, size_t data_size)
{
    cJSON *root;
    struct user_config *config;

    if (data_size == 0 || data_size == SIZE_MAX ||
        memchr(data, '\0', data_size) != NULL) {
        fprintf(stderr, "Configuration file is empty or contains invalid data.\n");
        return NULL;
    }

    root = cJSON_ParseWithLengthOpts(data, data_size + 1, NULL, 1);
    if (!cJSON_IsObject(root)) {
        fprintf(stderr, "Could not parse configuration JSON object.\n");
        cJSON_Delete(root);
        return NULL;
    }

    config = calloc(1, sizeof(*config));
    if (config == NULL) {
        fprintf(stderr, "Could not allocate user configuration.\n");
        cJSON_Delete(root);
        return NULL;
    }

    config->account = copy_required_string(root, "account");
    config->password = copy_required_string(root, "password");
#if defined(__linux__)
    config->interface_name = copy_required_string(root, "interface");
#endif
    cJSON_Delete(root);

    if (config->account == NULL || config->password == NULL
#if defined(__linux__)
        || config->interface_name == NULL
#endif
    ) {
        user_config_destroy(config);
        return NULL;
    }

    return config;
}

struct user_config *user_config_load(const char *path)
{
    FILE *file;
    long file_length;
    size_t data_size;
    char *data;
    size_t bytes_read;
    struct user_config *config;

    if (path == NULL || path[0] == '\0') {
        fprintf(stderr, "A configuration file path is required.\n");
        return NULL;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        perror("Could not open configuration file");
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Could not seek in configuration file");
        fclose(file);
        return NULL;
    }
    file_length = ftell(file);
    if (file_length < 0) {
        perror("Could not determine configuration file size");
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("Could not rewind configuration file");
        fclose(file);
        return NULL;
    }

    if (file_length == 0 || (uintmax_t)file_length > SIZE_MAX - 1) {
        fprintf(stderr, "Configuration file is empty or too large.\n");
        fclose(file);
        return NULL;
    }
    data_size = (size_t)file_length;
    data = malloc(data_size + 1);
    if (data == NULL) {
        fprintf(stderr, "Could not allocate memory for configuration file.\n");
        fclose(file);
        return NULL;
    }

    bytes_read = fread(data, 1, data_size, file);
    if (bytes_read != data_size) {
        if (ferror(file)) {
            perror("Could not read configuration file");
        } else {
            fprintf(stderr, "Configuration file changed while being read.\n");
        }
        free(data);
        fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        perror("Could not close configuration file");
        free(data);
        return NULL;
    }
    data[data_size] = '\0';

    config = parse_config(data, data_size);
    free(data);
    return config;
}
