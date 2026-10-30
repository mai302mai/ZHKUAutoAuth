#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "network_auth.h"
#include "user_config.h"

static int authenticate_from_config(const char *path)
{
    struct user_config *config = user_config_load(path);
    int succeeded;

    if (config == NULL) {
        return EXIT_FAILURE;
    }

#if defined(__linux__)
    succeeded = network_authenticate(config->account, config->password,
                                     config->interface_name);
#else
    succeeded = network_authenticate(config->account, config->password);
#endif
    user_config_destroy(config);
    return succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv)
{
#if defined(_WIN32)
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD console_mode;

    if (stdout_handle != INVALID_HANDLE_VALUE &&
        GetConsoleMode(stdout_handle, &console_mode)) {
        (void)SetConsoleOutputCP(CP_UTF8);
    }
#endif

    if (argc == 3 &&
        (strcmp(argv[1], "--config") == 0 || strcmp(argv[1], "-c") == 0)) {
        return authenticate_from_config(argv[2]);
    }

#if defined(__linux__)
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s --config <path>\n"
                "       %s <account> <password> <interface-name>\n",
                argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    return network_authenticate(argv[1], argv[2], argv[3])
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
#else
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s --config <path>\n"
                "       %s <account> <password>\n",
                argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    return network_authenticate(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
