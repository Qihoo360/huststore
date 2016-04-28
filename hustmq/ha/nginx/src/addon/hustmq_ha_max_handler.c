#include "hustmq_ha_handler.h"
#include "hustmq_ha_handler_filter.h"
#include "hustmq_ha_request_handler.h"

static ngx_bool_t __is_number(const char * param)
{
	if (!param)
	{
		return false;
	}

	size_t len = strlen(param);
	size_t i = 0;
	for (i = 0; i < len; ++i)
	{
		if (param[i] < '0' || param[i] > '9')
		{
			return false;
		}
	}
	return true;
}

static ngx_bool_t __check_request(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
	if (queue_dict && queue_dict->dict.ref && !hustmq_ha_max_queue_item_check(queue_dict, queue))
	{
		return false;
	}
	char * val = (ngx_http_get_param_val(&r->args, "num", r->pool));
	return __is_number(val);
}

ngx_int_t hustmq_ha_max_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	return hustmq_ha_handler_base(backend_uri, r, __check_request, hustmq_ha_post_subrequest_handler);
}
