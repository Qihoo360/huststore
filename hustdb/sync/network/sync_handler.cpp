#include "sync_handler.h"
#include "../module/global.h"

void sync_status_handler(const sync_status_ctx_t& args, evhtp_request_t * request, sync_network_ctx_t * ctx)
{
    std::string resp;
    if (!get_status(args.backend_count, resp))
    {
        evhtp::send_nobody_reply(EVHTP_RES_NOTFOUND, request);
    }
    else
    {
        evhtp::send_reply(EVHTP_RES_200, resp.c_str(), resp.size(), request);
    }
}
