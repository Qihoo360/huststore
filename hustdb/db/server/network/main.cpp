#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <evhtp.h>
#include "hustdb.h"
#include "hustdb_network.h"

int main(int argc, const char* argv[])
{
    hustdb_t db;
    if (!db.open())
    {
        LOG_ERROR ("[hustdb_network] db open failed");
        return -1;
    }
    
    server_conf_t cf = db.get_server_conf();
    
    evhtp::http_basic_auth_t auth(cf.http_security_user.c_str(), cf.http_security_passwd.c_str());
    
    hustdb_network::ip_allow_t ip_allow_map = {0};
    hustdb_network::get_ip_allow_map ((char *) cf.http_access_allow.c_str(), cf.http_access_allow.size(), &ip_allow_map);
    
    hustdb_network_ctx_t ctx;
    ctx.base.threads = cf.tcp_worker_count;
    ctx.base.port = cf.tcp_port;
    ctx.base.backlog = cf.tcp_backlog;
    ctx.base.max_body_size = cf.tcp_max_body_size;
    ctx.base.max_keepalive_requests = cf.tcp_max_keepalive_requests;
    ctx.base.auth = auth.c_str();
    ctx.base.disable_100_cont = cf.disable_100_cont;
    ctx.base.enable_defer_accept = cf.tcp_enable_defer_accept;
    ctx.base.enable_nodelay  = cf.tcp_enable_nodelay;
    ctx.base.enable_reuseport = cf.tcp_enable_reuseport;
    ctx.base.recv_timeout.tv_sec = cf.tcp_recv_timeout;
    ctx.base.send_timeout.tv_sec = cf.tcp_send_timeout;
    ctx.ip_allow_map = &ip_allow_map;
    ctx.db = &db;

    if (!hustdb_loop(&ctx))
    {
        LOG_INFO ("[hustdb_network] hustdb_loop error");
    }

    LOG_INFO ("[hustdb_network] hustdb closed");
    return 0;
}
