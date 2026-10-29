#ifndef AUTOAUTH_HTTP_CLIENT_H
#define AUTOAUTH_HTTP_CLIENT_H

#include <stddef.h>

int http_client_init(void);
void http_client_cleanup(void);

#if defined(_WIN32)
char *http_get(const char *url, size_t *response_size);
char *http_get_direct(const char *url, size_t *response_size);
#elif defined(__linux__)
char *http_get_direct(const char *url, const char *interface_name,
                      size_t *response_size);
#if defined(AUTOAUTH_ENABLE_LINUX_INTERFACE_BINDING)
char *http_get(const char *url, const char *interface_name,
               size_t *response_size);
#else
char *http_get(const char *url, size_t *response_size);
#endif
#else
#error "AutoAuth supports Windows and Linux."
#endif

#endif
