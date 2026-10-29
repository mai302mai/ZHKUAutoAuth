#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "network_auth.h"

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

#if defined(__linux__)
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <account> <password> <interface-name>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    return network_authenticate(argv[1], argv[2], argv[3])
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
#else
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <account> <password>\n", argv[0]);
        return EXIT_FAILURE;
    }

    return network_authenticate(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
}
