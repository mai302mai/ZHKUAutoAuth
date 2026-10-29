#ifndef AUTOAUTH_NETWORK_AUTH_H
#define AUTOAUTH_NETWORK_AUTH_H

#if defined(__linux__)
int network_authenticate(const char *account, const char *password,
                         const char *interface_name);
#elif defined(_WIN32)
int network_authenticate(const char *account, const char *password);
#else
#error "AutoAuth supports Windows and Linux."
#endif

#endif
