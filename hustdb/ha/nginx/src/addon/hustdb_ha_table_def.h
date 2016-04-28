#ifndef __hustdb_ha_table_def_20150728103656_h__
#define __hustdb_ha_table_def_20150728103656_h__

#include "hustdb_ha_utils.h"

typedef struct
{
	ngx_http_subrequest_peer_t * readlist;
	ngx_http_subrequest_peer_t * writelist;

	size_t __index;
	size_t __start;
	size_t __end;
} hustdb_ha_table_elem_t;

typedef struct
{
    ngx_http_subrequest_peer_t * readlist;
    ngx_http_subrequest_peer_t * writelist;
    size_t start;
    size_t end;
} hustdb_ha_bucket_t;

ngx_bool_t hustdb_ha_init_peer_dict();
ngx_http_upstream_rr_peer_t * hustdb_ha_peer_dict_get(const char * key);

ngx_bool_t hustdb_ha_build_table(const HustDbHaTable * table, ngx_pool_t * pool);

ngx_http_subrequest_peer_t * hustdb_ha_get_readlist(const char * key);
ngx_http_subrequest_peer_t * hustdb_ha_get_writelist(const char * key);

size_t hustdb_ha_get_buckets();
hustdb_ha_bucket_t * hustdb_ha_get_bucket(size_t index);

typedef enum
{
    SET_TABLE_SUCCESS = 1,
    WRITE_TABLE_TMP_ERR = 2,
    CHECK_TABLE_ERR = 3,
    RENAME_TABLE_ERR = 4,
    RENAME_TABLE_TMP_ERR = 5,
    ROLLBACK_OVERWRITE_ERR = 6
} hustdb_ha_set_table_result_t;

void hustdb_ha_init_table_path(ngx_str_t table_path, ngx_pool_t * pool);
hustdb_ha_set_table_result_t hustdb_ha_overwrite_table(const ngx_str_t * data, ngx_pool_t * pool);

ngx_bool_t hustdb_ha_load_backends(ngx_pool_t * pool, ngx_str_array_t * backends);

#endif // __hustdb_ha_table_def_20150728103656_h__
