#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "module/global.h"
#include "module/sync_conf.h"
#include "network/sync_network_utils.h"
#include "network/sync_network.h"

void manual()
{
    printf("\n");
    printf("    usage:\n");
    printf("        ./hustdbsync [option]\n");
    printf("            [option]\n");
    printf("                -d: run in debug mode\n");
    printf("\n");
    printf("    sample:\n");
    printf("        ./hustdbsync\n");
    printf("        ./hustdbsync -d\n");
    printf("\n");
}

int main ( int argc, char *argv[] )
{
    if (2 != argc && 1 != argc)
    {
        manual();
        return -1;
    }
    bool debug_mode = false;
    if (2 == argc)
    {
        std::string option = argv[1];
        std::string debug("-d");
        if (option == debug)
        {
            debug_mode = true;
        }
        else
        {
            manual();
            return -1;
        }
    }
    if(!debug_mode && daemon(1, 1) < 0)
    {
        printf("daemon error\n");
        return -1;
    }

    jos_lib::SyncServerConf cf;
    if (!jos_lib::Load("sync_server.json", cf))
    {
        printf("load \"sync_server.json\" error\n");
        return -1;
    }

    if (!init(cf.sync.logs_path.c_str(),
        cf.sync.ngx_path.c_str(),
        cf.sync.auth_path.c_str(),
        cf.sync.threads,
        cf.sync.release_interval,
        cf.sync.checkdb_interval,
        cf.sync.checklog_interval))
    {
        printf("init sync server error\n");
        return -1;
    }

    evhtp::http_basic_auth_t auth ( cf.network.user.c_str(), cf.network.passwd.c_str() );

    sync_network::ip_allow_t ip_allow_map = {0};
    std::string access_allow = cf.network.access_allow;
    sync_network::get_ip_allow_map ( ( char * ) access_allow.c_str(), access_allow.size(), &ip_allow_map );

    sync_network_ctx_t ctx;
    ctx.base.threads = cf.network.threads;
    ctx.base.port = cf.network.port;
    ctx.base.backlog = cf.network.backlog;
    ctx.base.max_body_size = cf.network.max_body_size;
    ctx.base.max_keepalive_requests = cf.network.max_keepalive_requests;
    ctx.base.auth = auth.c_str();
    ctx.base.disable_100_cont = true;
    ctx.base.enable_defer_accept = cf.network.enable_defer_accept;
    ctx.base.enable_nodelay  = cf.network.enable_nodelay;
    ctx.base.enable_reuseport = cf.network.enable_reuseport;
    ctx.base.recv_timeout.tv_sec = cf.network.recv_timeout;
    ctx.base.send_timeout.tv_sec = cf.network.send_timeout;
    ctx.ip_allow_map = &ip_allow_map;

    if (!sync_loop(&ctx))
    {
        printf("sync_loop error\n");
    }

    if (!stop_sync())
    {
        printf("stop_sync error\n");
    }

    printf("sync server closed\n");

    return 0;
}
