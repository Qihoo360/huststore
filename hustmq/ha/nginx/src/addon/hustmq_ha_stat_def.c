#include "hustmq_ha_stat_def.h"

hustmq_ha_queue_item_t * hustmq_ha_host_dict_get(hustmq_ha_queue_value_t * dict, const char * key)
{
    if (!dict)
    {
        return 0;
    }
    hustmq_ha_queue_item_t ** val = c_dict_get(&dict->host_dict, key);
    return val ? *val : 0;
}

hustmq_ha_queue_value_t * hustmq_ha_queue_dict_get(hustmq_ha_queue_dict_t * dict, const char * key)
{
    if (!dict)
    {
        return 0;
    }
    hustmq_ha_queue_value_t ** val = c_dict_get(&dict->dict, key);
    return val ? *val : 0;
}

void hustmq_ha_dispose_backend_stat_array(backend_stat_array_t * arr)
{
	if (!arr || !arr->arr)
	{
		return;
	}
	size_t i = 0;
	for(i = 0; i < arr->size; ++i)
	{
		cjson_dispose_hustmqhamessagequeuearray(&(arr->arr + i)->arr);
	}
}

static void __update_queue_data(const HustmqHaMessageQueue * src, hustmq_ha_queue_base_t * dst)
{
    dst->valid = true;
    dst->type = src->type;
    dst->lock = src->lock;
    dst->max = src->max;
    dst->idx.start_idx = src->si;
    dst->idx.consistent_idx = src->ci;
    dst->idx.timestamp = src->tm;
    dst->unacked = src->unacked;
    dst->timeout = src->timeout;
    hustmq_ha_copy_from_integer_array(&src->ready, dst->ready, HUSTMQ_HA_READY_SIZE);
}

static void __copy_idx(const hustmq_ha_idx_t * src, hustmq_ha_idx_t * dst)
{
    if (!src || !dst)
    {
        return;
    }
    dst->start_idx = src->start_idx;
    dst->consistent_idx = src->consistent_idx;
    dst->timestamp = src->timestamp;
}

void hustmq_ha_init_queue_base(const hustmq_ha_queue_base_t * src, hustmq_ha_queue_base_t * dst)
{
    dst->valid = true;
    dst->type = src->type;
    dst->lock = src->lock;
    dst->max = src->max;
    dst->unacked = src->unacked;
    dst->timeout = src->timeout;
    __copy_idx(&src->idx, &dst->idx);
    memcpy(dst->ready, src->ready, sizeof(int) * HUSTMQ_HA_READY_SIZE);
}

static hustmq_ha_queue_item_t * __create_item(ngx_str_t * host, HustmqHaMessageQueue * it, ngx_pool_t * pool)
{
	hustmq_ha_queue_item_t * item = ngx_palloc(pool, sizeof(hustmq_ha_queue_item_t));
	if (!item)
	{
	    return NULL;
	}
	memset(item, 0, sizeof(hustmq_ha_queue_item_t));
	hustmq_ha_copy_from_json_str(&it->queue, item->queue, HUSTMQ_HA_QUEUE_SIZE);
	item->host = (const char *) host->data;
	__update_queue_data(it, (hustmq_ha_queue_base_t *)item);
	return item;
}

static void __update_item(ngx_str_t * host, HustmqHaMessageQueue * it, ngx_pool_t * pool, hustmq_ha_queue_value_t * queue_val)
{
	hustmq_ha_queue_item_t * queue_item = hustmq_ha_host_dict_get(queue_val, (const char *)host->data);
	if (queue_item)
	{
		__update_queue_data(it, &queue_item->base);
	}
	else
	{
		hustmq_ha_queue_item_t * item = __create_item(host, it, pool);
		if (!item)
		{
		    return;
		}
		c_dict_set(&queue_val->host_dict, (const char *)host->data, item);
	}
}

static void __add_item(ngx_str_t * host, HustmqHaMessageQueue * it, ngx_pool_t * pool, hustmq_ha_queue_dict_t * queue_dict)
{
    if (queue_dict->queue_items > hustmq_ha_get_max_queue_size() - 1)
    {
        return; // reach max limit
    }
    hustmq_ha_queue_value_t * queue_val = ngx_palloc(pool, sizeof(hustmq_ha_queue_value_t));
    if (!queue_val)
    {
        return;
    }
	memset(queue_val, 0, sizeof(hustmq_ha_queue_value_t));
	hustmq_ha_queue_item_t * item = __create_item(host, it, pool);
	if (!item)
	{
	    return;
	}
	c_dict_init(&queue_val->host_dict);
	c_dict_set(&queue_val->host_dict, (const char *)host->data, item);
	c_dict_set(&queue_dict->dict, (const char *)item->queue, queue_val);
	++queue_dict->queue_items;
}

