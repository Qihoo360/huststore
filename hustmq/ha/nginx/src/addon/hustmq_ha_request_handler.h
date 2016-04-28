#ifndef __hustmq_ha_request_handler_20150618061512_h__
#define __hustmq_ha_request_handler_20150618061512_h__

#include "hustmq_ha_stat_def.h"

ngx_int_t hustmq_ha_send_reply(ngx_uint_t status, ngx_buf_t * buf, size_t buf_size, ngx_http_request_t *r);

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
	ngx_int_t result;
} hustmq_ha_loop_ctx_t;

typedef ngx_bool_t (*check_request_pt)(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue);

ngx_int_t hustmq_ha_post_subrequest_handler(ngx_http_request_t * r, void * data, ngx_int_t rc);

ngx_str_t hustmq_ha_get_queue(ngx_http_request_t * r);
int hustmq_ha_get_idx(ngx_http_request_t * r);

ngx_int_t hustmq_ha_handler_base(
        ngx_str_t * backend_uri,
		ngx_http_request_t *r,
		check_request_pt checker,
		ngx_http_post_subrequest_pt handler);

typedef ngx_bool_t (*decode_json_array_t)(const char * input, void * obj_val);
ngx_bool_t hustmq_ha_decode_json_array(ngx_http_request_t *r, decode_json_array_t decode, void * arr);

void hustmq_ha_init_evget_handler(ngx_log_t * log);
void hustmq_ha_invoke_evget_handler();

void hustmq_ha_init_evsub_handler(ngx_log_t * log);
void hustmq_ha_invoke_evsub_handler();

typedef struct
{
    ngx_str_t token;
    ngx_str_t queue;
    ngx_buf_t * buf;
    size_t buf_size;
} hustmq_ha_do_task_t;

ngx_int_t hustmq_ha_init_do_get(ngx_http_hustmq_ha_main_conf_t * mcf);
hustmq_ha_do_task_t * hustmq_ha_assign_do_task();
ngx_bool_t hustmq_ha_reply_do_task(const char * token, ngx_buf_t * buf, size_t buf_size);

ngx_int_t hustmq_ha_init_do_post(ngx_http_hustmq_ha_main_conf_t * mcf);
ngx_bool_t hustmq_ha_dispatch_do_task(hustmq_ha_do_task_t * task);

#endif // __hustmq_ha_request_handler_20150618061512_h__
