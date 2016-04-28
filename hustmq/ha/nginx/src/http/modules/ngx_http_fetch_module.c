#include "ngx_http_fetch.h"

static ngx_http_module_t ngx_http_fetch_module_ctx =
{
    NULL, // ngx_int_t (*preconfiguration)(ngx_conf_t *cf);
    NULL, // ngx_int_t (*postconfiguration)(ngx_conf_t *cf);
    NULL, // void *(*create_main_conf)(ngx_conf_t *cf);
    NULL, // char *(*init_main_conf)(ngx_conf_t *cf, void *conf);
    NULL, // void * (*create_srv_conf)(ngx_conf_t *cf);
    NULL, // char * (*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);
    NULL, // void * (*create_loc_conf)(ngx_conf_t *cf);
    NULL // char * (*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
};

ngx_module_t ngx_http_fetch_module =
{
    NGX_MODULE_V1,
    &ngx_http_fetch_module_ctx,
    NULL,
    NGX_HTTP_MODULE,
    NULL, // ngx_int_t (*init_master)(ngx_log_t *log);
    NULL, // ngx_int_t (*init_module)(ngx_cycle_t *cycle);
    NULL, // ngx_int_t (*init_process)(ngx_cycle_t *cycle);
    NULL, // ngx_int_t (*init_thread)(ngx_cycle_t *cycle);
    NULL, // void (*exit_thread)(ngx_cycle_t *cycle);
    NULL, // void (*exit_process)(ngx_cycle_t *cycle);
    NULL, // void (*exit_master)(ngx_cycle_t *cycle);
    NGX_MODULE_V1_PADDING
};

void * ngx_http_fetch_get_module_ctx(ngx_http_request_t * r)
{
    if (!r)
    {
        return NULL;
    }
    return ngx_http_get_module_ctx(r, ngx_http_fetch_module);
}

void ngx_http_fetch_set_module_ctx(ngx_http_request_t * r, void * ctx)
{
    if (!r)
    {
        return;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_fetch_module);
}
