#if defined(__linux__) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE
#endif

#include "network_auth.h"

#include <cJSON.h>
#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_client.h"
#include "online_info.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define PORTAL_HOST "10.255.254.254"
#define ONLINE_LIST_URL "http://" PORTAL_HOST ":801/eportal/portal/online_list"
#define LOGIN_URL_FORMAT                                                   \
    "http://" PORTAL_HOST                                                  \
    ":801/eportal/portal/login?callback=dr1003&login_method=1"             \
    "&user_account=%s&user_password=%s&wlan_user_ip="                     \
    "&wlan_user_ipv6=&wlan_user_mac=000000000000&wlan_ac_ip=%s"            \
    "&wlan_ac_name=&jsVersion=4.2&terminal_type=1&lang=zh-cn&v=6656&lang=zh"

static cJSON *parse_json_or_jsonp(const char *response, size_t response_size)
{
    const char *begin;
    const char *end;
    const char *open_paren;
    const char *close_paren;
    const char *suffix;
    size_t payload_size;
    char *payload;
    cJSON *json;

    if (response == NULL || response_size == 0 ||
        memchr(response, '\0', response_size) != NULL) {
        fprintf(stderr, "Portal returned an empty or invalid response.\n");
        return NULL;
    }

    begin = response;
    end = response + response_size;
    while (begin < end && (*begin == ' ' || *begin == '\t' ||
                           *begin == '\r' || *begin == '\n')) {
        ++begin;
    }
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t' ||
                           end[-1] == '\r' || end[-1] == '\n')) {
        --end;
    }

    if (begin < end && *begin != '{') {
        open_paren = memchr(begin, '(', (size_t)(end - begin));
        if (open_paren == NULL) {
            fprintf(stderr, "Portal returned invalid JSONP data.\n");
            return NULL;
        }

        close_paren = end;
        while (close_paren > open_paren + 1 && close_paren[-1] != ')') {
            --close_paren;
        }
        if (close_paren <= open_paren + 1) {
            fprintf(stderr, "Portal returned invalid JSONP data.\n");
            return NULL;
        }
        --close_paren;
        suffix = close_paren + 1;
        if (suffix < end && *suffix == ';') {
            ++suffix;
        }
        if (suffix != end) {
            fprintf(stderr, "Portal returned invalid JSONP data.\n");
            return NULL;
        }
        begin = open_paren + 1;
        end = close_paren;
    }

    payload_size = (size_t)(end - begin);
    if (payload_size == SIZE_MAX) {
        fprintf(stderr, "Portal response is too large.\n");
        return NULL;
    }
    payload = malloc(payload_size + 1);
    if (payload == NULL) {
        fprintf(stderr, "Could not allocate memory to parse portal response.\n");
        return NULL;
    }
    memcpy(payload, begin, payload_size);
    payload[payload_size] = '\0';
    json = cJSON_ParseWithLengthOpts(payload, payload_size + 1, NULL, 1);
    free(payload);
    if (!cJSON_IsObject(json)) {
        fprintf(stderr, "Portal response does not contain a valid JSON object.\n");
        cJSON_Delete(json);
        return NULL;
    }
    return json;
}

