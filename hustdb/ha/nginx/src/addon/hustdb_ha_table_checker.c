#include "hustdb_ha_table_inner.h"

ngx_bool_t hustdb_ha_str_eq(const char * src, const char * dst)
{
    return 0 == strcmp(src, dst);
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

static ngx_bool_t __check_val(const string_pair_t * arr, size_t size)
{
    if (!arr)
    {
        return false;
    }
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        ngx_bool_t result = hustdb_ha_str_eq(arr[i].src, arr[i].dst);
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

static ngx_bool_t __check_remove_idx(int idx)
{
    return MASTER1 == idx || MASTER2 == idx;
}


static ngx_bool_t __check_decr_item(const HustDbHaTableDecr * decr, const HustDbHaTablePair * item)
{
    if (!decr->json_has_remove || !decr->json_has_replace_by)
    {
        return false;
    }
    if (hustdb_ha_str_eq(decr->remove.buf, decr->replace_by.buf))
    {
        return false;
    }
    int idx = hustdb_ha_get_remove_idx(decr, item);
    return __check_remove_idx(idx);
}

ngx_bool_t hustdb_ha_enable_increase(const HustDbHaTableElem * item)
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
    else if (hustdb_ha_enable_increase(item))
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

ngx_bool_t hustdb_ha_check_table_item(
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

ngx_bool_t hustdb_ha_review_table(hustdb_ha_table_t * table)
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

ngx_bool_t hustdb_ha_precheck_table(const HustDbHaTableElemArray * table)
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


ngx_bool_t hustdb_ha_check_decrease(const HustDbHaTableElemArray * table)
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
