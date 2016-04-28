#include "hustmq_ha_data_def.h"

static const char * QUEUE_ITEM_JSON_FORMAT = "{\
\"queue\":\"%s\",\
\"type\":%d,\
\"ready\":[%d,%d,%d],\
\"max\":%d,\
\"lock\":%d,\
\"si\":%d,\
\"ci\":%d,\
\"tm\":%d\
}";

typedef ngx_bool_t (*serialize_item_t)(void * obj_val, char * json_val);
typedef size_t (*dump_item_t)(void * obj_val, size_t index, char * json_val);

static ngx_bool_t __update_buffer(size_t size, size_t item_size, ngx_pool_t * pool, hustmq_ha_buffer_t * buf)
{
	if (!buf || !buf->buf)
	{
		return false;
	}
	if (buf->items >= size)
	{
		return true;
	}
	buf->items = size * 2;
	buf->max_size = item_size * buf->items;
	buf->buf = ngx_palloc(pool, buf->max_size);
	if (!buf->buf)
	{
		return false;
	}
	return true;
}

static ngx_bool_t __serialize_one_arr(void * obj_val, serialize_item_t serialize_item, char * json_val)
{
	if (!obj_val || !json_val)
	{
		return false;
	}
	sprintf(json_val, "[");
	if (!serialize_item(obj_val, json_val + 1))
	{
		return false;
	}
	strcat(json_val, "]");
	return true;
}

static ngx_bool_t __serialize_array_base(
		void * arr,
		size_t size,
		ngx_pool_t * pool,
		size_t item_size,
		serialize_item_t serialize_item,
		dump_item_t dump_item,
		hustmq_ha_buffer_t * json_val)
{
	if (!arr || !pool || !json_val || !json_val->buf)
	{
		return false;
	}
	if (size < 1)
	{
		sprintf(json_val->buf, "[]");
		return true;
	}

	if (1 == size)
	{
		return __serialize_one_arr(arr, serialize_item, json_val->buf);
	}

	if (!__update_buffer(size, item_size, pool, json_val))
	{
		return false;
	}

	sprintf(json_val->buf, "[");
	size_t offset = 1;
	size_t i = 0;
	for (i = 0; i < size; ++i)
	{
		size_t len = dump_item(arr, i, json_val->buf + offset);
		if (len < 1)
		{
			return false;
		}
		strcat(json_val->buf, i == size - 1 ? "]" : ",");
		offset += (len + 1);
	}
	return true;
}

static void __dump_message_queue_item_imp(hustmq_ha_message_queue_item_t * obj_val, char * json_val)
{
	if (!obj_val || !json_val)
	{
		return;
	}
	sprintf(json_val, QUEUE_ITEM_JSON_FORMAT,
			obj_val->queue,
			obj_val->base.type,
			obj_val->base.ready[0],
			obj_val->base.ready[1],
			obj_val->base.ready[2],
			obj_val->base.max,
			obj_val->base.lock,
			obj_val->base.idx.start_idx,
			obj_val->base.idx.consistent_idx,
			obj_val->base.idx.timestamp);
}

ngx_bool_t hustmqha_serialize_message_queue_item(hustmq_ha_message_queue_item_t * obj_val, char * json_val)
{
	if (!obj_val || !obj_val->queue || !obj_val->base.ready || !json_val)
	{
		return false;
	}
	__dump_message_queue_item_imp(obj_val, json_val);
	return true;
}

static ngx_bool_t __serialize_message_queue_item(void * obj_val, char * json_val)
{
	return hustmqha_serialize_message_queue_item(obj_val, json_val);
}

static size_t __dump_message_queue_item(void * obj_val, size_t index, char * json_val)
{
	if (!obj_val)
	{
		return 0;
	}
	hustmq_ha_message_queue_item_t * it = (hustmq_ha_message_queue_item_t *)obj_val + index;
	if (!it || !it->queue || !it->base.ready || !json_val)
	{
		return 0;
	}
	__dump_message_queue_item_imp(it, json_val);
	return strlen(json_val);
}

ngx_bool_t hustmqha_serialize_message_queue_array(hustmq_ha_message_queue_array_t * obj_val, ngx_pool_t * pool, hustmq_ha_buffer_t * json_val)
{
	return obj_val ? __serialize_array_base(
			obj_val->arr,
			obj_val->size,
			pool,
			HUSTMQ_HA_QUEUE_ITEM_SIZE,
			__serialize_message_queue_item,
			__dump_message_queue_item,
			json_val) : false;
}

static void __dump_worker_item_imp(hustmq_worker_t * obj_val, char * json_val)
{
	if (!obj_val || !json_val)
	{
		return;
	}
	static const char * WORKER_JSON_FORMAT = "{\"w\":\"%s\",\"t\":%d}";
	sprintf(json_val, WORKER_JSON_FORMAT, obj_val->worker, obj_val->time);
}

static ngx_bool_t __serialize_worker_item(void * obj_val, char * json_val)
{
	hustmq_worker_t * item = obj_val;
	if (!item || !item->worker || !json_val)
	{
		return false;
	}
	__dump_worker_item_imp(item, json_val);
	return true;
}

static size_t __dump_worker_item(void * obj_val, size_t index, char * json_val)
{
	if (!obj_val)
	{
		return 0;
	}
	hustmq_worker_t * it = (hustmq_worker_t *)obj_val + index;
	if (!it || !it->worker || !json_val)
	{
		return false;
	}
	__dump_worker_item_imp(it, json_val);
	return strlen(json_val);
}

ngx_bool_t hustmqha_serialize_worker_array(hustmq_worker_array_t * obj_val, ngx_pool_t * pool, hustmq_ha_buffer_t * json_val)
{
	return obj_val ? __serialize_array_base(
			obj_val->arr,
			obj_val->size,
			pool,
			HUSTMQ_HA_WORKER_SIZE,
			__serialize_worker_item,
			__dump_worker_item,
			json_val) : false;
}
