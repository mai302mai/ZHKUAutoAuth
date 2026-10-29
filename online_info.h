#ifndef AUTOAUTH_ONLINE_INFO_H
#define AUTOAUTH_ONLINE_INFO_H

#include <stddef.h>

struct online_info {
    char *online_time;
    char *online_ip;
    char *uplink_bytes;
    char *downlink_bytes;
};

struct online_info *online_info_parse(const char *json, size_t json_size);
void online_info_destroy(struct online_info *info);

#endif