static void __update_queue_dict(ngx_str_t * host, HustmqHaMessageQueueArray * arr, ngx_pool_t * pool, hustmq_ha_queue_dict_t * queue_dict)
{
    size_t i = 0;
    for (i = 0; i < arr->size; ++i)
    {
        HustmqHaMessageQueue * it = arr->arr + i;
        hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, it->queue.buf);
        if (queue_val)
        {
            __update_item(host, it, pool, queue_val);
        }
        else
        {
            __add_item(host, it, pool, queue_dict);
        }
    }
}

static ngx_bool_t __in_backend_stat_array(const u_char * key, const backend_stat_array_t * arr)
{
	if (!key || !arr || !arr->arr)
	{
		return false;
	}
	size_t i = 0;
	for (i = 0; i < arr->size; ++i)
	{
		if (0 == ngx_strncmp((arr->arr + i)->host->data, key, (arr->arr + i)->host->len))
		{
			return true;
		}
	}
	return false;
}

static void __reset_host_dict_status(ngx_bool_t status_cache, const backend_stat_array_t * arr, hustmq_ha_queue_value_t * queue_val)
{
	const char * key;
	c_dict_iter_t iter = c_dict_iter(&queue_val->host_dict);
	while ((key = c_dict_next(&queue_val->host_dict, &iter)))
	{
		if (status_cache && !__in_backend_stat_array((const u_char *)key, arr))
		{
			continue;
		}
		hustmq_ha_queue_item_t * queue_item = hustmq_ha_host_dict_get(queue_val, key);
		if (queue_item)
		{
			queue_item->base.valid = false;
		}
	}
}

static void __reset_queue_dict_status(ngx_bool_t status_cache, const backend_stat_array_t * arr, hustmq_ha_queue_dict_t * queue_dict)
{
	if (!queue_dict)
	{
		return;
	}

	queue_dict->queue_items = 0;

	const char * key;
	c_dict_iter_t iter = c_dict_iter(&queue_dict->dict);
	while ((key = c_dict_next(&queue_dict->dict, &iter)))
	{
		hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, key);
		if (queue_val)
		{
			__reset_host_dict_status(status_cache, arr, queue_val);
		}
	}
}

static ngx_bool_t __is_invalid(hustmq_ha_queue_value_t * queue_val)
{
    const char * key;
    c_dict_iter_t iter = c_dict_iter(&queue_val->host_dict);
    while ((key = c_dict_next(&queue_val->host_dict, &iter)))
    {
        hustmq_ha_queue_item_t * queue_item = hustmq_ha_host_dict_get(queue_val, key);
        if (!queue_item->base.valid)
        {
            return true;
        }
    }
    return false;
}

static ngx_bool_t __is_dict_item_invalid(c_dict_base_t * dict, const char * key)
{
    return __is_invalid(hustmq_ha_queue_dict_get((hustmq_ha_queue_dict_t *)dict, key));
}

void hustmq_ha_update_queue_dict(ngx_bool_t status_cache, const backend_stat_array_t * arr, ngx_pool_t * pool, hustmq_ha_queue_dict_t * queue_dict)
{
	if (!arr || !arr->arr || !pool || !queue_dict)
	{
		return;
	}

	__reset_queue_dict_status(status_cache, arr, queue_dict);

	size_t i = 0;
	for (i = 0; i < arr->size; ++i)
	{
		backend_stat_item_t * it = arr->arr + i;
		__update_queue_dict(it->host, &it->arr, pool, queue_dict);
	}
	if (queue_dict->queue_items > hustmq_ha_get_max_queue_size() - 1)
    {
	    hustmq_ha_clean_invalid_items(&queue_dict->dict.base, __is_dict_item_invalid, &queue_dict->queue_items);
    }
}

static void __merge_type(hustmq_queue_type_t src, hustmq_queue_type_t * dst)
{
    if (dst && HUSTMQ_PUSH_QUEUE == src)
    {
        *dst = src;
    }
}

static void __merge_ready(const int * src, size_t size, int * dst)
{
	size_t i = 0;
	for (i = 0; i < size; ++i)
	{
		*(dst + i) += *(src + i);
	}
}

