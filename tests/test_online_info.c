#include "online_info.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_parses_online_info(void)
{
    static const char json[] =
        "{\"result\":1,\"msg\":\"获取用户在线信息成功！\",\"list\":[{"
        "\"online_time\":\"2026-10-22 23:25:51\","
        "\"online_ip\":\"10.46.3.47\","
        "\"uplink_bytes\":\"2410642\","
        "\"downlink_bytes\":\"22769333\"}],\"total\":1}";
    struct online_info *info = online_info_parse(json, sizeof(json) - 1);
    int passed;

    if (info == NULL) {
        return 0;
    }

    passed = strcmp(info->online_time, "2026-10-22 23:25:51") == 0 &&
             strcmp(info->online_ip, "10.46.3.47") == 0 &&
             strcmp(info->uplink_bytes, "2410642") == 0 &&
             strcmp(info->downlink_bytes, "22769333") == 0;
    online_info_destroy(info);
    return passed;
}

static int test_rejects_missing_field(void)
{
    static const char json[] =
        "{\"result\":1,\"list\":[{\"online_time\":\"now\","
        "\"online_ip\":\"127.0.0.1\",\"uplink_bytes\":\"1\"}]}";
    struct online_info *info = online_info_parse(json, sizeof(json) - 1);

    if (info != NULL) {
        online_info_destroy(info);
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_parses_online_info() || !test_rejects_missing_field()) {
        fputs("Online info parser test failed.\n", stderr);
        return EXIT_FAILURE;
    }

    puts("Online info parser tests passed.");
    return EXIT_SUCCESS;
}
