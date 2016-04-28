#include "i_kv.h"
#include "leveldb/kv_leveldb.h"
#include "md5db/kv_md5db.h"
#include "kv_array/kv_array.h"
#include "../base.h"

i_kv_t * create_kv ( )
{
    try
    {
        return new kv_leveldb_t ();
    }
    catch ( ... )
    {
        LOG_ERROR ( "[kv]new kv_leveldb_t() exception" );
        return NULL;
    }

    return NULL;
}

i_server_kv_t * create_server_kv ( )
{
    try
    {
        return new kv_md5db_t ();
    }
    catch ( ... )
    {
        LOG_ERROR ( "[kv]new kv_md5db_t() exception" );
        return NULL;
    }

    return NULL;
}