static void __merge_lock(int src, int * dst)
{
	if (dst)
	{
		(*dst) |= src;
	}
}

static void __merge_max(int src, int * dst)
{
	if (dst && src > 0)
	{
	    if (0 == *dst)
	    {
	        *dst = src;
	    }
	    else
	    {
	        if (src < *dst)
		    {
		        *dst = src;
		    }
	    }
	}
}

static void __merge_unacked(int src, int * dst)
{
    if (dst)
    {
        *dst += src;
    }
}

static void __merge_timeout(int src, int * dst)
{
    if (dst)
    {
        if (src > *dst)
        {
            *dst = src;
        }
    }
}

static ngx_bool_t __greater_than(unsigned int src, unsigned int dst)
{
    if (src > dst)
    {
        unsigned int delta = src - dst;
        if (delta < MAX_QUEUE_ITEM_NUM)
        {
            return true;
        }
        return false;
    }
    else
    {
        unsigned int delta = dst - src;
        if (delta < MAX_QUEUE_ITEM_NUM)
        {
            return false;
        }
        return true;
    }
}

static void __merge_idx(const hustmq_ha_idx_t * src, hustmq_ha_idx_t * dst)
{
    if (!src || !dst)
    {
        return;
    }

    if (__greater_than(src->consistent_idx, dst->consistent_idx) && (src->timestamp > dst->timestamp))
    {
        __copy_idx(src, dst);
    }
}

void hustmq_ha_merge_queue_base(const hustmq_ha_queue_base_t * src, hustmq_ha_queue_base_t * dst)
{
    __merge_type(src->type, &dst->type);
    __merge_ready(src->ready, HUSTMQ_HA_READY_SIZE, dst->ready);
    __merge_lock(src->lock, &dst->lock);
    __merge_max(src->max, &dst->max);
    __merge_idx(&src->idx, &dst->idx);
    __merge_unacked(src->unacked, &dst->unacked);
    __merge_timeout(src->timeout, &dst->timeout);
}

static ngx_bool_t __merge_queue_item(const char * host, const hustmq_ha_queue_item_t * item, void * data)
{
	if (!item->base.valid || !data)
	{
		return false;
	}
	hustmq_ha_message_queue_item_t * it = data;
	if (!it->base.valid)
	{
		it->queue = item->queue;
		hustmq_ha_init_queue_base(&item->base, &it->base);
	}
	else
	{
	    hustmq_ha_merge_queue_base(&item->base, &it->base);
	}
	return true;
}

ngx_bool_t hustmq_ha_traverse_host_dict(hustmq_ha_queue_value_t * queue_val, hustmq_ha_queue_item_handler handler, void * data)
{
	ngx_bool_t ret = false;
	const char * key;
	c_dict_iter_t iter = c_dict_iter(&queue_val->host_dict);
	while ((key = c_dict_next(&queue_val->host_dict, &iter)))
	{
		hustmq_ha_queue_item_t * queue_item = hustmq_ha_host_dict_get(queue_val, key);
		if (handler(key, queue_item, data))
		{
			ret = true;
		}
	}
	return ret;
}

ngx_bool_t hustmq_ha_merge_queue_dict(
        hustmq_ha_queue_dict_t * queue_dict,
        ngx_pool_t * pool,
        hustmq_ha_message_queue_array_t * arr)
{
	if (!queue_dict || !pool || !arr || !arr->arr)
	{
		return false;
	}

	arr->size = 0;

	hustmq_ha_message_queue_item_t * it = arr->arr;

	const char * key;
	c_dict_iter_t iter = c_dict_iter(&queue_dict->dict);
	while ((key = c_dict_next(&queue_dict->dict, &iter)))
	{
		hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, key);
        (it + arr->size)->base.valid = false;
		if (hustmq_ha_traverse_host_dict(queue_val, __merge_queue_item, it + arr->size))
        {
            ++arr->size;
            if (arr->size > hustmq_ha_get_max_queue_size() - 1)
            {
                break;
            }
        }
    }
	return true;
}

ngx_bool_t hustmq_ha_merge_queue_item(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue, hustmq_ha_message_queue_item_t * item)
{
	if (!queue_dict || !queue || !item)
	{
		return false;
	}

	hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, (const char *)queue->data);
	if (!queue_val)
	{
		return false;
	}

	item->base.valid = false;

	return hustmq_ha_traverse_host_dict(queue_val, __merge_queue_item, item);
}
