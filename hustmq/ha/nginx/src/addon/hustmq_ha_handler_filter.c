#include "hustmq_ha_handler_filter.h"

static ngx_bool_t __merge_queue_base_item(const char * host, const hustmq_ha_queue_item_t * item, void * data)
{
	if (!item->base.valid || !data)
	{
		return false;
	}
	hustmq_ha_queue_base_t * it = data;
	if (!it->valid)
	{
		hustmq_ha_init_queue_base(&item->base, it);
	}
	else
	{
		hustmq_ha_merge_queue_base(&item->base, it);
	}
	return true;
}

static ngx_bool_t __put_queue_item_check(const hustmq_ha_queue_base_t * item)
{
	if (!item)
	{
		return false;
	}
	if (item->lock)
	{
		return false;
	}
	if (item->max > 0)
	{
		int sum = hustmq_ha_get_ready_sum(item->ready, HUSTMQ_HA_READY_SIZE);
		if (sum >= item->max)
		{
			return false;
		}
	}
	return true;
}

ngx_bool_t __get_queue_base_item(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue, hustmq_ha_queue_base_t * item)
{
	if (!queue_dict || !queue)
	{
		return false;
	}
	hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, (const char *)queue->data);
	if (!queue_val)
	{
		return false;
	}
	return hustmq_ha_traverse_host_dict(queue_val, __merge_queue_base_item, item);
}

ngx_bool_t hustmq_ha_put_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
	hustmq_ha_queue_base_t item;
    memset(&item, 0, sizeof(hustmq_ha_queue_base_t));
	if (!__get_queue_base_item(queue_dict, queue, &item))
	{
		return true; // a new queue, return true
	}
	return __put_queue_item_check(&item);
}

static ngx_bool_t __get_queue_item_check(const hustmq_ha_queue_base_t * item)
{
	if (!item)
	{
		return false;
	}
	int sum = hustmq_ha_get_ready_sum(item->ready, HUSTMQ_HA_READY_SIZE);
	if (sum < 1)
	{
		return false;
	}
	return true;
}

ngx_bool_t hustmq_ha_get_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
	hustmq_ha_queue_base_t item;
    memset(&item, 0, sizeof(hustmq_ha_queue_base_t));
	if (!__get_queue_base_item(queue_dict, queue, &item))
	{
		return false;
	}
	return __get_queue_item_check(&item);
}

static ngx_bool_t __max_queue_item_check(const hustmq_ha_queue_base_t * item)
{
	if (!item)
	{
		return false;
	}
	if (item->lock)
	{
		return false;
	}
	return true;
}

ngx_bool_t hustmq_ha_max_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
    hustmq_ha_queue_base_t item;
    memset(&item, 0, sizeof(hustmq_ha_queue_base_t));
	if (!__get_queue_base_item(queue_dict, queue, &item))
	{
		return false;
	}
	return __max_queue_item_check(&item);
}

ngx_bool_t hustmq_ha_publish_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue, hustmq_ha_queue_ctx_t * ctx)
{
    if (!queue_dict || !queue || !ctx)
    {
        return false;
    }
    hustmq_ha_queue_value_t * val = hustmq_ha_queue_dict_get(queue_dict, (const char *)queue->data);
    if (!val)
    {
        return true; // a new queue, return true
    }

    ctx->queue_val = val;
    if (!hustmq_ha_traverse_host_dict(val, __merge_queue_base_item, &ctx->item))
    {
        return false;
    }
    if (HUSTMQ_PUSH_QUEUE != ctx->item.type)
    {
        return false;
    }
    return __put_queue_item_check(&ctx->item);
}

ngx_bool_t hustmq_ha_idx_check(unsigned int idx, unsigned int si, unsigned int ci)
{
    if (0 == hustmq_ha_get_idx_cycle(si, ci))
    {
        return false;
    }
    if (0 == hustmq_ha_get_idx_cycle(si, idx))
    {
        return false;
    }
    idx = (idx + CYCLE_QUEUE_ITEM_NUM - 1) % CYCLE_QUEUE_ITEM_NUM;
    if (0 == hustmq_ha_get_idx_cycle(idx, ci))
    {
        return false;
    }
    return true;
}
