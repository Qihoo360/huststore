#ifndef __hustdb_network_utils_20160314135056_h__
#define __hustdb_network_utils_20160314135056_h__

#include "libevhtp_utils.h"
#include "hustdb_utils.h"
#include "hustdb.h"

namespace hustdb_network {

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

struct hustdb_network_ctx_t
{
    evhtp::conf_t base;
    hustdb_network::ip_allow_t * ip_allow_map;
    hustdb_t * db;
};

namespace hustdb_network {

void add_version(uint32_t ver, evhtp_request_t * request);
void add_ver_err(item_ctxt_t * ctxt, evhtp_request_t * request);
void add_ver_err(bool is_version_error, evhtp_request_t * request);
void send_write_reply(evhtp_res code, uint32_t ver, item_ctxt_t * ctxt, evhtp_request_t * request);
void send_write_reply(evhtp_res code, uint32_t ver, bool is_version_error, evhtp_request_t * request);
bool get_ip_allow_map(const char * ip_allow_string, unsigned int ip_allow_string_length, ip_allow_t * ip_allow_map);
bool can_access (struct sockaddr_in * addr, ip_allow_t * ip_allow_map);

int compress(evhtp::c_str_t src, evhtp::c_str_t * dst);
int decompress(evhtp::c_str_t src, evhtp::c_str_t * dst);

}

#endif // __hustdb_network_utils_20160314135056_h__
