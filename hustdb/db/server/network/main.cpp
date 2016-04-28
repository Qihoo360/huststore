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
    
    hustdb_network::http_basic_auth_t auth(cf.http_security_user.c_str(), cf.http_security_passwd.c_str());
    
    hustdb_network::ip_allow_t ip_allow_map = {0};
    hustdb_network::get_ip_allow_map ((char *) cf.http_access_allow.c_str(), cf.http_access_allow.size(), &ip_allow_map);
    
    hustdb_network_ctx_t ctx;
    ctx.threads = cf.tcp_worker_count;
    ctx.port = cf.tcp_port;
    ctx.backlog = cf.tcp_backlog;
    ctx.max_body_size = cf.tcp_max_body_size;
    ctx.max_keepalive_requests = cf.tcp_max_keepalive_requests;
    ctx.auth = auth.c_str();
    ctx.disable_100_cont = cf.disable_100_cont;
    ctx.enable_defer_accept = cf.tcp_enable_defer_accept;
    ctx.enable_nodelay  = cf.tcp_enable_nodelay;
    ctx.enable_reuseport = cf.tcp_enable_reuseport;
    ctx.recv_timeout.tv_sec = cf.tcp_recv_timeout;
    ctx.send_timeout.tv_sec = cf.tcp_send_timeout;
    ctx.ip_allow_map = &ip_allow_map;
    ctx.db = &db;

    if (!hustdb_loop(&ctx))
    {
        LOG_INFO ("[hustdb_network] hustdb_loop error");
    }

    LOG_INFO ("[hustdb_network] hustdb closed");
    return 0;
}
