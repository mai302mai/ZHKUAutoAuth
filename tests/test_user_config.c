#include "user_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_CONFIG_PATH "test_user_config.json"

static int write_config(const char *json)
{
    FILE *file = fopen(TEST_CONFIG_PATH, "wb");
    size_t json_size = strlen(json);
    int succeeded;

    if (file == NULL) {
        perror("Could not create test configuration file");
        return 0;
    }

    succeeded = fwrite(json, 1, json_size, file) == json_size;
    if (fclose(file) != 0) {
        perror("Could not close test configuration file");
        succeeded = 0;
    }
    return succeeded;
}

static int test_loads_valid_config(void)
{
    static const char json[] =
        "{\"account\":\"student123\",\"password\":\"test-password\","
        "\"interface\":\"eth0\"}";
    struct user_config *config;
    int passed;

    if (!write_config(json)) {
        return 0;
    }

    config = user_config_load(TEST_CONFIG_PATH);
    if (config == NULL) {
        remove(TEST_CONFIG_PATH);
        return 0;
    }

    passed = strcmp(config->account, "student123") == 0 &&
             strcmp(config->password, "test-password") == 0;
#if defined(__linux__)
    passed = passed && strcmp(config->interface_name, "eth0") == 0;
#endif
    user_config_destroy(config);
    if (remove(TEST_CONFIG_PATH) != 0) {
        perror("Could not remove test configuration file");
        passed = 0;
    }
    return passed;
}

static int test_rejects_missing_required_fields(void)
{
    static const char json[] = "{\"account\":\"student123\"}";
    struct user_config *config;

    if (!write_config(json)) {
        return 0;
    }
    config = user_config_load(TEST_CONFIG_PATH);
    remove(TEST_CONFIG_PATH);
    if (config != NULL) {
        user_config_destroy(config);
        return 0;
    }
    return 1;
}

static int test_requires_interface_on_linux(void)
{
#if defined(__linux__)
    static const char json[] =
        "{\"account\":\"student123\",\"password\":\"test-password\"}";
    struct user_config *config;

    if (!write_config(json)) {
        return 0;
    }
    config = user_config_load(TEST_CONFIG_PATH);
    remove(TEST_CONFIG_PATH);
    if (config != NULL) {
        user_config_destroy(config);
        return 0;
    }
#endif
    return 1;
}

static int test_rejects_empty_account(void)
{
    static const char json[] =
        "{\"account\":\"\",\"password\":\"test-password\","
        "\"interface\":\"eth0\"}";
    struct user_config *config;

    if (!write_config(json)) {
        return 0;
    }
    config = user_config_load(TEST_CONFIG_PATH);
    remove(TEST_CONFIG_PATH);
    if (config != NULL) {
        user_config_destroy(config);
        return 0;
    }
    return 1;
}

static int test_rejects_invalid_json(void)
{
    static const char json[] = "{\"account\":";
    struct user_config *config;

    if (!write_config(json)) {
        return 0;
    }
    config = user_config_load(TEST_CONFIG_PATH);
    remove(TEST_CONFIG_PATH);
    if (config != NULL) {
        user_config_destroy(config);
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_loads_valid_config() ||
        !test_rejects_missing_required_fields() ||
        !test_requires_interface_on_linux() ||
        !test_rejects_empty_account() ||
        !test_rejects_invalid_json()) {
        fputs("User configuration tests failed.\n", stderr);
        return EXIT_FAILURE;
    }

    puts("User configuration tests passed.");
    return EXIT_SUCCESS;
}