static char *copy_local_ipv4(const char *interface_name)
{
#if defined(__linux__)
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *entry;
    char address[INET_ADDRSTRLEN];
    char *result = NULL;

    if (getifaddrs(&interfaces) != 0) {
        perror("Could not enumerate network interfaces");
        return NULL;
    }

    for (entry = interfaces; entry != NULL; entry = entry->ifa_next) {
        const struct sockaddr_in *ipv4;
        if (entry->ifa_addr == NULL || entry->ifa_addr->sa_family != AF_INET ||
            entry->ifa_name == NULL ||
            strcmp(entry->ifa_name, interface_name) != 0 ||
            (entry->ifa_flags & IFF_UP) == 0) {
            continue;
        }

        ipv4 = (const struct sockaddr_in *)entry->ifa_addr;
        if (inet_ntop(AF_INET, &ipv4->sin_addr, address, sizeof(address)) ==
            NULL) {
            perror("Could not format interface IPv4 address");
            break;
        }
        result = malloc(strlen(address) + 1);
        if (result == NULL) {
            fprintf(stderr, "Could not allocate memory for interface address.\n");
            break;
        }
        strcpy(result, address);
        break;
    }

    freeifaddrs(interfaces);
    if (result == NULL) {
        fprintf(stderr, "No active IPv4 address found on interface \"%s\".\n",
                interface_name);
    }
    return result;
#elif defined(_WIN32)
    SOCKET socket_handle;
    struct sockaddr_in destination;
    struct sockaddr_in local_address;
    int local_address_size = sizeof(local_address);
    char address[INET_ADDRSTRLEN];
    char *result;

    (void)interface_name;
    socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_handle == INVALID_SOCKET) {
        fprintf(stderr, "Could not create route lookup socket (error %d).\n",
                WSAGetLastError());
        return NULL;
    }

    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_port = htons(801);
    if (InetPtonA(AF_INET, PORTAL_HOST, &destination.sin_addr) != 1 ||
        connect(socket_handle, (const struct sockaddr *)&destination,
                sizeof(destination)) == SOCKET_ERROR ||
        getsockname(socket_handle, (struct sockaddr *)&local_address,
                    &local_address_size) == SOCKET_ERROR) {
        fprintf(stderr, "Could not determine the local route to the portal "
                        "(error %d).\n",
                WSAGetLastError());
        closesocket(socket_handle);
        return NULL;
    }
    closesocket(socket_handle);

    if (InetNtopA(AF_INET, &local_address.sin_addr, address,
                  (DWORD)sizeof(address)) == NULL) {
        fprintf(stderr, "Could not format local IPv4 address (error %d).\n",
                WSAGetLastError());
        return NULL;
    }
    result = malloc(strlen(address) + 1);
    if (result == NULL) {
        fprintf(stderr, "Could not allocate memory for local IPv4 address.\n");
        return NULL;
    }
    strcpy(result, address);
    return result;
#endif
}

static char *url_encode(const char *value)
{
    CURL *curl = curl_easy_init();
    char *encoded;
    char *copy;

    if (curl == NULL) {
        fprintf(stderr, "Could not create a URL encoder.\n");
        return NULL;
    }
    encoded = curl_easy_escape(curl, value, 0);
    curl_easy_cleanup(curl);
    if (encoded == NULL) {
        fprintf(stderr, "Could not encode authentication parameters.\n");
        return NULL;
    }
    copy = malloc(strlen(encoded) + 1);
    if (copy == NULL) {
        fprintf(stderr, "Could not allocate encoded authentication parameter.\n");
    } else {
        strcpy(copy, encoded);
    }
    curl_free(encoded);
    return copy;
}

static char *build_login_url(const char *account, const char *password,
                             const char *local_ip)
{
    char *encoded_account = url_encode(account);
    char *encoded_password = url_encode(password);
    char *encoded_ip = url_encode(local_ip);
    char *url = NULL;
    int url_size;

    if (encoded_account == NULL || encoded_password == NULL ||
        encoded_ip == NULL) {
        goto cleanup;
    }

    url_size = snprintf(NULL, 0, LOGIN_URL_FORMAT, encoded_account,
                        encoded_password, encoded_ip);
    if (url_size < 0) {
        fprintf(stderr, "Could not construct the authentication URL.\n");
        goto cleanup;
    }
    url = malloc((size_t)url_size + 1);
    if (url == NULL) {
        fprintf(stderr, "Could not allocate the authentication URL.\n");
        goto cleanup;
    }
    (void)snprintf(url, (size_t)url_size + 1, LOGIN_URL_FORMAT, encoded_account,
                   encoded_password, encoded_ip);

cleanup:
    free(encoded_account);
    free(encoded_password);
    free(encoded_ip);
    return url;
}

static char *portal_get(const char *url, size_t *response_size,
                        const char *interface_name)
{
#if defined(__linux__)
    return http_get_direct(url, interface_name, response_size);
#else
    (void)interface_name;
    return http_get_direct(url, response_size);
#endif
}

static int print_online_info(const cJSON *json)
{
    char *normalized_json = cJSON_PrintUnformatted(json);
    struct online_info *info;

    if (normalized_json == NULL) {
        fprintf(stderr, "Could not serialize online information.\n");
        return 0;
    }
    info = online_info_parse(normalized_json, strlen(normalized_json));
    cJSON_free(normalized_json);
    if (info == NULL) {
        return 0;
    }

    if (printf("用户已在线：\n"
               "online_time: %s\n"
               "online_ip: %s\n"
               "uplink_bytes: %s\n"
               "downlink_bytes: %s\n",
               info->online_time, info->online_ip, info->uplink_bytes,
               info->downlink_bytes) < 0) {
        fprintf(stderr, "Could not write online information to stdout.\n");
        online_info_destroy(info);
        return 0;
    }
    online_info_destroy(info);
    return 1;
}

