#include "hustmq_ha_worker_def.h"

static hustmq_worker_item_t * hustmq_worker_dict_get(hustmq_worker_dict_t * dict, const char * key)
{
    if (!dict)
    {
        return 0;
    }
    hustmq_worker_item_t ** val = c_dict_get(&dict->dict, key);
    return val ? *val : 0;
}

void hustmq_ha_dispose_hustmq_workers_array(hustmq_workers_array_t * arr)
{
	if (!arr || !arr->arr)
	{
		return;
	}
	size_t i = 0;
	for(i = 0; i < arr->size; ++i)
	{
		cjson_dispose_hustmqworkerarray(arr->arr + i);
	}
}

static void __reset_worker_dict_status(hustmq_worker_dict_t * worker_dict)
{
	if (!worker_dict)
	{
		return;
	}

	worker_dict->worker_items = 0;

	const char * key;
	c_dict_iter_t iter = c_dict_iter(&worker_dict->dict);
	while ((key = c_dict_next(&worker_dict->dict, &iter)))
	{
		hustmq_worker_item_t * it = hustmq_worker_dict_get(worker_dict, key);
		if (it)
		{
			it->valid = false;
		}
	}
}

static void __merge_time(const int src, int * des)
{
	if (! des)
	{
		return;
	}
	if (src > *des)
	{
		*des = src;
	}
}

static void __update_worker_item(HustmqWorkerArray * arr, ngx_pool_t * pool, hustmq_worker_dict_t * worker_dict)
{
	size_t i = 0;
	for (i = 0; i < arr->size; ++i)
	{
		HustmqWorker * worker = arr->arr + i;
		hustmq_worker_item_t * worker_item = hustmq_worker_dict_get(worker_dict, worker->w.buf);
		if (worker_item)
		{
			if (!worker_item->valid)
			{
				worker_item->valid = true;
				worker_item->time = worker->t;
			}
			else
			{
				__merge_time(worker->t, &worker_item->time);
			}
		}
		else
		{
		    if (worker_dict->worker_items > HUSTMQ_HA_WORKER_ITEMS - 1)
		    {
		        return;
		    }
			hustmq_worker_item_t * item = ngx_palloc(pool, sizeof(hustmq_worker_item_t));
			if (!item)
			{
			    return;
			}
			memset(item, 0, sizeof(hustmq_worker_item_t));
			item->valid = true;
			item->time = worker->t;
			hustmq_ha_copy_from_json_str(&worker->w, item->worker, HUSTMQ_HA_WORKER_SIZE);
			c_dict_set(&worker_dict->dict, item->worker, item);

			++worker_dict->worker_items;
		}
	}
}

static ngx_bool_t __is_dict_item_invalid(c_dict_base_t * dict, const char * key)
{
    hustmq_worker_item_t * worker_item = hustmq_worker_dict_get((hustmq_worker_dict_t *)dict, key);
    return (!worker_item || !worker_item->valid);
}

void hustmq_ha_update_worker_dict(const hustmq_workers_array_t * arr, ngx_pool_t * pool, hustmq_worker_dict_t * worker_dict)
{
	if (!arr || !arr->arr || !pool || !worker_dict)
	{
		return;
	}

	__reset_worker_dict_status(worker_dict);

	size_t i = 0;
	for (i = 0; i < arr->size; ++i)
	{
		__update_worker_item(arr->arr + i, pool, worker_dict);
	}
	if (worker_dict->worker_items > HUSTMQ_HA_WORKER_ITEMS - 1)
	{
	    hustmq_ha_clean_invalid_items(&worker_dict->dict.base, __is_dict_item_invalid, &worker_dict->worker_items);
	}
}

ngx_bool_t hustmq_ha_merge_worker_dict(hustmq_worker_dict_t * worker_dict, ngx_pool_t * pool, hustmq_worker_array_t * arr)
{
	if (!worker_dict || !pool || !arr || !arr->arr)
	{
		return false;
	}

	arr->size = 0;

	hustmq_worker_t * it = arr->arr;

	const char * key;
	c_dict_iter_t iter = c_dict_iter(worker_dict);
	while ((key = c_dict_next(&worker_dict->dict, &iter)))
	{
		hustmq_worker_item_t * worker_item = hustmq_worker_dict_get(worker_dict, key);

		if (worker_item && worker_item->valid)
		{
			(it + arr->size)->worker = worker_item->worker;
			(it + arr->size)->time = worker_item->time;
			++arr->size;
			if (arr->size > HUSTMQ_HA_WORKER_ITEMS - 1)
			{
			    break;
			}
		}
	}

	return true;
}
