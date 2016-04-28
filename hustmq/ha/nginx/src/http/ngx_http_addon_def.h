#ifndef __ngx_http_addon_def_20151028094729_h__
#define __ngx_http_addon_def_20151028094729_h__

// to be implement by addon
void * ngx_http_get_addon_module_ctx(ngx_http_request_t * r);
void ngx_http_set_addon_module_ctx(ngx_http_request_t * r, void * ctx);

#endif // __ngx_http_addon_def_20151028094729_h__
