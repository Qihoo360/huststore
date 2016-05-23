#include "hustmq_ha_handler.h"
#include "hustmq_ha_request_handler.h"

static ngx_bool_t __check_request(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
    char * val = ngx_http_get_param_val(&r->args, "minute", r->pool);
    if (!val)
    {
        return false;
    }
    int timeout = atoi(val);
    return timeout > 0 && timeout < 256;
}

ngx_int_t hustmq_ha_timeout_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustmq_ha_handler_base(backend_uri, r, __check_request, hustmq_ha_post_subrequest_handler);
}
