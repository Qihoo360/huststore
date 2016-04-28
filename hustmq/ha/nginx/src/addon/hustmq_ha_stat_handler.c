#include "hustmq_ha_handler.h"
#include "hustmq_ha_data_def.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"

ngx_int_t hustmq_ha_stat_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	ngx_str_t queue = hustmq_ha_get_queue(r);
	if (!queue.data)
	{
		return NGX_ERROR;
	}

	hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();

	do
	{
		if (!queue_dict)
		{
			break;
		}

		static hustmq_ha_message_queue_item_t queue_item;
		if (!hustmq_ha_merge_queue_item(queue_dict, &queue, &queue_item))
		{
			break;
		}

		static char response[HUSTMQ_HA_QUEUE_ITEM_SIZE];
		ngx_bool_t ret = hustmqha_serialize_message_queue_item(&queue_item, response);
		if (!ret)
		{
			break;
		}

		ngx_str_t tmp;
		tmp.data = (u_char *) response;
		tmp.len = strlen(response);

		return ngx_http_send_response_imp(NGX_HTTP_OK, &tmp, r);

	} while (0);

	return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
}
