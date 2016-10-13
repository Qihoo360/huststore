#include "hustdb_ha_table_inner.h"

// set_table
typedef struct
{
    ngx_str_t table_path;
    ngx_str_t table_tmp_path;
    ngx_str_t table_del_path;
} hustdb_ha_table_path_t;

static hustdb_ha_table_path_t g_table_path;

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
        if (!hustdb_ha_build_hash_table(&table, hustdb_ha_get_peer_dict(), pool, &hash_table, &bucket_table))
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

hustdb_ha_set_table_result_t hustdb_ha_overwrite_table(const ngx_str_t * data, ngx_pool_t * pool)
{
    return __overwrite_table(data, &g_table_path, pool);
}
