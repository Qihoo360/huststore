#include "hustdb_ha_table_def.h"

static const int START = 0;
static const int END = 1;

static const int UNKNOWN_MASTER = -1;
static const int MASTER1 = 0;
static const int MASTER2 = 1;

typedef struct
{
    hustdb_ha_table_elem_t table[HUSTDB_TABLE_SIZE];
} hustdb_ha_table_t;

typedef struct
{
    hustdb_ha_bucket_t * arr;
    size_t size;
} hustdb_ha_bucket_table_t;

static ngx_http_peer_dict_t g_peer_dict;
static hustdb_ha_table_t g_table;
static hustdb_ha_bucket_table_t g_bucket_table;

typedef size_t bucket_index_t;

typedef struct
{
	const char * src;
	const char * dst;
	ngx_bool_t eq; // expected result
} string_pair_t;

ngx_bool_t hustdb_ha_init_peer_dict()
{
    return ngx_http_init_peer_dict(ngx_http_get_backends(), &g_peer_dict);
}

ngx_http_upstream_rr_peer_t * hustdb_ha_peer_dict_get(const char * key)
{
    return ngx_http_peer_dict_get(&g_peer_dict, key);
}

static ngx_bool_t __check_incr_key(const HustDbHaTableIncr * incr, const HustDbHaTablePair * item)
{
	if (!incr || !item)
	{
		return false;
	}
	if (2 != incr->low.key.size || 2 != incr->high.key.size)
	{
		return false;
	}

	if (incr->low.key.arr[START] != item->key.arr[START])
	{
		return false;
	}
	if (incr->high.key.arr[END] != item->key.arr[END])
	{
		return false;
	}
	if (incr->low.key.arr[END] != incr->high.key.arr[START])
	{
		return false;
	}
	if (!(incr->low.key.arr[START] < incr->low.key.arr[END] && incr->high.key.arr[START] < incr->high.key.arr[END]))
	{
		return false;
	}
	return true;
}

ngx_bool_t __str_eq(const char * src, const char * dst)
{
    return 0 == strcmp(src, dst);
}

static ngx_bool_t __check_val(const string_pair_t * arr, size_t size)
{
	if (!arr)
	{
		return false;
	}
	size_t i = 0;
	for (i = 0; i < size; ++i)
	{
		ngx_bool_t result = __str_eq(arr[i].src, arr[i].dst);
		if (arr[i].eq != result)
		{
			return false;
		}
	}
	return true;
}

static ngx_bool_t __check_incr_val(const HustDbHaTableIncr * incr, const HustDbHaTablePair * item)
{
	if (!incr || !item)
	{
		return false;
	}
	if (2 != incr->low.val.size || 2 != incr->high.val.size)
	{
		return false;
	}

	string_pair_t arr[] = {
		{ incr->low.val.arr[MASTER1].buf,  item->val.arr[MASTER1].buf, true  },
		{ incr->high.val.arr[MASTER2].buf, item->val.arr[MASTER2].buf, true  },
		{ incr->low.val.arr[MASTER2].buf,  item->val.arr[MASTER1].buf, false },
		{ incr->low.val.arr[MASTER2].buf,  item->val.arr[MASTER2].buf, false },
		{ incr->high.val.arr[MASTER1].buf, item->val.arr[MASTER1].buf, false },
		{ incr->high.val.arr[MASTER1].buf, item->val.arr[MASTER2].buf, false }
	};
	size_t size = sizeof(arr) / sizeof(string_pair_t);

	return __check_val(arr, size);
}

static ngx_bool_t __check_incr_item(const HustDbHaTableIncr * incr, const HustDbHaTablePair * item)
{
	return __check_incr_key(incr, item) && __check_incr_val(incr, item);
}

static int __get_remove_idx(const HustDbHaTableDecr * decr, const HustDbHaTablePair * item)
{
    if (__str_eq(decr->remove.buf, item->val.arr[MASTER1].buf))
    {
        return MASTER1;
    }
    if (__str_eq(decr->remove.buf, item->val.arr[MASTER2].buf))
    {
        return MASTER2;
    }
    return UNKNOWN_MASTER;
}

ngx_bool_t __check_remove_idx(int idx)
{
    return MASTER1 == idx || MASTER2 == idx;
}

static ngx_bool_t __check_decr_item(const HustDbHaTableDecr * decr, const HustDbHaTablePair * item)
{
    if (!decr->json_has_remove || !decr->json_has_replace_by)
    {
        return false;
    }
    if (__str_eq(decr->remove.buf, decr->replace_by.buf))
    {
        return false;
    }
    int idx = __get_remove_idx(decr, item);
    return __check_remove_idx(idx);
}

