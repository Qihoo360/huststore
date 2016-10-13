#include "hustdb_ha_table_inner.h"

static ngx_http_peer_dict_t g_peer_dict;
static hustdb_ha_table_t g_table;
static hustdb_ha_bucket_table_t g_bucket_table;

ngx_bool_t hustdb_ha_init_peer_dict()
{
    return ngx_http_init_peer_dict(ngx_http_get_backends(), &g_peer_dict);
}

ngx_http_peer_dict_t * hustdb_ha_get_peer_dict()
{
    return &g_peer_dict;
}

ngx_bool_t hustdb_ha_build_table(const HustDbHaTable * table, ngx_pool_t * pool)
{
    return hustdb_ha_build_hash_table(table, &g_peer_dict, pool, &g_table, &g_bucket_table);
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

hustdb_ha_bucket_t * hustdb_ha_get_bucket(size_t index)
{
    return (index < g_bucket_table.size) ? &g_bucket_table.arr[index] : NULL;
}
