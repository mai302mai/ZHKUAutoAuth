#ifndef AUTOAUTH_USER_CONFIG_H
#define AUTOAUTH_USER_CONFIG_H

struct user_config {
    char *account;
    char *password;
#if defined(__linux__)
    char *interface_name;
#endif
};

struct user_config *user_config_load(const char *path);
void user_config_destroy(struct user_config *config);

#endif
