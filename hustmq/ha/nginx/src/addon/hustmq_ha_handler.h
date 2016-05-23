#ifndef __hustmq_ha_handler_20150601202210_h__
#define __hustmq_ha_handler_20150601202210_h__

#include <ngx_http.h>
#include <nginx.h>

ngx_int_t hustmq_ha_worker_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_ack_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_timeout_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_autost_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_purge_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_max_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_lock_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_publish_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_subscribe_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_stat_all_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_stat_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_evget_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_evsub_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_do_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_do_post_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_do_get_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustmq_ha_do_post_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);

#endif // __hustmq_ha_handler_20150601202210_h__
