#ifndef __sync_network_utils_20160531151242_h__
#define __sync_network_utils_20160531151242_h__

#include "libevhtp_utils.h"

namespace sync_network {

typedef struct
{
    unsigned int start;
    unsigned int end;
} ip_t;

typedef struct
{
    int size;
    ip_t ip_map [ 1024 ];
} ip_allow_t;

}
struct sync_network_ctx_t
{
    evhtp::conf_t base;
    sync_network::ip_allow_t * ip_allow_map;
};

namespace sync_network {

bool get_ip_allow_map(const char * ip_allow_string, unsigned int ip_allow_string_length, ip_allow_t * ip_allow_map);
bool can_access (struct sockaddr_in * addr, ip_allow_t * ip_allow_map);

}
#endif // __sync_network_utils_20160531151242_h__