static ngx_bool_t __enable_increase(const HustDbHaTableElem * item)
{
    return item->json_has_increase && item->increase.json_has_low && item->increase.json_has_high;
}

static ngx_bool_t __check_table_item_base(ngx_bool_t include_decrease, const HustDbHaTableElem * item)
{
    if (include_decrease)
    {
        if (item->json_has_decrease)
        {
            return __check_decr_item(&item->decrease, &item->item);
        }
    }
    else if (__enable_increase(item))
	{
		if (!__check_incr_item(&item->increase, &item->item))
		{
			return false;
		}
	}
	return true;
}

static ngx_bool_t __check_first_table_item(ngx_bool_t include_decrease, const HustDbHaTableElem * item)
{
	if (0 != item->item.key.arr[START])
	{
		return false;
	}
	if (item->item.key.arr[END] < 1 || item->item.key.arr[END] > HUSTDB_TABLE_SIZE)
	{
		return false;
	}
	return __check_table_item_base(include_decrease, item);
}

static ngx_bool_t __check_table_item(
    ngx_bool_t include_decrease,
    ngx_bool_t last,
    const HustDbHaTableElem * prev,
    const HustDbHaTableElem * item)
{
	if (!item)
	{
		return false;
	}
	if (2 != item->item.key.size || 2 != item->item.val.size)
	{
		return false;
	}
	if (!prev)
	{
		return __check_first_table_item(include_decrease, item);
	}
	if (last && HUSTDB_TABLE_SIZE != item->item.key.arr[END])
	{
		return false;
	}
	if (prev->item.key.arr[END] != item->item.key.arr[START])
	{
		return false;
	}
	return __check_table_item_base(include_decrease, item);
}

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

static ngx_bool_t __append_decr_item(
    const HustDbHaTableDecr * decr,
    const HustDbHaTablePair * item,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    bucket_index_t * index,
    hustdb_ha_table_t * table)
{
    int idx = __get_remained_idx(__get_remove_idx(decr, item));
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
	else if (__enable_increase(src))
	{
	    if (!__append_table_item(&src->increase.low, peer_dict, pool, index, des))
        {
            return false;
        }
        return __append_incr_item(&src->increase.high, &src->increase.low, peer_dict, pool, index, des);
	}
    return __append_table_item(&src->item, peer_dict, pool, index, des);
}

static ngx_bool_t __review_writelist(const ngx_http_subrequest_peer_t * writelist)
{
	if (!writelist)
	{
		return false;
	}
	int count = 0;
	const ngx_http_subrequest_peer_t * peer = writelist;
	while (peer)
	{
		++count;
		peer = peer->next;
	}
	return 2 == count;
}

static ngx_bool_t __review_table(hustdb_ha_table_t * table)
{
	if (!table)
	{
		return false;
	}
	size_t i = 0;
	for (i = 0; i < HUSTDB_TABLE_SIZE; ++i)
	{
		hustdb_ha_table_elem_t * elem = &(table->table[i]);
		if (!elem || !elem->readlist || !elem->writelist)
		{
			return false;
		}
		if (!__review_writelist(elem->writelist))
		{
			return false;
		}
	}
	return true;
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

ngx_bool_t __precheck_table(const HustDbHaTableElemArray * table)
{
    if (!table)
    {
        return false;
    }
    size_t i = 0;
    for (i = 0; i < table->size; ++i)
    {
        if (!table->arr[i].json_has_item)
        {
            return false;
        }
    }
    return true;
}

ngx_bool_t __check_decrease(const HustDbHaTableElemArray * table)
{
    c_dict_int_t remove_dict;
    c_dict_int_t replace_dict;

    c_dict_init(&remove_dict);
    c_dict_init(&replace_dict);

    size_t i = 0;
    for (i = 0; i < table->size; ++i)
    {
        HustDbHaTableElem * item = table->arr + i;
        if (!item->json_has_decrease)
        {
            continue;
        }
        int * val = c_dict_get(&remove_dict, item->decrease.remove.buf);
        if (!val)
        {
            c_dict_set(&remove_dict, item->decrease.remove.buf, 1);
        }
        val = c_dict_get(&replace_dict, item->decrease.replace_by.buf);
        if (!val)
        {
            c_dict_set(&replace_dict, item->decrease.replace_by.buf, 1);
        }
    }

    const char * key;
    c_dict_iter_t iter = c_dict_iter(&replace_dict);
    while ((key = c_dict_next(&replace_dict, &iter)))
    {
        int * val = c_dict_get(&remove_dict, key);
        if (val)
        {
            return false;
        }
    }
    return true;
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
        if (!__check_table_item(include_decrease, last, prev, item))
        {
            return false;
        }
        if (!__build_table_item(include_decrease, item, ctx->peer_dict, pool, &index, ctx->table))
        {
            return false;
        }
        prev = item;
    }
    if (include_decrease && !__check_decrease(table))
    {
        return false;
    }
    if (!__review_table(ctx->table))
    {
        return false;
    }
    return __build_bucket_table(ctx->table, pool, index, ctx->bucket_table);
}

