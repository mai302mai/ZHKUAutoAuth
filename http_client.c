#include "http_client.h"

#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct response_buffer {
    char *data;
    size_t size;
};

static size_t append_response(char *data, size_t size, size_t count,
                              void *user_data)
{
    struct response_buffer *response = user_data;
    size_t bytes;
    char *new_data;

    if (size != 0 && count > SIZE_MAX / size) {
        return 0;
    }
    bytes = size * count;
    if (bytes > SIZE_MAX - response->size - 1) {
        return 0;
    }

    new_data = realloc(response->data, response->size + bytes + 1);
    if (new_data == NULL) {
        return 0;
    }

    response->data = new_data;
    memcpy(response->data + response->size, data, bytes);
    response->size += bytes;
    response->data[response->size] = '\0';
    return bytes;
}

int http_client_init(void)
{
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);

    if (result != CURLE_OK) {
        fprintf(stderr, "libcurl initialization failed: %s\n",
                curl_easy_strerror(result));
        return 0;
    }
    return 1;
}

void http_client_cleanup(void)
{
    curl_global_cleanup();
}

static char *http_get_internal(const char *url, size_t *response_size,
                               int disable_proxy,
                               const char *interface_name)
{
    CURL *curl = curl_easy_init();
    struct response_buffer response = {NULL, 0};
    CURLcode result;

    if (curl == NULL) {
        fprintf(stderr, "Could not create a libcurl handle.\n");
        return NULL;
    }

    result = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (result == CURLE_OK) {
        result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_response);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    }
    if (result == CURLE_OK && disable_proxy) {
        result = curl_easy_setopt(curl, CURLOPT_PROXY, "");
    }
#if defined(__linux__)
    if (result == CURLE_OK && interface_name != NULL) {
        result = curl_easy_setopt(curl, CURLOPT_INTERFACE, interface_name);
    }
#else
    (void)interface_name;
#endif
    if (result == CURLE_OK) {
        result = curl_easy_perform(curl);
    }

    if (result != CURLE_OK) {
#if defined(__linux__) && defined(AUTOAUTH_ENABLE_LINUX_INTERFACE_BINDING)
        fprintf(stderr, "GET request failed%s%s: %s\n",
                interface_name != NULL ? " using interface " : "",
                interface_name != NULL ? interface_name : "",
                curl_easy_strerror(result));
#else
        fprintf(stderr, "GET request failed: %s\n", curl_easy_strerror(result));
#endif
        free(response.data);
        response.data = NULL;
    } else if (response.data == NULL) {
        response.data = malloc(1);
        if (response.data == NULL) {
            fprintf(stderr, "Could not allocate memory for the response.\n");
        } else {
            response.data[0] = '\0';
        }
    }

    curl_easy_cleanup(curl);
    if (response.data != NULL && response_size != NULL) {
        *response_size = response.size;
    }
    return response.data;
}

#if defined(_WIN32)
char *http_get(const char *url, size_t *response_size)
{
    return http_get_internal(url, response_size, 0, NULL);
}

char *http_get_direct(const char *url, size_t *response_size)
{
    return http_get_internal(url, response_size, 1, NULL);
}
#elif defined(__linux__)
#if defined(AUTOAUTH_ENABLE_LINUX_INTERFACE_BINDING)
char *http_get(const char *url, const char *interface_name,
               size_t *response_size)
{
    return http_get_internal(url, response_size, 0, interface_name);
}

char *http_get_direct(const char *url, const char *interface_name,
                      size_t *response_size)
{
    return http_get_internal(url, response_size, 1, interface_name);
}
#else
char *http_get(const char *url, size_t *response_size)
{
    return http_get_internal(url, response_size, 0, NULL);
}

char *http_get_direct(const char *url, const char *interface_name,
                      size_t *response_size)
{
    return http_get_internal(url, response_size, 1, interface_name);
}
#endif
#endif
