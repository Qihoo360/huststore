#ifndef __hustdb_ha_table_inner_20161013193359_h__
#define __hustdb_ha_table_inner_20161013193359_h__

#include "hustdb_ha_table_def.h"

static const int START = 0;
static const int END = 1;

static const int UNKNOWN_MASTER = -1;
static const int MASTER1 = 0;
static const int MASTER2 = 1;

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
    hustdb_ha_table_elem_t table[HUSTDB_TABLE_SIZE];
} hustdb_ha_table_t;

typedef struct
{
    hustdb_ha_bucket_t * arr;
    size_t size;
} hustdb_ha_bucket_table_t;

typedef size_t bucket_index_t;

typedef struct
{
    const char * src;
    const char * dst;
    ngx_bool_t eq; // expected result
} string_pair_t;

ngx_bool_t hustdb_ha_build_hash_table(
    const HustDbHaTable * table,
    const ngx_http_peer_dict_t * peer_dict,
    ngx_pool_t * pool,
    hustdb_ha_table_t * hash_table,
    hustdb_ha_bucket_table_t * bucket_table);

ngx_http_peer_dict_t * hustdb_ha_get_peer_dict();

ngx_bool_t hustdb_ha_str_eq(const char * src, const char * dst);
ngx_bool_t hustdb_ha_precheck_table(const HustDbHaTableElemArray * table);
ngx_bool_t hustdb_ha_check_decrease(const HustDbHaTableElemArray * table);
ngx_bool_t hustdb_ha_review_table(hustdb_ha_table_t * table);
ngx_bool_t hustdb_ha_check_table_item(
    ngx_bool_t include_decrease,
    ngx_bool_t last,
    const HustDbHaTableElem * prev,
    const HustDbHaTableElem * item);
ngx_bool_t hustdb_ha_enable_increase(const HustDbHaTableElem * item);
int hustdb_ha_get_remove_idx(const HustDbHaTableDecr * decr, const HustDbHaTablePair * item);

#endif // __hustdb_ha_table_inner_20161013193359_h__
