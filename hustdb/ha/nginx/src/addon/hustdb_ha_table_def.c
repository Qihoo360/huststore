#include "hustdb_ha_table_inner.h"

ngx_http_subrequest_peer_t * __build_peer_list(const char *keys[], size_t size, const ngx_http_peer_dict_t * peer_dict, ngx_pool_t * pool)
{
	if (!keys || !peer_dict || !pool)
	{
		return NULL;
	}
	ngx_http_subrequest_peer_t * head = ngx_palloc(pool, sizeof(ngx_http_subrequest_peer_t));
	if (!head)
	{
		return NULL;
	}
	head->peer = NULL;
	head->next = NULL;

	ngx_http_subrequest_peer_t * node = head;
	size_t i = 0;
	for (i = 0; i < size; ++i)
	{
	    ngx_http_upstream_rr_peer_t * peer = ngx_http_peer_dict_get((ngx_http_peer_dict_t *)peer_dict, keys[i]);
        if (!peer)
        {
            return NULL;
        }
		node = ngx_http_append_peer(pool, peer, node);
		if (!node)
		{
			return NULL;
		}
	}
	return head;
}

static void __set_table_item(
    json_int_t * key,
    ngx_http_subrequest_peer_t * readlist,
    ngx_http_subrequest_peer_t * writelist,
    bucket_index_t * index,
    hustdb_ha_table_t * table)
{
	if (!key || !readlist || !writelist || !index)
	{
		return;
	}
	json_int_t i;
	for (i = key[START]; i < key[END]; ++i)
	{
		table->table[i].readlist = readlist;
		table->table[i].writelist = writelist;
		table->table[i].__index = *index;
		table->table[i].__start = key[START];
		table->table[i].__end = key[END];
	}
	++*index;
}

static ngx_bool_t __append_table_item(
    const HustDbHaTablePair * item,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    bucket_index_t * index,
    hustdb_ha_table_t * table)
{
	const char * keys[] = { item->val.arr[MASTER1].buf, item->val.arr[MASTER2].buf };
	size_t size = sizeof(keys) / sizeof(char *);

	ngx_http_subrequest_peer_t * list = __build_peer_list(keys, size, peer_dict, pool);
	if (!list)
	{
		return false;
	}

	__set_table_item(item->key.arr, list, list, index, table);
	return true;
}

static ngx_bool_t __append_incr_item(
    const HustDbHaTablePair * high,
    const HustDbHaTablePair * low,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    bucket_index_t * index,
    hustdb_ha_table_t * table)
{
	const char * wkeys[] = { high->val.arr[MASTER1].buf, high->val.arr[MASTER2].buf };
	size_t size = sizeof(wkeys) / sizeof(char *);

	ngx_http_subrequest_peer_t * writelist = __build_peer_list(wkeys, size, peer_dict, pool);
	if (!writelist)
	{
		return false;
	}

	const char * rkeys[] = {
		high->val.arr[MASTER1].buf,
		high->val.arr[MASTER2].buf,
		low->val.arr[MASTER2].buf,
		low->val.arr[MASTER2].buf,
	};
	size = sizeof(rkeys) / sizeof(char *);

	ngx_http_subrequest_peer_t * readlist = __build_peer_list(rkeys, size, peer_dict, pool);
	if (!readlist)
	{
		return false;
	}

	__set_table_item(high->key.arr, readlist, writelist, index, table);
	return true;
}

static int __get_remained_idx(int idx)
{
    if (MASTER1 == idx)
    {
        return MASTER2;
    }
    if (MASTER2 == idx)
    {
        return MASTER1;
    }
    return UNKNOWN_MASTER;
}

int hustdb_ha_get_remove_idx(const HustDbHaTableDecr * decr, const HustDbHaTablePair * item)
{
    if (hustdb_ha_str_eq(decr->remove.buf, item->val.arr[MASTER1].buf))
    {
        return MASTER1;
    }
    if (hustdb_ha_str_eq(decr->remove.buf, item->val.arr[MASTER2].buf))
    {
        return MASTER2;
    }
    return UNKNOWN_MASTER;
}

static ngx_bool_t __append_decr_item(
    const HustDbHaTableDecr * decr,
    const HustDbHaTablePair * item,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    bucket_index_t * index,
    hustdb_ha_table_t * table)
{
    int idx = __get_remained_idx(hustdb_ha_get_remove_idx(decr, item));
    if (UNKNOWN_MASTER == idx)
    {
        return false;
    }

    const char * keys[] = { item->val.arr[idx].buf, decr->replace_by.buf };
    size_t size = sizeof(keys) / sizeof(char *);

    ngx_http_subrequest_peer_t * list = __build_peer_list(keys, size, peer_dict, pool);
    if (!list)
    {
        return false;
    }

    __set_table_item(item->key.arr, list, list, index, table);
    return true;
}