static int authenticate(const char *account, const char *password,
                        const char *interface_name)
{
    char *response;
    char *local_ip;
    char *login_url;
    size_t response_size = 0;
    cJSON *json;
    const cJSON *result;
    const cJSON *list;
    const cJSON *ret_code;
    int succeeded = 0;

    response = portal_get(ONLINE_LIST_URL, &response_size, interface_name);
    if (response == NULL) {
        return 0;
    }
    json = parse_json_or_jsonp(response, response_size);
    free(response);
    if (json == NULL) {
        return 0;
    }

    result = cJSON_GetObjectItemCaseSensitive(json, "result");
    if (!cJSON_IsNumber(result)) {
        fprintf(stderr, "Online-list response is missing numeric result.\n");
        cJSON_Delete(json);
        return 0;
    }

    if (result->valuedouble == 1.0) {
        list = cJSON_GetObjectItemCaseSensitive(json, "list");
        if (!cJSON_IsArray(list) || cJSON_GetArraySize(list) == 0) {
            fprintf(stderr, "Online-list response reports success but has no "
                            "user data.\n");
        } else {
            succeeded = print_online_info(json);
        }
        cJSON_Delete(json);
        return succeeded;
    }
    if (result->valuedouble != 0.0) {
        fprintf(stderr, "Online-list response has an unsupported result.\n");
        cJSON_Delete(json);
        return 0;
    }
    cJSON_Delete(json);

    local_ip = copy_local_ipv4(interface_name);
    if (local_ip == NULL) {
        return 0;
    }
    login_url = build_login_url(account, password, local_ip);
    free(local_ip);
    if (login_url == NULL) {
        return 0;
    }

    response_size = 0;
    response = portal_get(login_url, &response_size, interface_name);
    free(login_url);
    if (response == NULL) {
        return 0;
    }
    json = parse_json_or_jsonp(response, response_size);
    free(response);
    if (json == NULL) {
        return 0;
    }

    result = cJSON_GetObjectItemCaseSensitive(json, "result");
    if (cJSON_IsNumber(result) && result->valuedouble == 1.0) {
        const cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "msg");
        if (cJSON_IsString(message) && message->valuestring != NULL) {
            printf("%s\n", message->valuestring);
        } else {
            puts("Portal协议认证成功！");
        }
        succeeded = 1;
    } else if (cJSON_IsNumber(result) && result->valuedouble == 0.0) {
        const cJSON *message =
            cJSON_GetObjectItemCaseSensitive(json, "msg");
        if (cJSON_IsString(message) && message->valuestring != NULL) {
            printf("认证未完成：%s\n", message->valuestring);
        } else {
            puts("认证未完成：认证服务返回失败。");
        }
        ret_code = cJSON_GetObjectItemCaseSensitive(json, "ret_code");
        succeeded = cJSON_IsNumber(ret_code) && ret_code->valueint == 2;
    } else {
        fprintf(stderr, "Authentication response is missing a valid result.\n");
    }
    cJSON_Delete(json);
    return succeeded;
}

#if defined(__linux__)
int network_authenticate(const char *account, const char *password,
                         const char *interface_name)
{
    int succeeded;

    if (account == NULL || account[0] == '\0' || password == NULL ||
        password[0] == '\0' || interface_name == NULL ||
        interface_name[0] == '\0') {
        fprintf(stderr, "Account, password, and interface name are required.\n");
        return 0;
    }
#else
int network_authenticate(const char *account, const char *password)
{
    int succeeded;

    if (account == NULL || account[0] == '\0' || password == NULL ||
        password[0] == '\0') {
        fprintf(stderr, "Account and password are required.\n");
        return 0;
    }
#endif
    if (!http_client_init()) {
        return 0;
    }

#if defined(__linux__)
    succeeded = authenticate(account, password, interface_name);
#else
    succeeded = authenticate(account, password, NULL);
#endif
    http_client_cleanup();
    return succeeded;
}