static ngx_bool_t __build_hash_table(
    const HustDbHaTable * table,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    hustdb_ha_table_t * hash_table,
    hustdb_ha_bucket_table_t * bucket_table)
{
    if (!table || !table->table.arr || !pool || !__precheck_table(&table->table))
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

ngx_bool_t hustdb_ha_build_table(const HustDbHaTable * table, ngx_pool_t * pool)
{
    return __build_hash_table(table, &g_peer_dict, pool, &g_table, &g_bucket_table);
}

hustdb_ha_table_elem_t * __get_table_elem(const char * key, hustdb_ha_table_t * table)
{
    size_t len = strlen(key);
    ngx_uint_t hash = ngx_hash_key((u_char *)key, len);
    hash = hash % HUSTDB_TABLE_SIZE;

    return &(table->table[hash]);
}

ngx_http_subrequest_peer_t * hustdb_ha_get_readlist(const char * key)
{
    return !key ? NULL : __get_table_elem(key, &g_table)->readlist;
}

ngx_http_subrequest_peer_t * hustdb_ha_get_writelist(const char * key)
{
    return !key ? NULL : __get_table_elem(key, &g_table)->writelist;
}

size_t hustdb_ha_get_buckets()
{
    return g_bucket_table.size;
}

hustdb_ha_bucket_t * hustdb_ha_get_bucket(size_t index)
{
    return (index < g_bucket_table.size) ? &g_bucket_table.arr[index] : NULL;
}

// set_table
typedef struct
{
    ngx_str_t table_path;
    ngx_str_t table_tmp_path;
    ngx_str_t table_del_path;
} hustdb_ha_table_path_t;

static hustdb_ha_table_path_t g_table_path;

static ngx_bool_t __include_incr(HustDbHaTableElemArray * table)
{
    size_t i = 0;
    for (i = 0; i < table->size; ++i)
    {
        HustDbHaTableElem * item = table->arr + i;
        if (item->json_has_increase)
        {
            return true;
        }
    }
    return false;
}

static ngx_bool_t __check_table(const char * path, ngx_pool_t * pool)
{
    HustDbHaTable table;
    if (!cjson_load_hustdbhatable_from_file(path, &table))
    {
        return false;
    }
    ngx_bool_t rc = true;
    do
    {
        if (!table.json_has_table)
        {
            rc = false;
            break;
        }
        if (__include_incr(&table.table))
        {
            break;
        }
        hustdb_ha_table_t hash_table;
        hustdb_ha_bucket_table_t bucket_table;
        if (!__build_hash_table(&table, &g_peer_dict, pool, &hash_table, &bucket_table))
        {
            rc = false;
            break;
        }
    } while (0);

    cjson_dispose_hustdbhatable(&table);
    return rc;
}

static ngx_bool_t __save_to_file(const ngx_str_t * data, const char * path)
{
    if (!data || !path)
    {
        return false;
    }
    FILE * f = fopen(path, "wb");
    if (!f)
    {
        return false;
    }
    fwrite(data->data, 1, data->len, f);
    fclose(f);
    return true;
}

static hustdb_ha_set_table_result_t __overwrite_table(const ngx_str_t * data, const hustdb_ha_table_path_t * path, ngx_pool_t * pool)
{
    const char * table_tmp_path = (const char *)path->table_tmp_path.data;
    const char * table_del_path = (const char *)path->table_del_path.data;
    const char * table_path = (const char *)path->table_path.data;

    // write "file.tmp"
    if (!__save_to_file(data, table_tmp_path))
    {
        return WRITE_TABLE_TMP_ERR;
    }

    // check "file.tmp"
    if (!__check_table(table_tmp_path, pool))
    {
        return CHECK_TABLE_ERR;
    }

    // rename "file" to "file.del"
    if (0 != rename(table_path, table_del_path))
    {
        return RENAME_TABLE_ERR;
    }

    // rename "file.tmp" to "file"
    if (0 != rename(table_tmp_path, table_path))
    {
        // rollback
        if (0 != rename(table_del_path, table_path))
        {
            return ROLLBACK_OVERWRITE_ERR;
        }
        return RENAME_TABLE_TMP_ERR;
    }
    return SET_TABLE_SUCCESS;
}

hustdb_ha_set_table_result_t hustdb_ha_overwrite_table(const ngx_str_t * data, ngx_pool_t * pool)
{
    return __overwrite_table(data, &g_table_path, pool);
}

static void __init_table_tmp_path(const ngx_str_t * conf_file, const ngx_str_t * ext, ngx_pool_t * pool, ngx_str_t * path)
{
    size_t size = conf_file->len + ext->len;
    path->len = size;
    path->data = ngx_palloc(pool, size + 1);
    memcpy(path->data, conf_file->data, conf_file->len);
    memcpy(path->data + conf_file->len, ext->data, ext->len);
    path->data[size] = '\0';
}

void hustdb_ha_init_table_path(ngx_str_t table_path, ngx_pool_t * pool)
{
    g_table_path.table_path = table_path;

    ngx_str_t tmp = ngx_string(".tmp");
    __init_table_tmp_path(&table_path, &tmp, pool, &g_table_path.table_tmp_path);

    ngx_str_t del = ngx_string(".del");
    __init_table_tmp_path(&table_path, &del, pool, &g_table_path.table_del_path);
}

static void __add_hosts(const char * hosts[], size_t size, c_dict_int_t * set)
{
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        c_dict_set(set, hosts[i], true);
    }
}

