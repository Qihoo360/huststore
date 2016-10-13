#ifndef __hustdb_ha_utils_inner_20161013210424_h__
#define __hustdb_ha_utils_inner_20161013210424_h__

#include "hustdb_ha_utils.h"

ngx_str_t hustdb_ha_init_dir(const ngx_str_t * prefix, const ngx_str_t * file, ngx_pool_t * pool);
size_t hustdb_ha_get_peer_array_count();
const char * hustdb_ha_get_peer_item_uri(size_t index);

#endif // __hustdb_ha_utils_inner_20161013210424_h__