static ngx_bool_t __build_table_item(
    ngx_bool_t include_decrease,
    const HustDbHaTableElem * src,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    bucket_index_t * index,
    hustdb_ha_table_t * des)
{
	if (!src || !pool || !des)
	{
		return false;
	}
	if (include_decrease)
	{
	    if (src->json_has_decrease)
	    {
	        return __append_decr_item(&src->decrease, &src->item, peer_dict, pool, index, des);
	    }
	}
	else if (hustdb_ha_enable_increase(src))
	{
	    if (!__append_table_item(&src->increase.low, peer_dict, pool, index, des))
        {
            return false;
        }
        return __append_incr_item(&src->increase.high, &src->increase.low, peer_dict, pool, index, des);
	}
    return __append_table_item(&src->item, peer_dict, pool, index, des);
}

static void __set_bucket(const hustdb_ha_table_elem_t * src, hustdb_ha_bucket_t * des)
{
    des->start = src->__start;
    des->end = src->__end;
    des->readlist = src->readlist;
    des->writelist = src->writelist;
}

static ngx_bool_t __skip(const hustdb_ha_table_elem_t * src, hustdb_ha_bucket_t * des)
{
    return (src->__start == des->start) && (src->__end == des->end);
}

ngx_bool_t __build_bucket_table(
    const hustdb_ha_table_t * src,
    ngx_pool_t * pool,
    size_t size,
    hustdb_ha_bucket_table_t * des)
{
    if (!src || !pool || !des || size < 1)
    {
        return false;
    }
    des->arr = ngx_palloc(pool, sizeof(hustdb_ha_bucket_t) * size);
    des->size = 0;
    size_t i = 0;
    for (i = 0; i < HUSTDB_TABLE_SIZE; ++i)
    {
        if (0 == i)
        {
            __set_bucket(&src->table[i], des->arr);
            ++des->size;
        }
        else
        {
            if (__skip(&src->table[i], des->arr + (des->size - 1)))
            {
                continue;
            }
            if (des->size >= size)
            {
                return false;
            }
            __set_bucket(&src->table[i], des->arr + des->size);
            ++des->size;
        }
    }
    return true;
}

ngx_bool_t __include_increase(const HustDbHaTableElemArray * table)
{
    size_t i = 0;
    for (i = 0; i < table->size; ++i)
    {
        if (table->arr[i].json_has_increase)
        {
            return true;
        }
    }
    return false;
}

ngx_bool_t __include_decrease(const HustDbHaTableElemArray * table)
{
    size_t i = 0;
    for (i = 0; i < table->size; ++i)
    {
        if (table->arr[i].json_has_decrease)
        {
            return true;
        }
    }
    return false;
}

typedef struct
{
    const ngx_http_peer_dict_t * peer_dict;
    hustdb_ha_table_t * table;
    hustdb_ha_bucket_table_t * bucket_table;
} build_table_ctx_t;

ngx_bool_t __build_table(
    ngx_bool_t include_decrease,
    const HustDbHaTableElemArray * table,
    ngx_pool_t * pool,
    build_table_ctx_t * ctx)
{
    bucket_index_t index = 0;
    HustDbHaTableElem * prev = NULL;
    size_t size = table->size;
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        HustDbHaTableElem * item = table->arr + i;
        ngx_bool_t last = (size - 1 == i);
        if (!hustdb_ha_check_table_item(include_decrease, last, prev, item))
        {
            return false;
        }
        if (!__build_table_item(include_decrease, item, ctx->peer_dict, pool, &index, ctx->table))
        {
            return false;
        }
        prev = item;
    }
    if (include_decrease && !hustdb_ha_check_decrease(table))
    {
        return false;
    }
    if (!hustdb_ha_review_table(ctx->table))
    {
        return false;
    }
    return __build_bucket_table(ctx->table, pool, index, ctx->bucket_table);
}

ngx_bool_t hustdb_ha_build_hash_table(
    const HustDbHaTable * table,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    hustdb_ha_table_t * hash_table,
    hustdb_ha_bucket_table_t * bucket_table)
{
    if (!table || !table->table.arr || !pool || !hustdb_ha_precheck_table(&table->table))
    {
        return false;
    }

    ngx_bool_t include_increase = __include_increase(&table->table);
    ngx_bool_t include_decrease = __include_decrease(&table->table);

    if (include_increase && include_decrease)
    {
        return false; // could not include both increase and decrease
    }

    build_table_ctx_t ctx = { peer_dict, hash_table, bucket_table };
    return __build_table(include_decrease, &table->table, pool, &ctx);
}