static void __update_hosts(c_dict_int_t * removed_set, c_dict_int_t * backend_set)
{
    const char * host;
    c_dict_iter_t iter = c_dict_iter(removed_set);
    while ((host = c_dict_next(removed_set, &iter)))
    {
        c_dict_set(backend_set, host, false);
    }
}

static ngx_bool_t __traverse_set(c_dict_int_t * backend_set, ngx_pool_t * pool, hustdb_ha_traverse_line_cb cb, void * data)
{
    const char * host;
    c_dict_iter_t iter = c_dict_iter(backend_set);
    while ((host = c_dict_next(backend_set, &iter)))
    {
        int * val = c_dict_get(backend_set, host);
        if (val && *val)
        {
            if (!cb(host, pool, data))
            {
                return false;
            }
        }
    }
    return true;
}

static ngx_bool_t __get_backends_from_set(c_dict_int_t * backend_set, ngx_pool_t * pool, ngx_str_array_t * arr)
{
    size_t size = 0;
    if (!__traverse_set(backend_set, pool, hustdb_ha_incr_count, &size))
    {
        return false;
    }
    arr->arr = ngx_palloc(pool, size * sizeof(ngx_str_t));
    if (!arr->arr)
    {
        return false;
    }
    arr->size = 0;
    if (!__traverse_set(backend_set, pool, hustdb_ha_append_item, arr))
    {
        return false;
    }
    return true;
}

ngx_bool_t hustdb_ha_load_backends(ngx_pool_t * pool, ngx_str_array_t * backends)
{
    if (!pool || !backends)
    {
        return false;
    }
    HustDbHaTable table;
    if (!cjson_load_hustdbhatable_from_file((const char *)g_table_path.table_path.data, &table))
    {
        return false;
    }

    c_dict_int_t backend_set;
    c_dict_int_t removed_set;
    c_dict_init(&backend_set);
    c_dict_init(&removed_set);

    size_t i = 0;
    for (i = 0; i < table.table.size; i++)
    {
        HustDbHaTableElem * item = table.table.arr + i;
        if (item->json_has_increase)
        {
            const char * hosts[] = {
                item->increase.low.val.arr[MASTER1].buf,
                item->increase.low.val.arr[MASTER2].buf,
                item->increase.high.val.arr[MASTER1].buf,
                item->increase.high.val.arr[MASTER2].buf
            };
            size_t size = sizeof(hosts) / sizeof(char *);
            __add_hosts(hosts, size, &backend_set);
        }
        else if (item->json_has_decrease)
        {
            const char * hosts[] = {
                item->item.val.arr[MASTER1].buf,
                item->item.val.arr[MASTER2].buf,
                item->decrease.replace_by.buf
            };
            size_t size = sizeof(hosts) / sizeof(char *);
            __add_hosts(hosts, size, &backend_set);

            c_dict_set(&removed_set, item->decrease.remove.buf, true);
        }
        else
        {
            const char * hosts[] = {
                item->item.val.arr[MASTER1].buf,
                item->item.val.arr[MASTER2].buf
            };
            size_t size = sizeof(hosts) / sizeof(char *);
            __add_hosts(hosts, size, &backend_set);
        }
    }
    __update_hosts(&removed_set, &backend_set);
    ngx_bool_t rc = __get_backends_from_set(&backend_set, pool, backends);

    c_dict_deinit(&removed_set);
    c_dict_deinit(&backend_set);

    cjson_dispose_hustdbhatable(&table);
    return rc;
}
