#include "kv_md5db.h"
#include "i_kv.h"
#include "bucket.h"
#include "bucket_array.h"
#include "content_array.h"
#include "fullkey_array.h"
#include "conflict_array.h"
#include "fast_conflict_array.h"
#include "../kv_array/kv_array.h"
#include "../../binlog/binlog.h"
#include "../../hustdb.h"

using namespace md5db;

struct query_ctxt_t
{
    std::string   tbkey;
    std::string   value;
    uint32_t      table_len;
    uint32_t      wttl;
    uint32_t      rttl;

    query_ctxt_t ( )
    : tbkey ( )
    , value ( )
    , table_len ( 0 )
    , wttl ( 0 )
    , rttl ( 0 )
    {
        tbkey.reserve ( HASH_TB_LEN * 64 );
    }

    void reset ( )
    {
        tbkey.resize ( 0 );
        value.resize ( 0 );
        table_len = 0;
        wttl = 0;
        rttl = 0;
    }

} __attribute__ ( ( aligned ( 64 ) ) );

typedef std::vector< query_ctxt_t >  query_ctxts_t;

#pragma pack( push, 1 )

struct data_item_for_content_db_t
{
    uint32_t    data_len    : 30;
    uint32_t    _reserved   :  2;
    uint32_t    file_id;
    uint32_t    data_id;

    data_item_for_content_db_t ( )
    : data_len ( 0 )
    , _reserved ( 0 )
    , file_id ( 0 )
    , data_id ( 0 )
    {
    }
} __attribute__ ( ( aligned ( 64 ) ) );

#pragma pack( pop )

struct kv_md5db_t::inner
{
public:

    inner ( )
    : m_buckets ( )
    , m_contents ( )
    , m_fullkeys ( )
    , m_fast_conflicts ( )
    , m_conflicts ( )
    , m_query_ctxts ( )
    , m_data ( )
    , m_binlog ( )
    {
    }

    ~ inner ( )
    {
        m_buckets.close ();
        m_contents.close ();
        m_fullkeys.close ();
        m_fast_conflicts.close ();
        m_conflicts.close ();
        m_data.close ();
    }

    bucket_array_t          m_buckets;
    content_array_t         m_contents;
    fullkey_array_t         m_fullkeys;
    fast_conflict_array_t   m_fast_conflicts;
    conflict_array_t        m_conflicts;
    query_ctxts_t           m_query_ctxts;
    kv_array_t              m_data;
    binlog_t                m_binlog;
} ;

void kv_md5db_t::kill_me ( )
{
    delete this;
}

kv_md5db_t::kv_md5db_t ( )
: m_inner ( NULL )
, m_ok ( false )

, m_perf_put_ok ( )
, m_perf_put_fail ( )
, m_perf_binlog_put_ok ( )
, m_perf_binlog_put_fail ( )
, m_count_put_wrong_version ( 0 )
, m_count_binlog_put_wrong_version ( 0 )

, m_perf_del_ok ( )
, m_perf_del_not_found ( )
, m_perf_del_fail ( )
, m_perf_binlog_del_ok ( )
, m_perf_binlog_del_not_found ( )
, m_perf_binlog_del_fail ( )
, m_count_del_wrong_version ( 0 )
, m_count_binlog_del_wrong_version ( 0 )

, m_perf_get_ok ( )
, m_perf_get_not_found ( )
, m_perf_get_fail ( )

, m_perf_conflict_get ( )
, m_perf_conflict_put ( )
, m_perf_conflict_del ( )
, m_perf_fast_conflict_get ( )
, m_perf_fast_conflict_put ( )
, m_perf_fast_conflict_del ( )

, m_perf_fullkey_get ( )
, m_perf_fullkey_write ( )
, m_perf_fullkey_del ( )

, m_perf_content_get ( )
, m_perf_content_write ( )
, m_perf_content_update ( )
, m_perf_content_del ( )

, m_perf_data_get ( )
, m_perf_data_put ( )
, m_perf_data_del ( )
{
    memset ( m_count_conflicts, 0, sizeof ( m_count_conflicts ) );
}

kv_md5db_t::~ kv_md5db_t ( )
{
    destroy ();
}

bool kv_md5db_t::check_inner_key (
                                   const void *    inner_key,
                                   unsigned int    inner_key_len
                                   )
{
    if ( unlikely ( NULL == m_inner ) )
    {
        LOG_ERROR ( "[md5db][db]m_inner is NULL" );
        return false;
    }
    if ( unlikely ( NULL == inner_key || 16 != inner_key_len ) )
    {
        LOG_INFO ( "[md5db][db]invalid key" );
        return false;
    }

    return true;
}

bool kv_md5db_t::check_user_key (
                                  const void *    user_key,
                                  unsigned int    user_key_len
                                  )
{
    if ( unlikely ( NULL == m_inner ) )
    {
        LOG_ERROR ( "[md5db][db]m_inner is NULL" );
        return false;
    }

    if ( unlikely ( NULL == user_key || 0 == user_key_len || user_key_len > 0xFFFFFF ) )
    {
        LOG_INFO ( "[md5db][db][user_key_ptr=%p][user_key_len=%d]invalid user_key", user_key, user_key_len );
        return false;
    }

    return true;
}

uint32_t kv_md5db_t::add_version ( uint32_t & version )
{
    if ( version < BUCKET_DATA_MAX_VERSION )
    {
        ++ version;
    }
    else
    {
        version = 1;
    }
    return version;
}

void kv_md5db_t::user_key_to_inner (
                                     void *          inner_key,
                                     const void *    user_key,
                                     unsigned int    user_key_len
                                     )
{
    G_APPTOOL->md5 ( user_key, user_key_len, ( char * ) inner_key );
}

void kv_md5db_t::destroy ( )
{
    if ( m_inner )
    {
        try
        {
            delete m_inner;
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][db]delete m_inner exception" );
        }
        m_inner = NULL;
    }
}

bool kv_md5db_t::open ( )
{
    if ( NULL == m_inner )
    {
        try
        {
            m_inner = new inner ();
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][db]new inner() exception" );
            return false;
        }
    }

    hustdb_t * hustdb = ( hustdb_t * ) G_APPTOOL->get_hustdb ();

    m_inner->m_query_ctxts.resize ( 0 );
    try
    {
        m_inner->m_query_ctxts.resize ( hustdb->get_worker_count () + 2 );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[md5db][db]bad_alloc" );
        return false;
    }

    // data
    if ( ! m_inner->m_data.open ( ) )
    {
        LOG_ERROR ( "[md5db][db]kvs open failed" );
        return false;
    }
    LOG_INFO ( "[md5db][db]kvs opened OK" );

    // content
    if ( content_array_t::enable ( HUSTDB_CONFIG ) )
    {
        if ( ! m_inner->m_contents.open ( DB_CONTENTS_DIR, HUSTDB_CONFIG ) )
        {
            LOG_ERROR ( "[md5db][db]contents open failed" );
            return false;
        }
        LOG_INFO ( "[md5db][db]contents opened OK" );
    }
    else
    {
        LOG_INFO ( "[md5db][db]contents disabled" );
    }

    // bucket
    if ( ! m_inner->m_buckets.open ( DB_BUCKETS_DIR ) )
    {
        LOG_ERROR ( "[md5db][db]buckets open failed" );
        return false;
    }
    LOG_INFO ( "[md5db][db]buckets opened OK" );

    // fullkey
    if ( ! m_inner->m_fullkeys.open ( DB_FULLKEY_DIR ) )
    {
        LOG_ERROR ( "[md5db][db]fullkeys create failed" );
        return false;
    }
    LOG_INFO ( "[md5db][db]fullkeys opened OK" );

    m_inner->m_buckets.set_fullkeys ( & m_inner->m_fullkeys );

    // fast conflict
    if ( ! m_inner->m_fast_conflicts.open ( DB_FAST_CONFLICT_DIR, & m_inner->m_buckets, HUSTDB_CONFIG ) )
    {
        LOG_ERROR ( "[md5db][db]fast_conflicts open failed" );
        return false;
    }
    LOG_INFO ( "[md5db][db]fast_conflicts opened OK" );

    // conflict
    if ( ! m_inner->m_conflicts.open ( DB_CONFLICT_DIR, HUSTDB_CONFIG ) )
    {
        LOG_ERROR ( "[md5db][db]conflicts open failed" );
        return false;
    }
    LOG_INFO ( "[md5db][db]conflicts opened OK" );

    // binlog
    if ( ! m_inner->m_binlog.init ( hustdb->get_store_conf ().db_binlog_thread_count,
                                    hustdb->get_store_conf ().db_binlog_queue_capacity,
                                    hustdb->get_store_conf ().db_binlog_queue_capacity,
                                    hustdb->get_server_conf ().http_security_user.c_str (),
                                    hustdb->get_server_conf ().http_security_passwd.c_str ()
                                   )
         )
    {
        LOG_ERROR ( "[md5db][db]binlog open failed" );
        return false;
    }
    LOG_INFO ( "[md5db][db]binlog opened OK" );

    m_inner->m_binlog.set_callback ( binlog_done_callback );

    m_ok = true;

    LOG_INFO ( "[md5db][db]open OK" );

    return true;
}

int kv_md5db_t::flush ( )
{
    return EINVAL;
}

int kv_md5db_t::get_conflict_block_id (
                                        const void *                inner_key,
                                        size_t                      inner_key_len,
                                        md5db::block_id_t &         block_id,
                                        uint32_t &                  version
                                        )
{
    block_id.reset ();
    version     = 0;

    int r;

    {
        scope_perf_target_t perf_fast ( m_perf_fast_conflict_get );
        r = m_inner->m_fast_conflicts.get ( inner_key, ( unsigned int ) inner_key_len, block_id, version );
    }
    if ( 0 == r )
    {
        return 0;
    }
    else if ( r > 0 )
    {
        if ( r != ENOENT )
        {
            LOG_ERROR ( "[md5db][db]m_fast_conflicts.get return %d", r );
        }
        return r;
    }
    // < 1 means need search conflict

    {
        scope_perf_target_t perf ( m_perf_conflict_get );
        r = m_inner->m_conflicts.get ( inner_key, ( unsigned int ) inner_key_len, block_id, version );
    }
    if ( unlikely ( 0 != r ) )
    {
        if ( r != ENOENT )
        {
            LOG_ERROR ( "[md5db][db]m_conflicts.get return %d", r );
        }
        return r;
    }
    return 0;
}

int kv_md5db_t::check_same_key (
                                 bucket_t &                  bucket,
                                 const md5db::block_id_t &   block_id,
                                 const char *                inner_key,
                                 size_t                      inner_key_len,
                                 const char *                user_key,
                                 size_t                      user_key_len
                                 )
{
    scope_perf_target_t perf ( m_perf_fullkey_get );
    bool b = bucket.get_fullkey ()->compare (
                                             block_id,
                                             inner_key,
                                             inner_key_len,
                                             user_key,
                                             user_key_len
                                             );
    if ( b )
    {
        return 0;
    }

    return 1;
}

int kv_md5db_t::set_data (
                           const md5db::block_id_t &   block_id,
                           const char *                user_key,
                           size_t                      user_key_len,
                           const char *                val,
                           size_t                      val_len,
                           conn_ctxt_t                 conn,
                           item_ctxt_t * &             ctxt
                           )
{
    if ( m_inner->m_contents.is_open () )
    {
        return set_data_with_content_db (
                                         block_id,
                                         user_key,
                                         user_key_len,
                                         val,
                                         val_len,
                                         conn,
                                         ctxt );
    }
    else
    {
        return set_data_with_kv_array (
                                       block_id,
                                       user_key,
                                       user_key_len,
                                       val,
                                       val_len,
                                       conn,
                                       ctxt );
    }
}

int kv_md5db_t::add_data_with_content_db (
                                           const md5db::block_id_t &   block_id,
                                           const char *                user_key,
                                           size_t                      user_key_len,
                                           const char *                val,
                                           size_t                      val_len,
                                           conn_ctxt_t                 conn,
                                           item_ctxt_t * &             ctxt
                                           )
{
    int r;

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    uint32_t ttl = tmp_ctxt->wttl;

    uint32_t ukey_len = ( uint32_t ) user_key_len;

    size_t need = val_len + user_key_len + sizeof (uint32_t ) * 2;
    tmp_ctxt->value.resize ( 0 );
    if ( tmp_ctxt->value.capacity () < need )
    {
        try
        {
            tmp_ctxt->value.reserve ( need );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][db]bad_alloc" );
            return ENOMEM;
        }
    }

    // value
    tmp_ctxt->value.assign ( val, val + val_len );
    // key
    tmp_ctxt->value.append ( user_key, user_key + user_key_len );
    // key_len
    tmp_ctxt->value.append ( ( const char * ) & ukey_len, ( ( const char * ) & ukey_len ) + sizeof (uint32_t ) );
    // ttl
    tmp_ctxt->value.append ( ( const char * ) & ttl, ( ( const char * ) & ttl ) + sizeof (uint32_t ) );

    // add to content_db
    uint32_t    content_file_id = 0;
    uint32_t    content_data_id = 0;
    bool b;
    {
        scope_perf_target_t perf ( m_perf_content_write );
        b = m_inner->m_contents.write ( tmp_ctxt->value.c_str (),
                                       ( uint32_t ) tmp_ctxt->value.size (),
                                       content_file_id,
                                       content_data_id );
    }
    if ( unlikely ( ! b || 0 == content_data_id ) )
    {
        LOG_ERROR ( "[md5db][db]m_contents.write failed, content_file_id:%d, content_data_id: %d, size: %d",
                   content_file_id, content_data_id, ( uint32_t ) tmp_ctxt->value.size () );
        return EFAULT;
    }

    data_item_for_content_db_t cdb_data;
    cdb_data.file_id    = content_file_id;
    cdb_data.data_id    = content_data_id;
    cdb_data.data_len   = ( uint32_t ) tmp_ctxt->value.size ();
    {
        scope_perf_target_t perf ( m_perf_data_put );
        r = m_inner->m_data.put_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            ( const char * ) & cdb_data,
                                            sizeof ( cdb_data ),
                                            ttl,
                                            ctxt );
    }

    if ( unlikely ( 0 != r ) )
    {
        LOG_ERROR ( "[md5db][db][user_file=%d][inner_file=%d]m_data.put_from_md5db"
                   "( %d.%u ) return %d",
                   ctxt->user_file_id,
                   ctxt->inner_file_id,
                   ( int ) block_id.bucket_id (), block_id.data_id (), r );
        return r;
    }

    return 0;
}

int kv_md5db_t::set_data_with_content_db (
                                           const md5db::block_id_t &   block_id,
                                           const char *                user_key,
                                           size_t                      user_key_len,
                                           const char *                val,
                                           size_t                      val_len,
                                           conn_ctxt_t                 conn,
                                           item_ctxt_t * &             ctxt
                                           )
{
    int r;

    std::string * rsp = NULL;

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    uint32_t ttl = tmp_ctxt->wttl;

    uint32_t ukey_len = ( uint32_t ) user_key_len;

    // find 
    data_item_for_content_db_t  addr;
    memset ( & addr, 0, sizeof ( addr ) );
    bool                        found = false;
    do
    {
        {
            scope_perf_target_t perf ( m_perf_data_get );
            r = m_inner->m_data.get_from_md5db ( block_id,
                                                ( uint32_t ) ctxt->inner_file_id,
                                                tmp_ctxt->tbkey.c_str (),
                                                tmp_ctxt->table_len,
                                                rsp,
                                                ctxt );
        }
        if ( 0 != r )
        {
            if ( ENOENT != r )
            {
                LOG_ERROR ( "[md5db][db]m_data.get( %u ) return %d", block_id.data_id (), r );
                return r;
            }
            break;
        }
        if ( NULL == rsp )
        {
            LOG_ERROR ( "[md5db][db]err" );
            return EFAULT;
        }
        std::string & s = * rsp;
        if ( s.size () != sizeof ( data_item_for_content_db_t ) )
        {
            LOG_ERROR ( "[md5db][db]invalid size" );
            s.resize ( 0 );
            return EFAULT;
        }

        memcpy ( & addr, s.c_str (), sizeof ( data_item_for_content_db_t ) );
        s.resize ( 0 );
        if ( 0 == addr.data_len )
        {
            LOG_ERROR ( "[md5db][db]invalid size 0" );
            memset ( & addr, 0, sizeof ( addr ) );
            return EFAULT;
        }

        found = true;

    }
    while ( 0 );

    if ( ! found )
    {
        // add new data, I like this
        return add_data_with_content_db (
                                         block_id,
                                         user_key,
                                         user_key_len,
                                         val,
                                         val_len,
                                         conn,
                                         ctxt
                                         );
    }

    // overwrite exist data

    size_t need = val_len + user_key_len + sizeof (uint32_t ) * 2;
    tmp_ctxt->value.resize ( 0 );
    if ( tmp_ctxt->value.capacity () < need )
    {
        try
        {
            tmp_ctxt->value.reserve ( need );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][db]bad_alloc" );
            return ENOMEM;
        }
    }

    // value
    tmp_ctxt->value.assign ( val, val + val_len );
    // key
    tmp_ctxt->value.append ( user_key, user_key + user_key_len );
    // key_len
    tmp_ctxt->value.append ( ( const char * ) & ukey_len, ( ( const char * ) & ukey_len ) + sizeof (uint32_t ) );
    // ttl
    tmp_ctxt->value.append ( ( const char * ) & ttl, ( ( const char * ) & ttl ) + sizeof (uint32_t ) );

    // update to content_db
    uint32_t new_content_file_id = addr.file_id;
    uint32_t new_content_data_id = addr.data_id;
    bool b;
    {
        scope_perf_target_t perf ( m_perf_content_update );
        b = m_inner->m_contents.update ( new_content_file_id,
                                        new_content_data_id,
                                        addr.data_len,
                                        tmp_ctxt->value.c_str (),
                                        ( uint32_t ) tmp_ctxt->value.size () );
    }
    if ( ! b )
    {
        LOG_ERROR ( "[md5db][db]m_contents.update failed" );
        return EFAULT;
    }

    data_item_for_content_db_t cdb_data;
    cdb_data.file_id    = new_content_file_id;
    cdb_data.data_id    = new_content_data_id;
    cdb_data.data_len   = ( uint32_t ) tmp_ctxt->value.size ();
    {
        scope_perf_target_t perf ( m_perf_data_put );
        r = m_inner->m_data.put_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            ( const char * ) & cdb_data,
                                            sizeof ( cdb_data ),
                                            ttl,
                                            ctxt );
    }

    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][db][user_file=%d][inner_file=%d]m_data.put_from_md5db"
                   "( %d.%u ) return %d",
                   ctxt->user_file_id,
                   ctxt->inner_file_id,
                   ( int ) block_id.bucket_id (), block_id.data_id (), r );
        return r;
    }

    return 0;
}

int kv_md5db_t::set_data_with_kv_array (
                                         const md5db::block_id_t &   block_id,
                                         const char *                user_key,
                                         size_t                      user_key_len,
                                         const char *                val,
                                         size_t                      val_len,
                                         conn_ctxt_t                 conn,
                                         item_ctxt_t * &             ctxt
                                         )
{
    int r;

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    uint32_t ttl = tmp_ctxt->wttl;

    uint32_t ukey_len = ( uint32_t ) user_key_len;

    size_t need = val_len + user_key_len + sizeof (uint32_t ) * 2;
    tmp_ctxt->value.resize ( 0 );
    if ( tmp_ctxt->value.capacity () < need )
    {
        try
        {
            tmp_ctxt->value.reserve ( need );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][db]bad_alloc" );
            return ENOMEM;
        }
    }

    // value
    tmp_ctxt->value.assign ( val, val + val_len );
    // key
    tmp_ctxt->value.append ( user_key, user_key + user_key_len );
    // key_len
    tmp_ctxt->value.append ( ( const char * ) & ukey_len, ( ( const char * ) & ukey_len ) + sizeof (uint32_t ) );
    // ttl
    tmp_ctxt->value.append ( ( const char * ) & ttl, ( ( const char * ) & ttl ) + sizeof (uint32_t ) );

    {
        scope_perf_target_t perf ( m_perf_data_put );
        r = m_inner->m_data.put_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            tmp_ctxt->value.c_str (),
                                            tmp_ctxt->value.size (),
                                            ttl,
                                            ctxt );
    }

    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][db][user_file=%d][inner_file=%d]m_data.put_from_md5db"
                   "( %d.%u ) return %d",
                   ctxt->user_file_id,
                   ctxt->inner_file_id,
                   ( int ) block_id.bucket_id (), block_id.data_id (), r );
        return r;
    }

    return 0;
}

int kv_md5db_t::add_conflict_data (
                                    bucket_t &                  bucket,
                                    md5db::bucket_data_item_t * item,
                                    const char *                inner_key,
                                    size_t                      inner_key_len,
                                    const char *                user_key,
                                    size_t                      user_key_len,
                                    const char *                val,
                                    size_t                      val_len,
                                    uint32_t &                  version,
                                    conn_ctxt_t                 conn,
                                    item_ctxt_t * &             ctxt,
                                    bool                        from_binlog
                                    )
{
    uint32_t user_version = version;

    version     = 0;

    if ( unlikely ( ! from_binlog && 0 != user_version ) )
    {
        LOG_ERROR ( "[md5db][db][user_version=%d] user_version must be 0", user_version );
        return EINVAL;
    }

    int r;
    block_id_t      first_block_id;
    uint32_t        first_version;
    block_id_t      second_block_id;
    fullkey_t *     fullkey = bucket.get_fullkey ();
    bool            b;

    if ( unlikely ( NULL == fullkey ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey is NULL" );
        return EFAULT;
    }

    first_block_id  = item->block_id;
    first_version   = item->version;

    // get first block_id 
    char first_inner_key[ 16 ];
    fast_memcpy ( & first_inner_key, inner_key, 3 );
    {
        scope_perf_target_t perf ( m_perf_fullkey_get );
        b = fullkey->get ( first_block_id, & first_inner_key[ 3 ] );
    }
    if ( unlikely ( ! b ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey->get() failed" );
        return EFAULT;
    }

    // second block_id
    {
        scope_perf_target_t perf ( m_perf_fullkey_write );
        b = fullkey->write ( inner_key, inner_key_len, user_key, user_key_len, second_block_id );
    }
    if ( unlikely ( ! b ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey->write() failed" );
        return EFAULT;
    }

    if ( ! from_binlog )
    {
        version         = 1;
    }
    else
    {
        version         = user_version;
    }

    // fast conflict
    block_id_t fast_conflict_addr;
    {
        scope_perf_target_t perf ( m_perf_fast_conflict_put );
        bool b = m_inner->m_fast_conflicts.write (
                                                  first_inner_key,    16,
                                                  first_block_id,     first_version,
                                                  inner_key,          inner_key_len,
                                                  second_block_id,    version,
                                                  fast_conflict_addr
                                                  );
        if ( ! b )
        {
            LOG_ERROR ( "[md5db][db]m_fast_conflicts.write failed" );
            scope_perf_target_t perf ( m_perf_fullkey_del );
            fullkey->del ( second_block_id );
            version = 0;
            return EFAULT;
        }
    }

    // set data
    if ( m_inner->m_contents.is_open () )
    {
        r = add_data_with_content_db (
                                      second_block_id,
                                      user_key,
                                      user_key_len,
                                      val,
                                      val_len,
                                      conn,
                                      ctxt );
    }
    else
    {
        r = set_data_with_kv_array (
                                    second_block_id,
                                    user_key,
                                    user_key_len,
                                    val,
                                    val_len,
                                    conn,
                                    ctxt );
    }
    if ( unlikely ( 0 != r ) )
    {
        version = 0;
        LOG_ERROR ( "[md5db][db]set_data return %d", r );
        return r;
    }

    item->set_conflict_count ( 2 );
    item->block_id  = fast_conflict_addr;
    item->type      = BUCKET_CONFLICT_DATA;

    LOG_DEBUG ( "[md5db][db][first.block_id=%u][first.version=%u][second.block_id=%u][second_version=%u]put direct to conflict ok",
               first_block_id.data_id (), first_version, second_block_id.data_id (), version );

    return 0;
}

int kv_md5db_t::new_data (
                           bucket_t &                  bucket,
                           md5db::bucket_data_item_t * item,
                           const char *                inner_key,
                           size_t                      inner_key_len,
                           const char *                user_key,
                           size_t                      user_key_len,
                           const char *                val,
                           size_t                      val_len,
                           uint32_t &                  version,
                           conn_ctxt_t                 conn,
                           item_ctxt_t * &             ctxt,
                           bool                        from_binlog
                           )
{
    block_id_t      block_id;
    int             r;
    uint32_t        user_version = version;
    bool            b;

    version = 0;

    if ( unlikely ( ! from_binlog && 0 != user_version ) )
    {
        LOG_DEBUG ( "[md5db][db][user_version=%d] user_version must be 0", user_version );
        return EINVAL;
    }

    // second block_id
    fullkey_t * fullkey = bucket.get_fullkey ();
    if ( unlikely ( NULL == fullkey ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey is NULL" );
        return EFAULT;
    }
    {
        scope_perf_target_t perf ( m_perf_fullkey_write );
        b = fullkey->write ( inner_key, inner_key_len, user_key, user_key_len, block_id );
    }
    if ( unlikely ( ! b ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey->write() failed" );
        return EFAULT;
    }

    if ( m_inner->m_contents.is_open () )
    {
        r = add_data_with_content_db (
                                      block_id,
                                      user_key,
                                      user_key_len,
                                      val,
                                      val_len,
                                      conn,
                                      ctxt );
    }
    else
    {
        r = set_data_with_kv_array (
                                    block_id,
                                    user_key,
                                    user_key_len,
                                    val,
                                    val_len,
                                    conn,
                                    ctxt );
    }
    if ( unlikely ( 0 != r ) )
    {
        LOG_ERROR ( "[md5db][db]set_data return %d", r );
        scope_perf_target_t perf ( m_perf_fullkey_del );
        fullkey->del ( block_id );
        return r;
    }

    item->block_id  = block_id;
    item->type      = BUCKET_DIRECT_DATA;
    if ( ! from_binlog )
    {
        item->version   = 1;
    }
    else
    {
        item->version   = user_version;
    }
    version         = item->version;

    LOG_DEBUG ( "[md5db][db][block_id=%u][version=%u] ADD_NEW direct ok",
               item->block_id.bucket_id (), item->version );
    return 0;
}

int kv_md5db_t::set_conflict_data (
                                    bucket_t &                  bucket,
                                    md5db::bucket_data_item_t * item,
                                    const char *                inner_key,
                                    size_t                      inner_key_len,
                                    const char *                user_key,
                                    size_t                      user_key_len,
                                    const char *                val,
                                    size_t                      val_len,
                                    uint32_t &                  version,
                                    bool &                      is_found,
                                    conn_ctxt_t                 conn,
                                    item_ctxt_t * &             ctxt,
                                    bool                        from_binlog
                                    )
{
    int         r;
    bool        b;
    uint32_t    user_version;
    block_id_t  block_id;
    bool        found_in_fast_conflict = false;

    user_version = version;
    version      = 0;
    is_found     = false;

    // find
    do
    {
        {
            scope_perf_target_t perf_fast ( m_perf_fast_conflict_get );
            r = m_inner->m_fast_conflicts.get ( inner_key, ( unsigned int ) inner_key_len, block_id, version );
        }
        if ( 0 == r )
        {
            found_in_fast_conflict = true;
            break;
        }
        else if ( r > 0 )
        {
            break;
        }
        // < 0 means need search conflict

        scope_perf_target_t perf ( m_perf_conflict_get );
        r = m_inner->m_conflicts.get ( inner_key, ( unsigned int ) inner_key_len, block_id, version );
    }
    while ( 0 );

    if ( 0 != r )
    {
        if ( r != ENOENT )
        {
            LOG_ERROR ( "[md5db][db]m_conflicts.get return %d", r );
            return r;
        }

        // not found

        version = 0;

        if ( ! from_binlog && 0 != user_version )
        {
            LOG_ERROR ( "[md5db][db][user_version=%d] user_version must be 0", user_version );
            return EINVAL;
        }

        // fullkey
        fullkey_t * fullkey = bucket.get_fullkey ();
        if ( NULL == fullkey )
        {
            LOG_ERROR ( "[md5db][db]fullkey is NULL" );
            return EFAULT;
        }
        {
            scope_perf_target_t perf ( m_perf_fullkey_write );
            b = fullkey->write ( inner_key, inner_key_len, user_key, user_key_len, block_id );
        }
        if ( ! b )
        {
            LOG_ERROR ( "[md5db][db]fullkey->write() failed" );
            return EFAULT;
        }

        if ( m_inner->m_contents.is_open () )
        {
            r = add_data_with_content_db (
                                          block_id,
                                          user_key,
                                          user_key_len,
                                          val,
                                          val_len,
                                          conn,
                                          ctxt );
        }
        else
        {
            r = set_data_with_kv_array (
                                        block_id,
                                        user_key,
                                        user_key_len,
                                        val,
                                        val_len,
                                        conn,
                                        ctxt );
        }
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][db]set_data return %d", r );
            scope_perf_target_t perf ( m_perf_fullkey_del );
            fullkey->del ( block_id );
            return r;
        }

        version = 1;
        if ( from_binlog )
        {
            version = user_version;
        }

        // conflict
        do
        {
            {
                scope_perf_target_t perf ( m_perf_fast_conflict_put );
                r = m_inner->m_fast_conflicts.add ( inner_key, inner_key_len, block_id, version );
                if ( 0 == r )
                {
                    break;
                }
                else if ( r > 0 )
                {
                    LOG_ERROR ( "[md5db][db][r=%d]m_fast_conflicts.add failed", r );
                    return EFAULT;
                }
                // < 0 means need write to conflict
            }

            scope_perf_target_t perf ( m_perf_conflict_put );
            r = m_inner->m_conflicts.put ( inner_key, ( unsigned int ) inner_key_len, block_id, version );
        }
        while ( 0 );
        if ( 0 != r )
        {
            version = 0;
            LOG_ERROR ( "[md5db][db]conflicts.put return %d", r );
            return r;
        }

        ctxt->kv_type = NEW_KV;

        // in conflict, item->version is count of conflict item
        if ( ! item->add_conflict_count () )
        {
            LOG_ERROR ( "[md5db][db][block_id=%u][version=%u]ADD conflict count failed",
                       block_id.data_id (), version );
            return EFAULT;
        }

        {
            uint32_t c = item->get_conflict_count ();
            if ( c >= COUNT_OF ( m_count_conflicts ) )
            {
                c = COUNT_OF ( m_count_conflicts ) - 1;
            }
            else
            {
                if ( m_count_conflicts[ c - 1 ] > 0 )
                {
                    // check < 0, not lock
                    size_t t = m_count_conflicts[ c - 1 ] - 1;
                    if ( t < m_count_conflicts[ c - 1 ] )
                    {
                        m_count_conflicts[ c - 1 ] = t;
                    }
                }
            }
            ++ m_count_conflicts[ c ];
        }

        LOG_DEBUG ( "[md5db][db][block_id=%u][version=%u][conflict_count=%u]add conflict ok",
                   block_id.data_id (), version, item->get_conflict_count () );

        return 0;
    }

    // found

    is_found = true;
    if ( 0 != user_version )
    {
        if ( ! from_binlog )
        {
            // NOT binlog
            if ( user_version != version )
            {
                LOG_ERROR ( "[md5db][db][put]version not match user=%u,%u",
                           user_version, version );
                ctxt->is_version_error = true;
                return EINVAL;
            }
        }
        else
        {
            // binlog
            if ( user_version <= version )
            {
                LOG_DEBUG ( "[md5db][db][put][binlog]version too low user=%u,%u",
                           user_version, version );
                ctxt->is_version_error = true;
                return 0;
            }
        }
    }
    r = set_data (
                  block_id,
                  user_key,
                  user_key_len,
                  val,
                  val_len,
                  conn,
                  ctxt );
    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][db]set_data return %d", r );
        return r;
    }

    uint32_t t;
    if ( ! from_binlog )
    {
        t = version;
        add_version ( t );
    }
    else
    {
        t = user_version;
    }

    // conflict
    if ( found_in_fast_conflict )
    {
        scope_perf_target_t perf ( m_perf_fast_conflict_put );
        r = m_inner->m_fast_conflicts.update ( inner_key, ( unsigned int ) inner_key_len, block_id, t );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][db]fast_conflicts.put return %d", r );
            return r;
        }
    }
    else
    {
        scope_perf_target_t perf ( m_perf_conflict_put );
        r = m_inner->m_conflicts.put ( inner_key, ( unsigned int ) inner_key_len, block_id, t );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][db]conflicts.put return %d", r );
            return r;
        }
    }
    version = t;

    LOG_DEBUG ( "[md5db][db][block_id=%u][version=%u][conflict_count=%u]update conflict ok",
               block_id.data_id (), version, item->get_conflict_count () );

    return 0;
}

int kv_md5db_t::get_data (
                           const md5db::block_id_t &   block_id,
                           const char *                user_key,
                           size_t                      user_key_len,
                           conn_ctxt_t                 conn,
                           std::string * &             rsp,
                           item_ctxt_t * &             ctxt
                           )
{
    if ( m_inner->m_contents.is_open () )
    {
        return get_data_with_content_db (
                                         block_id,
                                         user_key,
                                         user_key_len,
                                         conn,
                                         rsp,
                                         ctxt );
    }
    else
    {
        return get_data_without_content_db (
                                            block_id,
                                            user_key,
                                            user_key_len,
                                            conn,
                                            rsp,
                                            ctxt );
    }
}

int kv_md5db_t::get_data_without_content_db (
                                              const md5db::block_id_t &   block_id,
                                              const char *                user_key,
                                              size_t                      user_key_len,
                                              conn_ctxt_t                 conn,
                                              std::string * &             rsp,
                                              item_ctxt_t * &             ctxt
                                              )
{
    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int         r;
    {
        scope_perf_target_t perf ( m_perf_data_get );
        r = m_inner->m_data.get_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            rsp,
                                            ctxt );
    }
    if ( 0 != r )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[md5db][db]m_data.get( %u ) return %d", block_id.data_id (), r );
        }
        return r;
    }

    if ( NULL == rsp )
    {
        LOG_ERROR ( "[md5db][db]err" );
        return EFAULT;
    }

    std::string & s = * rsp;

    uint8_t idx_len = sizeof ( uint32_t ) * 2;
    if ( s.size () <= idx_len )
    {
        LOG_ERROR ( "[md5db][db]err" );
        return EFAULT;
    }

    uint32_t ttl;
    fast_memcpy ( & ttl, & s[ s.size () - sizeof (uint32_t ) ], sizeof (uint32_t ) );
    tmp_ctxt->rttl = ttl;

    uint32_t ukey_len;
    fast_memcpy ( & ukey_len, & s[ s.size () - idx_len ], sizeof (uint32_t ) );
    if ( 0 == ukey_len )
    {
        LOG_ERROR ( "[md5db][db]GOD! invalid user key len" );
        return EFAULT;
    }
    if ( ( int ) user_key_len != ( int ) ukey_len )
    {
        LOG_ERROR ( "[md5db][db]GOD! user_key_len=%d, ukey_len=%d", ( int ) user_key_len, ( int ) ukey_len );
        return EFAULT;
    }

    if ( s.size () < idx_len + ukey_len )
    {
        LOG_ERROR ( "[md5db][db]GOD! invalid data_len %d, ukey_len=%d", ( int ) s.size (), ( int ) ukey_len );
        return EFAULT;
    }

    const char * ukey;
    ukey = ( const char * ) & s[ s.size () - idx_len - ukey_len ];
    if ( ! mem_equal ( ukey, user_key, user_key_len ) )
    {
        LOG_ERROR ( "[md5db][db]GOD! user key not match!" );
        return EFAULT;
    }
    s.resize ( s.size () - idx_len - ukey_len );

    return 0;
}

int kv_md5db_t::get_data_with_content_db (
                                           const md5db::block_id_t &   block_id,
                                           const char *                user_key,
                                           size_t                      user_key_len,
                                           conn_ctxt_t                 conn,
                                           std::string * &             rsp,
                                           item_ctxt_t * &             ctxt
                                           )
{
    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int         r;
    {
        scope_perf_target_t perf ( m_perf_data_get );
        r = m_inner->m_data.get_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            rsp,
                                            ctxt );
    }
    if ( 0 != r )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[md5db][db]m_data.get( %u ) return %d", block_id.data_id (), r );
        }
        return r;
    }
    if ( NULL == rsp )
    {
        LOG_ERROR ( "[md5db][db]err" );
        return EFAULT;
    }
    std::string & s = * rsp;
    if ( s.size () != sizeof ( data_item_for_content_db_t ) )
    {
        LOG_ERROR ( "[md5db][db]invalid size" );
        s.resize ( 0 );
        return EFAULT;
    }

    data_item_for_content_db_t addr;
    memcpy ( & addr, s.c_str (), sizeof ( data_item_for_content_db_t ) );
    s.resize ( 0 );
    if ( 0 == addr.data_len )
    {
        LOG_ERROR ( "[md5db][db]invalid size 0" );
        return EFAULT;
    }

    try
    {
        s.resize ( addr.data_len );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[md5db][db]bad_alloc" );
        s.resize ( 0 );
        return EFAULT;
    }

    bool b;
    {
        scope_perf_target_t perf ( m_perf_content_get );
        b = m_inner->m_contents.get ( addr.file_id, addr.data_id, addr.data_len, & s[ 0 ] );
    }
    if ( ! b )
    {
        LOG_ERROR ( "[md5db][db]get content failed" );
        s.resize ( 0 );
        return EFAULT;

    }

    uint8_t idx_len = sizeof ( uint32_t ) * 2;
    if ( s.size () <= idx_len )
    {
        LOG_ERROR ( "[md5db][db]err" );
        return EFAULT;
    }

    uint32_t ttl;
    fast_memcpy ( & ttl, & s[ s.size () - sizeof (uint32_t ) ], sizeof (uint32_t ) );
    tmp_ctxt->rttl = ttl;

    uint32_t ukey_len;
    fast_memcpy ( & ukey_len, & s[ s.size () - idx_len ], sizeof (uint32_t ) );
    if ( 0 == ukey_len )
    {
        LOG_ERROR ( "[md5db][db]GOD! invalid user key len" );
        return EFAULT;
    }
    if ( ( int ) user_key_len != ( int ) ukey_len )
    {
        LOG_ERROR ( "[md5db][db]GOD! user_key_len=%d, ukey_len=%d", ( int ) user_key_len, ( int ) ukey_len );
        return EFAULT;
    }

    if ( s.size () < idx_len + ukey_len )
    {
        LOG_ERROR ( "[md5db][db]GOD! invalid data_len %d, ukey_len=%d", ( int ) s.size (), ( int ) ukey_len );
        return EFAULT;
    }

    const char * ukey;
    ukey = ( const char * ) & s[ s.size () - idx_len - ukey_len ];
    if ( ! mem_equal ( ukey, user_key, user_key_len ) )
    {
        LOG_ERROR ( "[md5db][db]GOD! user key not match!" );
        return EFAULT;
    }
    s.resize ( s.size () - idx_len - ukey_len );

    return 0;
}

int kv_md5db_t::exists (
                         const char *        user_key,
                         size_t              user_key_len,
                         uint32_t &          version,
                         conn_ctxt_t         conn,
                         item_ctxt_t * &     ctxt
                         )
{
    version = 0;
    ctxt    = NULL;

    char        inner_key[ 16 ];
    size_t      inner_key_len   = 16;
    block_id_t block_id;

    if ( unlikely ( ! check_user_key ( user_key, user_key_len ) ) )
    {
        LOG_INFO ( "[md5db][db]invalid user key" );
        return EINVAL;
    }

    size_t new_user_key_len = user_key_len;
    const char * new_user_key = get_inner_tbkey ( user_key, new_user_key_len, conn );
    user_key_to_inner ( inner_key, new_user_key, new_user_key_len );

    bucket_t & bucket = m_inner->m_buckets.get_bucket ( inner_key, inner_key_len );

    bucket_data_item_t *    item    = NULL;
    int r = bucket.find ( inner_key, inner_key_len, item );
    if ( unlikely ( NULL == item ) )
    {
        LOG_ERROR ( "[md5db][db]find return %d, NULL", r );
        if ( 0 == r )
        {
            r = EFAULT;
        }
        return r;
    }

    scope_rlock_t lock( bucket.get_lock() );

    switch ( item->type )
    {
        case BUCKET_DIRECT_DATA:
            r = check_same_key ( bucket, item->block_id, inner_key, inner_key_len, user_key, user_key_len );
            if ( r < 0 )
            {
                LOG_ERROR ( "[md5db][db]check_same_key return %d", r );
                r = EFAULT;
                break;
            }
            if ( 0 == r )
            {
                // same key
                version = item->version;

            }
            else
            {
                // not same key, not found
                r = ENOENT;
                break;
            }
            r = 0;
            break;

        case BUCKET_CONFLICT_DATA:
            r = get_conflict_block_id ( inner_key, inner_key_len, block_id, version );
            if ( 0 != r )
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]invalid CONFLICT_DATA, r = %d, block_id = %u", r, block_id.data_id () );
                    r = EFAULT;
                }

                break;
            }
            break;

        case BUCKET_NO_DATA:
            r = ENOENT;
            break;

        default:
            LOG_ERROR ( "[md5db][db]invalid item->type %d", ( int ) item->type );
            r = EFAULT;
            break;
    }

    return r;
}

#define PERF_GET_CANCEL()                       \
    check_get_ok.cancel();                      \
    check_get_not_found.cancel();               \
    check_get_fail.cancel();

#define PERF_GET_OK()                           \
    check_get_not_found.cancel();               \
    check_get_fail.cancel();

#define PERF_GET_FAIL()                         \
    check_get_ok.cancel();                      \
    check_get_not_found.cancel();

#define PERF_GET_NOT_FOUND()                    \
    check_get_ok.cancel();                      \
    check_get_fail.cancel();

int kv_md5db_t::get (
                      const char *        user_key,
                      size_t              user_key_len,
                      uint32_t &          version,
                      conn_ctxt_t         conn,
                      std::string * &     rsp,
                      item_ctxt_t * &     ctxt
                      )
{
    version = 0;
    rsp     = NULL;
    ctxt    = NULL;

    scope_perf_target_t check_get_ok (          m_perf_get_ok );
    scope_perf_target_t check_get_not_found (   m_perf_get_not_found );
    scope_perf_target_t check_get_fail (        m_perf_get_fail );

    char        inner_key[ 16 ];
    size_t      inner_key_len   = 16;
    block_id_t  block_id;

    if ( unlikely ( ! check_user_key ( user_key, user_key_len ) ) )
    {
        LOG_INFO ( "[md5db][db]invalid user key" );
        PERF_GET_CANCEL ();
        return EINVAL;
    }

    size_t new_user_key_len = user_key_len;
    const char * new_user_key = get_inner_tbkey ( user_key, new_user_key_len, conn );
    user_key_to_inner ( inner_key, new_user_key, new_user_key_len );

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int r = 0;
    if ( tmp_ctxt->tbkey.empty () )
    {
        r = m_inner->m_data.hash_with_md5db ( inner_key, inner_key_len, conn, ctxt );
    }
    else if ( tmp_ctxt->table_len > ZSET_SCORE_LEN &&
              tmp_ctxt->tbkey.at ( tmp_ctxt->table_len - ZSET_SCORE_LEN - 1 ) == ZSET_TB
              )
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len - ZSET_SCORE_LEN, conn, ctxt );
    }
    else
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len, conn, ctxt );
    }

    if ( unlikely ( r < 0 ) )
    {
        LOG_ERROR ( "[md5db][db]hash failed" );
        PERF_GET_FAIL ();
        return r;
    }

    // get bucket
    bucket_t & bucket = m_inner->m_buckets.get_bucket ( inner_key, inner_key_len );

    bucket_data_item_t *    item    = NULL;
    r = bucket.find ( inner_key, inner_key_len, item );
    if ( unlikely ( NULL == item ) )
    {
        LOG_ERROR ( "[md5db][db]find return %d, NULL", r );
        if ( 0 == r )
        {
            r = EFAULT;
        }
        PERF_GET_FAIL ();
        return r;
    }

    scope_rlock_t lock( bucket.get_lock() );

    switch ( item->type )
    {
        case BUCKET_DIRECT_DATA:
            r = check_same_key ( bucket, item->block_id, inner_key, inner_key_len, user_key, user_key_len );
            if ( r < 0 )
            {
                LOG_ERROR ( "[md5db][db]check_same_key return %d", r );
                r = EFAULT;
                PERF_GET_FAIL ();
                break;
            }
            if ( 0 == r )
            {
                // same key
                version = item->version;
                r = get_data (
                              item->block_id,
                              user_key,
                              user_key_len,
                              conn,
                              rsp,
                              ctxt );
                if ( 0 != r )
                {
                    LOG_ERROR ( "[md5db][db]get_data return %d", r );
                    PERF_GET_FAIL ();
                    break;
                }
                PERF_GET_OK ();

            }
            else
            {
                // not same key, not found
                r = ENOENT;
                PERF_GET_NOT_FOUND ();
                break;
            }
            r = 0;
            break;

        case BUCKET_CONFLICT_DATA:
            r = get_conflict_block_id ( inner_key, inner_key_len, block_id, version );
            if ( 0 != r )
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]invalid CONFLICT_DATA, r = %d, block_id = %u",
                               r, block_id.data_id () );
                    r = EFAULT;
                    PERF_GET_FAIL ();
                }
                else
                {
                    PERF_GET_NOT_FOUND ();
                }
                break;
            }

            r = get_data (
                          block_id,
                          user_key,
                          user_key_len,
                          conn,
                          rsp,
                          ctxt );
            if ( 0 != r )
            {
                LOG_ERROR ( "[md5db][db]get_data return %d", r );
                if ( ENOENT == r )
                {
                    r = EFAULT;
                }
                PERF_GET_FAIL ();
                break;
            }
            PERF_GET_OK ();
            break;

        case BUCKET_NO_DATA:
            r = ENOENT;
            PERF_GET_NOT_FOUND ();
            break;

        default:
            LOG_ERROR ( "[md5db][db]invalid item->type %d", ( int ) item->type );
            r = EFAULT;
            PERF_GET_FAIL ();
            break;
    }

    return r;
}

int kv_md5db_t::delete_conflict_data (
                                       bucket_t &                  bucket,
                                       md5db::bucket_data_item_t * item,
                                       const void *                inner_key,
                                       size_t                      inner_key_len,
                                       uint32_t &                  version,
                                       conn_ctxt_t                 conn,
                                       item_ctxt_t * &             ctxt,
                                       bool                        from_binlog
                                       )
{
    int         r;
    block_id_t  block_id;

    r = del_conflict_block_id ( bucket,
                               item,
                               inner_key,
                               inner_key_len,
                               ctxt,
                               block_id,
                               version,
                               from_binlog );
    if ( unlikely ( 0 != r ) )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[md5db][db]del_conflict_block_id return r = %d, block_id = %u",
                       r, item->block_id.data_id () );
        }
        return r;
    }

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    if ( m_inner->m_contents.is_open () )
    {

        std::string * rsp = NULL;
        data_item_for_content_db_t  addr;
        memset ( & addr, 0, sizeof ( addr ) );
        bool                        found = false;
        do
        {
            {
                scope_perf_target_t perf ( m_perf_data_get );
                r = m_inner->m_data.get_from_md5db ( block_id,
                                                    ( uint32_t ) ctxt->inner_file_id,
                                                    tmp_ctxt->tbkey.c_str (),
                                                    tmp_ctxt->table_len,
                                                    rsp,
                                                    ctxt );
            }
            if ( 0 != r )
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]m_data.get( %u ) return %d", block_id.data_id (), r );
                    return r;
                }
                break;
            }
            if ( NULL == rsp )
            {
                LOG_ERROR ( "[md5db][db]err" );
                return EFAULT;
            }
            std::string & s = * rsp;
            if ( s.size () != sizeof ( data_item_for_content_db_t ) )
            {
                LOG_ERROR ( "[md5db][db]invalid size" );
                s.resize ( 0 );
                return EFAULT;
            }

            memcpy ( & addr, s.c_str (), sizeof ( data_item_for_content_db_t ) );
            s.resize ( 0 );
            if ( 0 == addr.data_len )
            {
                LOG_ERROR ( "[md5db][db]invalid size 0" );
                memset ( & addr, 0, sizeof ( addr ) );
                return EFAULT;
            }

            //content_block_id.set( addr.bucket_id, addr.data_id );
            found = true;

            bool b;
            {
                scope_perf_target_t perf ( m_perf_content_del );
                b = m_inner->m_contents.del ( addr.file_id, addr.data_id, addr.data_len );
            }
            if ( ! b )
            {
                LOG_ERROR ( "[md5db][db]contents.del failed" );
                return EFAULT;
            }

            scope_perf_target_t perf ( m_perf_data_del );
            r = m_inner->m_data.del_from_md5db ( block_id,
                                                ( uint32_t ) ctxt->inner_file_id,
                                                tmp_ctxt->tbkey.c_str (),
                                                tmp_ctxt->table_len,
                                                ctxt );
            if ( 0 != r )
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]m_data.del( %u ) failed", block_id.data_id () );
                }
            }

        }
        while ( 0 );

    }
    else
    {
        scope_perf_target_t perf ( m_perf_data_del );
        r = m_inner->m_data.del_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            ctxt );
        if ( 0 != r )
        {
            if ( ENOENT != r )
            {
                LOG_ERROR ( "[md5db][db]m_data.del( %u ) failed", block_id.data_id () );
            }
        }
    }

    return r;
}

int kv_md5db_t::del_conflict_block_id (
                                        bucket_t &                  bucket,
                                        md5db::bucket_data_item_t * item,
                                        const void *                inner_key,
                                        size_t                      inner_key_len,
                                        item_ctxt_t * &             ctxt,
                                        md5db::block_id_t &         deleted_block_id,
                                        uint32_t &                  deleted_version,
                                        bool                        from_binlog
                                        )
{
    uint32_t user_version = deleted_version;

    deleted_block_id.reset ();
    deleted_version     = 0;

    int     r;
    bool    b;
    bool    found_in_fast_conflict = false;

    // find
    do
    {
        {
            scope_perf_target_t perf_fast ( m_perf_fast_conflict_get );
            r = m_inner->m_fast_conflicts.get ( inner_key,
                                               ( unsigned int ) inner_key_len,
                                               deleted_block_id,
                                               deleted_version );
        }
        if ( 0 == r )
        {
            found_in_fast_conflict = true;
            break;
        }
        else if ( r > 0 )
        {
            break;
        }
        // < 0 means need search conflict

        scope_perf_target_t perf ( m_perf_conflict_get );
        r = m_inner->m_conflicts.get ( inner_key,
                                      ( unsigned int ) inner_key_len,
                                      deleted_block_id,
                                      deleted_version );
    }
    while ( 0 );
    if ( 0 != r )
    {
        if ( r != ENOENT )
        {
            LOG_ERROR ( "[md5db][db]m_conflicts.get return %d", r );
        }
        return r;
    }

    if ( 0 != user_version )
    {
        if ( ! from_binlog )
        {
            // NOT binlog
            if ( user_version != deleted_version )
            {
                LOG_DEBUG ( "[md5db][db][del]version not match user=%u,%u",
                           user_version, deleted_version );
                ctxt->is_version_error = true;
                return EINVAL;
            }
        }
        else
        {
            // binlog, del user_version always = deleted_version
            if ( user_version < deleted_version )
            {
                LOG_DEBUG ( "[md5db][db][del][binlog]version too low user=%u,%u",
                           user_version, deleted_version );
                ctxt->is_version_error = true;
                return 0;
            }
        }
    }

    // del
    if ( found_in_fast_conflict )
    {
        scope_perf_target_t perf ( m_perf_fast_conflict_del );
        r = m_inner->m_fast_conflicts.del ( inner_key, ( unsigned int ) inner_key_len );
    }
    else
    {
        scope_perf_target_t perf ( m_perf_conflict_del );
        r = m_inner->m_conflicts.del ( inner_key, ( unsigned int ) inner_key_len );
    }

    if ( 0 != r )
    {
        if ( r != ENOENT )
        {
            LOG_ERROR ( "[md5db][db]m_conflicts.del return %d", r );
        }
        //return r;
    }

    r = 0;

    // delete fullkey
    fullkey_t * fullkey = bucket.get_fullkey ();
    if ( fullkey )
    {
        scope_perf_target_t perf ( m_perf_fullkey_del );
        b = fullkey->del ( deleted_block_id );
        if ( ! b )
        {
            LOG_ERROR ( "[md5db][db]fullkey->del(%u) failed", deleted_block_id.data_id () );
            //return EFAULT;
        }
    }
    else
    {
        LOG_ERROR ( "fullkey is NULL!!!!!!!!!!!!!!!!!!!" );
        r = EFAULT;
    }

    if ( 0 == item->dec_conflict_count () )
    {
        item->type = BUCKET_NO_DATA;
    }

    return r;
}

int kv_md5db_t::delete_direct_data (
                                     bucket_t &                  bucket,
                                     const md5db::block_id_t &   block_id,
                                     conn_ctxt_t                 conn,
                                     item_ctxt_t * &             ctxt
                                     )
{
    bool    b;

    fullkey_t * fullkey = bucket.get_fullkey ();
    if ( unlikely ( NULL == fullkey ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey is NULL" );
        ctxt = NULL;
        return EFAULT;
    }
    {
        scope_perf_target_t perf ( m_perf_fullkey_del );
        b = fullkey->del ( block_id );
    }
    if ( unlikely ( ! b ) )
    {
        LOG_ERROR ( "[md5db][db]fullkey->del(%u) failed", block_id.data_id () );
        ctxt = NULL;
        return EFAULT;
    }

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int r;
    if ( m_inner->m_contents.is_open () )
    {
        bool found          = false;
        std::string * rsp   = NULL;

        data_item_for_content_db_t  addr;
        memset ( & addr, 0, sizeof ( addr ) );

        do
        {
            {
                scope_perf_target_t perf ( m_perf_data_get );
                r = m_inner->m_data.get_from_md5db ( block_id,
                                                    ( uint32_t ) ctxt->inner_file_id,
                                                    tmp_ctxt->tbkey.c_str (),
                                                    tmp_ctxt->table_len,
                                                    rsp,
                                                    ctxt );
            }
            if ( 0 != r )
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]m_data.get( %u ) return %d", block_id.data_id (), r );
                    return r;
                }
                break;
            }
            if ( NULL == rsp )
            {
                LOG_ERROR ( "[md5db][db]err" );
                return EFAULT;
            }
            std::string & s = * rsp;
            if ( s.size () != sizeof ( data_item_for_content_db_t ) )
            {
                LOG_ERROR ( "[md5db][db]invalid size" );
                s.resize ( 0 );
                return EFAULT;
            }

            memcpy ( & addr, s.c_str (), sizeof ( data_item_for_content_db_t ) );
            s.resize ( 0 );
            if ( 0 == addr.data_len )
            {
                LOG_ERROR ( "[md5db][db]invalid size 0" );
                memset ( & addr, 0, sizeof ( addr ) );
                return EFAULT;
            }

            //content_block_id.set( addr.bucket_id, addr.data_id );
            found = true;

            bool b;
            {
                scope_perf_target_t perf ( m_perf_content_del );
                b = m_inner->m_contents.del ( addr.file_id, addr.data_id, addr.data_len );
            }
            if ( ! b )
            {
                LOG_ERROR ( "[md5db][db]contents.del failed" );
                return EFAULT;
            }

            scope_perf_target_t perf ( m_perf_data_del );
            r = m_inner->m_data.del_from_md5db ( block_id,
                                                ( uint32_t ) ctxt->inner_file_id,
                                                tmp_ctxt->tbkey.c_str (),
                                                tmp_ctxt->table_len,
                                                ctxt );
            if ( 0 != r )
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]m_data.del( %u ) failed", block_id.data_id () );
                }
            }

        }
        while ( 0 );

    }
    else
    {
        scope_perf_target_t perf ( m_perf_data_del );
        r = m_inner->m_data.del_from_md5db ( block_id,
                                            ( uint32_t ) ctxt->inner_file_id,
                                            tmp_ctxt->tbkey.c_str (),
                                            tmp_ctxt->table_len,
                                            ctxt );
        if ( 0 != r )
        {
            if ( ENOENT != r )
            {
                LOG_ERROR ( "[md5db][db]m_data.del( %u ) failed", block_id.data_id () );
            }
        }
    }

    return r;
}

#define PERF_DEL_CANCEL()                       \
    if ( from_binlog )                          \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_not_found.cancel();           \
        check_del_fail.cancel();                \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_not_found.cancel();    \
        check_binlog_del_fail.cancel();         \
    }                                           \
    else                                        \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_not_found.cancel();           \
        check_del_fail.cancel();                \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_not_found.cancel();    \
        check_binlog_del_fail.cancel();         \
    }

#define PERF_DEL_OK()                           \
    if ( from_binlog )                          \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_not_found.cancel();           \
        check_del_fail.cancel();                \
        check_binlog_del_not_found.cancel();    \
        check_binlog_del_fail.cancel();         \
    }                                           \
    else                                        \
    {                                           \
        check_del_not_found.cancel();           \
        check_del_fail.cancel();                \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_not_found.cancel();    \
        check_binlog_del_fail.cancel();         \
    }

#define PERF_DEL_WRONG_VERSION()                \
    PERF_DEL_CANCEL();                          \
    if ( from_binlog )                          \
    {                                           \
        ++ m_count_binlog_del_wrong_version;    \
    }                                           \
    else                                        \
    {                                           \
        ++ m_count_del_wrong_version;           \
    }

#define PERF_DEL_FAIL()                         \
    if ( from_binlog )                          \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_not_found.cancel();           \
        check_del_fail.cancel();                \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_not_found.cancel();    \
    }                                           \
    else                                        \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_not_found.cancel();           \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_not_found.cancel();    \
        check_binlog_del_fail.cancel();         \
    }

#define PERF_DEL_NOT_FOUND()                    \
    if ( from_binlog )                          \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_not_found.cancel();           \
        check_del_fail.cancel();                \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_fail.cancel();         \
    }                                           \
    else                                        \
    {                                           \
        check_del_ok.cancel();                  \
        check_del_fail.cancel();                \
        check_binlog_del_ok.cancel();           \
        check_binlog_del_not_found.cancel();    \
        check_binlog_del_fail.cancel();         \
    }

int kv_md5db_t::del_inner (
                            const char *        user_key,
                            size_t              user_key_len,
                            uint32_t &          version,
                            conn_ctxt_t         conn,
                            item_ctxt_t * &     ctxt,
                            bool                from_binlog
                            )
{
    scope_perf_target_t check_del_ok (               m_perf_del_ok );
    scope_perf_target_t check_del_not_found (        m_perf_del_not_found );
    scope_perf_target_t check_del_fail (             m_perf_del_fail );
    scope_perf_target_t check_binlog_del_ok (        m_perf_binlog_del_ok );
    scope_perf_target_t check_binlog_del_not_found ( m_perf_binlog_del_not_found );
    scope_perf_target_t check_binlog_del_fail (      m_perf_binlog_del_fail );

    uint32_t user_version = version;
    version = 0;

    if ( unlikely ( user_version > BUCKET_DATA_MAX_VERSION ) )
    {
        LOG_ERROR ( "[md5db][db]invalid version %u, max=%u", user_version, BUCKET_DATA_MAX_VERSION );
        PERF_DEL_CANCEL ();
        return EINVAL;
    }

    char        inner_key[ 16 ];
    size_t      inner_key_len    = 16;

    if ( unlikely ( ! check_user_key ( user_key, user_key_len ) ) )
    {
        LOG_INFO ( "[md5db][db]invalid user key" );
        PERF_DEL_CANCEL ();
        return EINVAL;
    }

    size_t new_user_key_len = user_key_len;
    const char * new_user_key = get_inner_tbkey ( user_key, new_user_key_len, conn );
    user_key_to_inner ( inner_key, new_user_key, new_user_key_len );

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int r = 0;
    if ( tmp_ctxt->tbkey.empty () )
    {
        r = m_inner->m_data.hash_with_md5db ( inner_key, inner_key_len, conn, ctxt );
    }
    else if ( tmp_ctxt->table_len > ZSET_SCORE_LEN &&
              tmp_ctxt->tbkey.at ( tmp_ctxt->table_len - ZSET_SCORE_LEN - 1 ) == ZSET_TB
              )
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len - ZSET_SCORE_LEN, conn, ctxt );
    }
    else
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len, conn, ctxt );
    }

    if ( unlikely ( r < 0 ) )
    {
        LOG_ERROR ( "[md5db][db]hash failed" );
        PERF_DEL_FAIL ();
        return r;
    }

    // get bucket
    bucket_t & bucket = m_inner->m_buckets.get_bucket ( inner_key, inner_key_len );

    bucket_data_item_t *    item    = NULL;
    r = bucket.find ( inner_key, inner_key_len, item );
    if ( unlikely ( NULL == item ) )
    {
        LOG_ERROR ( "[md5db][db]find return %d, NULL", r );
        if ( 0 == r )
        {
            r = EFAULT;
        }
        PERF_DEL_FAIL ();
        return r;
    }

    scope_wlock_t lock ( bucket.get_lock () );

    switch ( item->type )
    {
        case BUCKET_DIRECT_DATA:
            r = check_same_key ( bucket,
                                item->block_id,
                                inner_key,
                                inner_key_len,
                                user_key,
                                user_key_len );
            if ( r < 0 )
            {
                LOG_ERROR ( "[md5db][db]check_same_key return %d", r );
                r = EFAULT;
                PERF_DEL_FAIL ();
                break;
            }
            if ( 0 != r )
            {
                // not same key
                r = ENOENT;
                PERF_DEL_NOT_FOUND ();
                break;
            }
            // same key
            version         = item->version;

            if ( 0 != user_version )
            {
                if ( ! from_binlog )
                {
                    // NOT binlog
                    if ( user_version != item->version )
                    {
                        LOG_DEBUG ( "[md5db][db][del]version not match user=%u,%u",
                                   user_version, item->version );
                        ctxt->is_version_error = true;
                        r = EINVAL;
                        PERF_DEL_WRONG_VERSION ();
                        break;
                    }
                }
                else
                {
                    // binlog, del user_version always = item->version
                    if ( user_version < item->version )
                    {
                        LOG_DEBUG ( "[md5db][db][del][binlog]version too low user=%u,%u",
                                   user_version, item->version );
                        ctxt->is_version_error = true;
                        r = 0;
                        PERF_DEL_WRONG_VERSION ();
                        break;
                    }
                }
            }

            r = delete_direct_data ( bucket,
                                    item->block_id,
                                    conn,
                                    ctxt );
            if ( 0 != r )
            {
                LOG_ERROR ( "[md5db][db]delete_direct_data return %d", r );
                if ( ENOENT == r )
                {
                    r = EFAULT;
                }
                PERF_DEL_FAIL ();
                break;
            }

            item->block_id.reset ();
            item->type      = BUCKET_NO_DATA;
            PERF_DEL_OK ();
            break;

        case BUCKET_CONFLICT_DATA:
            r = delete_conflict_data ( bucket,
                                      item,
                                      inner_key,
                                      inner_key_len,
                                      user_version,
                                      conn,
                                      ctxt,
                                      from_binlog );
            version = user_version;
            if ( 0 == r )
            {
                PERF_DEL_OK ();
            }
            else
            {
                if ( ENOENT != r )
                {
                    LOG_ERROR ( "[md5db][db]delete_conflict_data return %d", r );
                    PERF_DEL_FAIL ();
                }
                else
                {
                    PERF_DEL_NOT_FOUND ();
                }
            }
            break;

        case BUCKET_NO_DATA:
            r = ENOENT;
            PERF_DEL_NOT_FOUND ();
            break;

        default:
            LOG_ERROR ( "[md5db][db]invalid item->type %d", ( int ) item->type );
            r = EFAULT;
            PERF_DEL_FAIL ();
            break;
    }

    return r;
}

int kv_md5db_t::del (
                      const char *        user_key,
                      size_t              user_key_len,
                      uint32_t &          version,
                      bool                is_dup,
                      conn_ctxt_t         conn,
                      item_ctxt_t * &     ctxt
                      )
{
    return del_inner ( user_key, user_key_len, version, conn, ctxt, is_dup );
}

#define PERF_PUT_CANCEL()                       \
    if ( from_binlog )                          \
    {                                           \
        check_put_ok.cancel();                  \
        check_put_fail.cancel();                \
        check_binlog_put_ok.cancel();           \
        check_binlog_put_fail.cancel();         \
    }                                           \
    else                                        \
    {                                           \
        check_put_ok.cancel();                  \
        check_put_fail.cancel();                \
        check_binlog_put_ok.cancel();           \
        check_binlog_put_fail.cancel();         \
    }

#define PERF_PUT_OK()                           \
    if ( from_binlog )                          \
    {                                           \
        check_put_ok.cancel();                  \
        check_put_fail.cancel();                \
        check_binlog_put_fail.cancel();         \
    }                                           \
    else                                        \
    {                                           \
        check_put_fail.cancel();                \
        check_binlog_put_ok.cancel();           \
        check_binlog_put_fail.cancel();         \
    }

#define PERF_PUT_WRONG_VERSION()                \
    PERF_PUT_CANCEL();                          \
    if ( from_binlog )                          \
    {                                           \
        ++ m_count_binlog_put_wrong_version;    \
    }                                           \
    else                                        \
    {                                           \
        ++ m_count_put_wrong_version;           \
    }

#define PERF_PUT_FAIL()                         \
    if ( from_binlog )                          \
    {                                           \
        check_put_ok.cancel();                  \
        check_put_fail.cancel();                \
        check_binlog_put_ok.cancel();           \
    }                                           \
    else                                        \
    {                                           \
        check_put_ok.cancel();                  \
        check_binlog_put_ok.cancel();           \
        check_binlog_put_fail.cancel();         \
    }

int kv_md5db_t::put_inner (
                            const char *        user_key,
                            size_t              user_key_len,
                            const char *        val,
                            size_t              val_len,
                            uint32_t &          version,
                            conn_ctxt_t         conn,
                            item_ctxt_t * &     ctxt,
                            bool                from_binlog
                            )
{
    scope_perf_target_t check_put_ok (          m_perf_put_ok );
    scope_perf_target_t check_put_fail (        m_perf_put_fail );
    scope_perf_target_t check_binlog_put_ok (   m_perf_binlog_put_ok );
    scope_perf_target_t check_binlog_put_fail ( m_perf_binlog_put_fail );

    uint32_t user_version = version;

    version = 0;
    ctxt    = NULL;

    if ( unlikely ( user_version > BUCKET_DATA_MAX_VERSION ) )
    {
        LOG_ERROR ( "[md5db][db]invalid version %u, max=%u", user_version, BUCKET_DATA_MAX_VERSION );
        PERF_PUT_CANCEL ();
        return EINVAL;
    }

    if ( unlikely ( from_binlog && 0 == user_version ) )
    {
        LOG_DEBUG ( "[md5db][db]from_binlog is true, so version must not 0" );
        PERF_PUT_CANCEL ();
        return EINVAL;
    }

    if ( NULL == val || 0 == val_len )
    {
        val     = "";
        val_len = 0;
    }

    char        inner_key[ 16 ];
    size_t      inner_key_len = 16;
    bool        found;

    if ( unlikely ( ! check_user_key ( user_key, user_key_len ) ) )
    {
        LOG_ERROR ( "[md5db][db]invalid user key[user_key=%p][user_key_len=%d]", user_key, ( int ) user_key_len );
        PERF_PUT_CANCEL ();
        return EINVAL;
    }

    size_t new_user_key_len = user_key_len;
    const char * new_user_key = get_inner_tbkey ( user_key, new_user_key_len, conn );
    user_key_to_inner ( inner_key, new_user_key, new_user_key_len );

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int r = 0;
    if ( tmp_ctxt->tbkey.empty () )
    {
        r = m_inner->m_data.hash_with_md5db ( inner_key, inner_key_len, conn, ctxt );
    }
    else if ( tmp_ctxt->table_len > ZSET_SCORE_LEN &&
              tmp_ctxt->tbkey.at ( tmp_ctxt->table_len - ZSET_SCORE_LEN - 1 ) == ZSET_TB
              )
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len - ZSET_SCORE_LEN, conn, ctxt );
    }
    else
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len, conn, ctxt );
    }

    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][db]hash failed" );
        PERF_PUT_CANCEL ();
        return r;
    }

    // get bucket
    bucket_t & bucket = m_inner->m_buckets.get_bucket ( inner_key, inner_key_len );

    bucket_data_item_t *    item    = NULL;
    r = bucket.find ( inner_key, inner_key_len, item );
    if ( NULL == item )
    {
        LOG_ERROR ( "[md5db][db]find return %d, NULL", r );
        if ( 0 == r )
        {
            r = EFAULT;
        }
        PERF_PUT_FAIL ();
        return r;
    }

    scope_wlock_t lock ( bucket.get_lock () );

    switch ( item->type )
    {
        case BUCKET_DIRECT_DATA:
            r = check_same_key ( bucket,
                                item->block_id,
                                inner_key,
                                inner_key_len,
                                user_key,
                                user_key_len );
            if ( r < 0 )
            {
                LOG_ERROR ( "[md5db][db]check_same_key return %d", r );
                r = EFAULT;
                PERF_PUT_FAIL ();
                break;
            }
            if ( 0 == r )
            {
                // same key
                version = item->version;
                if ( 0 != user_version )
                {
                    if ( ! from_binlog )
                    {
                        // NOT binlog
                        if ( user_version != item->version )
                        {
                            LOG_ERROR ( "[md5db][db][put]version not match user=%u,%u",
                                       user_version, item->version );
                            ctxt->is_version_error = true;
                            r = EINVAL;
                            PERF_PUT_WRONG_VERSION ();
                            break;
                        }
                    }
                    else
                    {
                        // binlog
                        if ( user_version <= item->version )
                        {
                            LOG_DEBUG ( "[md5db][db][put][binlog]version too low user=%u,%u",
                                       user_version, item->version );
                            ctxt->is_version_error = true;
                            r = 0;
                            PERF_PUT_WRONG_VERSION ();
                            break;
                        }
                    }
                }

                r = set_data (
                              item->block_id,
                              user_key,
                              user_key_len,
                              val,
                              val_len,
                              conn,
                              ctxt );
                if ( 0 != r )
                {
                    version = item->version;
                    LOG_ERROR ( "[md5db][db]set_data return %d", r );
                    PERF_PUT_FAIL ();
                    break;
                }
                if ( ! from_binlog )
                {
                    version = item->version;
                    add_version ( version );
                    item->version = version;
                }
                else
                {
                    item->version = user_version;
                    version = user_version;
                }

                if ( 0 == version )
                {
                    LOG_ERROR ( "[md5db][db]set_data return version is 0" );
                    r = EFAULT;
                    PERF_PUT_FAIL ();
                    break;
                }
                PERF_PUT_OK ();
                LOG_DEBUG ( "[md5db][db][block_id=%u][version=%u][user_version=%u]UPDATE direct_data ok",
                           item->block_id.bucket_id (), version, user_version );
            }
            else
            {
                // not same key, direct -> conflict!
                r = add_conflict_data ( bucket,
                                       item,
                                       inner_key,
                                       inner_key_len,
                                       user_key,
                                       user_key_len,
                                       val,
                                       val_len,
                                       user_version,
                                       conn,
                                       ctxt,
                                       from_binlog );
                version = user_version;
                if ( 0 != r )
                {
                    LOG_ERROR ( "[md5db][db]add_conflict_data return %d", r );
                    PERF_PUT_FAIL ();
                    break;
                }

                if ( 0 == version )
                {
                    LOG_ERROR ( "[md5db][db]add_conflict_data return version is 0" );
                    r = EFAULT;
                    PERF_PUT_FAIL ();
                    break;
                }
                PERF_PUT_OK ();

                ctxt->kv_type = NEW_KV;

                ++ m_count_conflicts[ 2 ];
            }
            break;

        case BUCKET_CONFLICT_DATA:
            r = set_conflict_data ( bucket,
                                   item,
                                   inner_key,
                                   inner_key_len,
                                   user_key,
                                   user_key_len,
                                   val,
                                   val_len,
                                   user_version,
                                   found,
                                   conn,
                                   ctxt,
                                   from_binlog );
            version = user_version;
            if ( 0 == r )
            {
                if ( 0 == version )
                {
                    LOG_ERROR ( "[md5db][db]set_conflict_data return version is 0" );
                    r = EFAULT;
                    PERF_PUT_FAIL ();
                    break;
                }
                PERF_PUT_OK ();
            }
            else
            {
                if ( ctxt->is_version_error )
                {
                    if ( from_binlog )
                    {
                        r = 0;
                    }
                    PERF_PUT_WRONG_VERSION ();
                }
                else
                {
                    LOG_ERROR ( "[md5db][db]set_conflict_data return %d", r );
                    PERF_PUT_FAIL ();
                }
            }
            break;

        case BUCKET_NO_DATA:
            r = new_data ( bucket,
                          item,
                          inner_key,
                          inner_key_len,
                          user_key,
                          user_key_len,
                          val,
                          val_len,
                          user_version,
                          conn,
                          ctxt,
                          from_binlog );
            version = user_version;
            if ( 0 == r )
            {
                if ( 0 == version )
                {
                    LOG_ERROR ( "[md5db][db]new_data return version is 0" );
                    r = EFAULT;
                    PERF_PUT_FAIL ();
                    break;
                }
                PERF_PUT_OK ();

                ctxt->kv_type = NEW_KV;
            }
            else
            {
                if ( ctxt->is_version_error )
                {
                    if ( from_binlog )
                    {
                        r = 0;
                    }
                    PERF_PUT_WRONG_VERSION ();
                }
                else
                {
                    LOG_ERROR ( "[md5db][db]new_data return %d", r );
                    PERF_PUT_FAIL ();
                }
            }
            break;

        default:
            LOG_ERROR ( "[md5db][db]invalid item->type %d", ( int ) item->type );
            r = EFAULT;
            PERF_PUT_FAIL ();
            break;
    }

    return r;
}

int kv_md5db_t::put (
                      const char *        user_key,
                      size_t              user_key_len,
                      const char *        val,
                      size_t              val_len,
                      uint32_t &          version,
                      bool                is_dup,
                      conn_ctxt_t         conn,
                      item_ctxt_t * &     ctxt
                      )
{
    int r = put_inner ( user_key,
                       user_key_len,
                       val,
                       val_len,
                       version,
                       conn,
                       ctxt,
                       is_dup );
    if ( 0 != r )
    {
        if ( ctxt && ctxt->is_version_error )
        {
            if ( is_dup )
            {
                r = 0;
            }
            LOG_ERROR ( "[md5db][db]put_inner version error" );
        }
        else
        {
            LOG_ERROR ( "[md5db][db][user_key=%p]put_inner return %d", user_key, r );
        }
    }

    return r;
}

void kv_md5db_t::hash (
                        const char *        user_key,
                        size_t              user_key_len,
                        conn_ctxt_t         conn,
                        item_ctxt_t * &     ctxt
                        )
{
    char inner_key[ 16 ];
    size_t inner_key_len = 16;

    if ( unlikely ( ! check_user_key ( user_key, user_key_len ) ) )
    {
        LOG_INFO ( "[md5db][db]invalid user key" );
        ctxt = NULL;
        return;
    }
    user_key_to_inner ( inner_key, user_key, user_key_len );

    m_inner->m_data.hash ( inner_key, inner_key_len, conn, ctxt );
}

void kv_md5db_t::info (
                        std::stringstream & ss
                        )
{
    unsigned int file_id = 0;

    ss << "{\"fastdb\":{";
    write_perf_info ( ss, file_id, "put_ok", m_perf_put_ok );
    write_perf_info ( ss, file_id, "put_fail", m_perf_put_fail );
    write_perf_info ( ss, file_id, "binlog_put_ok", m_perf_binlog_put_ok );
    write_perf_info ( ss, file_id, "binlog_put_fail", m_perf_binlog_put_fail );
    write_single_count ( ss, file_id, "put_wrong_version", m_count_put_wrong_version );
    write_single_count ( ss, file_id, "binlog_put_wrong_version", m_count_binlog_put_wrong_version );

    write_perf_info ( ss, file_id, "del_ok", m_perf_del_ok );
    write_perf_info ( ss, file_id, "del_not_found", m_perf_del_not_found );
    write_perf_info ( ss, file_id, "del_fail", m_perf_del_fail );
    write_perf_info ( ss, file_id, "binlog_del_ok", m_perf_binlog_del_ok );
    write_perf_info ( ss, file_id, "binlog_del_not_found", m_perf_binlog_del_not_found );
    write_perf_info ( ss, file_id, "binlog_del_fail", m_perf_binlog_del_fail );
    write_single_count ( ss, file_id, "del_wrong_version", m_count_del_wrong_version );
    write_single_count ( ss, file_id, "binlog_del_wrong_version", m_count_binlog_del_wrong_version );

    write_perf_info ( ss, file_id, "get_ok", m_perf_get_ok );
    write_perf_info ( ss, file_id, "get_not_found", m_perf_get_not_found );
    write_perf_info ( ss, file_id, "get_fail", m_perf_get_fail );

    write_perf_info ( ss, file_id, "conflict_get", m_perf_conflict_get );
    write_perf_info ( ss, file_id, "conflict_put", m_perf_conflict_put );
    write_perf_info ( ss, file_id, "conflict_del", m_perf_conflict_del );
    write_perf_info ( ss, file_id, "fast_conflict_get", m_perf_fast_conflict_get );
    write_perf_info ( ss, file_id, "fast_conflict_put", m_perf_fast_conflict_put );
    write_perf_info ( ss, file_id, "fast_conflict_del", m_perf_fast_conflict_del );

    write_perf_info ( ss, file_id, "fullkey_get", m_perf_fullkey_get );
    write_perf_info ( ss, file_id, "fullkey_write", m_perf_fullkey_write );
    write_perf_info ( ss, file_id, "fullkey_del", m_perf_fullkey_del );

    write_perf_info ( ss, file_id, "content_get", m_perf_content_get );
    write_perf_info ( ss, file_id, "content_write", m_perf_content_write );
    write_perf_info ( ss, file_id, "content_update", m_perf_content_update );
    write_perf_info ( ss, file_id, "content_del", m_perf_content_del );

    write_perf_info ( ss, file_id, "data_get", m_perf_data_get );
    write_perf_info ( ss, file_id, "data_put", m_perf_data_put );
    write_perf_info ( ss, file_id, "data_del", m_perf_data_del, true );

    ss << "},";

    ss << "\"leveldb\":{";
    m_inner->m_data.info ( ss );
    ss << "},";

    ss << "\"conflictdb_count\":{";
    for ( size_t i = 0; i < COUNT_OF ( m_count_conflicts ); ++ i )
    {
        char key[ 128 ];
        sprintf ( key, "%d", ( int ) i );

        if ( i == COUNT_OF ( m_count_conflicts ) - 1 )
        {
            write_single_count ( ss, file_id, key, m_count_conflicts[ i ], true );
        }
        else
        {
            write_single_count ( ss, file_id, key, m_count_conflicts[ i ] );
        }

    }
    ss << "},";

    ss << "\"conflictdb\":{";
    m_inner->m_conflicts.info ( ss );
    ss << "},";

    ss << "\"fullkey\":{";
    m_inner->m_fullkeys.info ( ss );
    ss << "},";

    ss << "\"fast_conflictdb\":{";
    m_inner->m_fast_conflicts.info ( ss );
    ss << "}";

    ss << "}";
}

uint32_t kv_md5db_t::md5db_info_by_key (
                                         const char *                key,
                                         size_t                      key_len,
                                         const char *                table,
                                         size_t                      table_len,
                                         md5db::block_id_t &         block_id
                                         )
{
    char        inner_key[ 16 ];
    size_t      inner_key_len    = 16;

    if ( table && table_len > 0 )
    {
        std::string s ( table, table_len + 1 );
        s.append ( key, key_len );

        user_key_to_inner ( inner_key, s.c_str (), s.size () );
    }
    else
    {
        user_key_to_inner ( inner_key, key, key_len );
    }

    return md5db_info_by_inner_key ( key, key_len, inner_key, inner_key_len, block_id );
}

uint32_t kv_md5db_t::md5db_info_by_inner_key (
                                               const char *                key,
                                               size_t                      key_len,
                                               const char *                inner_key,
                                               size_t                      inner_key_len,
                                               md5db::block_id_t &         block_id
                                               )
{
    uint32_t                version = 0;
    bucket_data_item_t *    item    = NULL;

    bucket_t & bucket = m_inner->m_buckets.get_bucket ( inner_key, inner_key_len );

    int r = bucket.find ( inner_key, inner_key_len, item );
    if ( NULL == item )
    {
        LOG_ERROR ( "[md5db][db]find return %d, NULL", r );
        return 0;
    }

    scope_rlock_t lock( bucket.get_lock() );

    switch ( item->type )
    {
        case BUCKET_DIRECT_DATA:
            r = check_same_key ( bucket, item->block_id, inner_key, inner_key_len, key, key_len );
            block_id = item->block_id;

            if ( r < 0 )
            {
                return 0;
            }

            if ( 0 == r )
            {
                return item->version;
            }
            else
            {
                return 0;
            }

        case BUCKET_CONFLICT_DATA:
            r = get_conflict_block_id ( inner_key, inner_key_len, block_id, version );
            if ( 0 != r )
            {
                return 0;
            }
            return version;

        case BUCKET_NO_DATA:
        default:
            return 0;
    }
}

int kv_md5db_t::export_db (
                            int                         file_id,
                            const char *                path,
                            export_record_callback_t    callback,
                            void *                      callback_param
                            )
{
    if ( NULL == m_inner )
    {
        LOG_ERROR ( "[md5db][export]m_inner is NULL" );
        return EFAULT;
    }

    struct export_cb_param_t * cb_pm = ( struct export_cb_param_t * ) callback_param;
    cb_pm->db                        = this;

    int r = m_inner->m_data.export_db ( file_id, path, export_md5db_record_callback, cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][export]export_db return %d", r );
        return r;
    }

    return 0;
}

int kv_md5db_t::export_db_mem (
                                conn_ctxt_t                 conn,
                                std::string * &             rsp,
                                item_ctxt_t * &             ctxt,
                                export_record_callback_t    callback,
                                void *                      callback_param
                                )
{
    rsp     = NULL;
    ctxt    = NULL;

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int r = 0;
    if ( tmp_ctxt->tbkey.empty () )
    {
        LOG_ERROR ( "[md5db][export_db_mem]table empty" );
        return EFAULT;
    }

    r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len, conn, ctxt );
    if ( r < 0 )
    {
        LOG_ERROR ( "[md5db][export_db_mem]hash failed" );
        return r;
    }

    if ( NULL == m_inner )
    {
        LOG_ERROR ( "[md5db][export_db_mem]m_inner is NULL" );
        return EFAULT;
    }

    ctxt->key = ( strncmp ( tmp_ctxt->tbkey.c_str (), EXPORT_DB_ALL, sizeof ( EXPORT_DB_ALL ) - 1 ) != 0 ) ? tmp_ctxt->tbkey : EXPORT_DB_ALL;

    struct export_cb_param_t * cb_pm = ( struct export_cb_param_t * ) callback_param;
    cb_pm->db                        = this;

    r = m_inner->m_data.export_db_mem ( conn, rsp, ctxt, export_md5db_record_callback, cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][export_db_mem]export_db return %d", r );
        return r;
    }

    return 0;
}

int kv_md5db_t::ttl_scan (
                           export_record_callback_t callback,
                           void * callback_param
                           )
{
    if ( NULL == m_inner )
    {
        LOG_ERROR ( "[md5db][ttl_scan]m_inner is NULL" );
        return EFAULT;
    }

    struct export_cb_param_t * cb_pm = ( struct export_cb_param_t * ) callback_param;
    cb_pm->db                        = this;

    int r = m_inner->m_data.ttl_scan ( export_md5db_record_callback, cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][ttl_scan]ttl_scan return %d", r );
        return r;
    }

    return 0;
}

int kv_md5db_t::binlog_scan (
                              binlog_callback_t task_cb,
                              binlog_callback_t alive_cb,
                              void * alive_cb_param,
                              export_record_callback_t export_cb,
                              void * export_cb_param
                              )
{
    if ( NULL == m_inner )
    {
        LOG_ERROR ( "[md5db][binlog_scan]m_inner is NULL" );
        return EFAULT;
    }

    std::string binlog_status;
    m_inner->m_binlog.get_status ( binlog_status );
    
    LOG_INFO ( "[md5db][binlog_scan]%s", binlog_status.c_str () );
    
    std::map<std::string, char> alives;
    m_inner->m_binlog.get_alives ( alives );

    struct check_alive_cb_param_t * alive_cb_pm = ( struct check_alive_cb_param_t * ) alive_cb_param;
    alive_cb_pm->db                             = this;
    alive_cb_pm->alives                         = & alives;

    struct export_cb_param_t * export_cb_pm     = ( struct export_cb_param_t * ) export_cb_param;
    export_cb_pm->db                            = this;

    int r = m_inner->m_data.binlog_scan ( binlog_task_callback, check_alive_callback, alive_cb_pm, export_md5db_record_callback, export_cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][binlog_scan]binlog_scan return %d", r );
        return r;
    }

    return 0;
}

int kv_md5db_t::binlog (
                         const char * user_key,
                         size_t user_key_len,
                         const char * host,
                         size_t host_len,
                         uint8_t cmd_type,
                         bool is_rem,
                         conn_ctxt_t conn
                         )
{
    char                inner_key[ 16 ];
    size_t              inner_key_len    = 16;
    uint8_t             host_u8          = ( uint8_t ) host_len;
    item_ctxt_t *       item_ctxt        = NULL;
    block_id_t          block_id;

    if ( unlikely ( ! check_user_key ( user_key, user_key_len ) ) )
    {
        LOG_ERROR ( "[md5db][binlog]invalid user key[user_key=%p][user_key_len=%d]", user_key, ( int ) user_key_len );
        return EINVAL;
    }

    size_t new_user_key_len = user_key_len;
    const char * new_user_key = get_inner_tbkey ( user_key, new_user_key_len, conn );
    user_key_to_inner ( inner_key, new_user_key, new_user_key_len );

    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    int r = 0;
    if ( tmp_ctxt->tbkey.empty () )
    {
        r = m_inner->m_data.hash_with_md5db ( inner_key, inner_key_len, conn, item_ctxt );
    }
    else if ( tmp_ctxt->table_len > ZSET_SCORE_LEN &&
              tmp_ctxt->tbkey.at ( tmp_ctxt->table_len - ZSET_SCORE_LEN - 1 ) == ZSET_TB
              )
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len - ZSET_SCORE_LEN, conn, item_ctxt );
    }
    else
    {
        r = m_inner->m_data.hash_with_md5db ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len, conn, item_ctxt );
    }

    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][binlog]hash failed" );
        return r;
    }

    item_ctxt->key.append ( ( const char * ) & host_u8, sizeof ( uint8_t ) );
    item_ctxt->key.append ( host, host_len );
    
    if ( tmp_ctxt->table_len > 0 )
    {
        item_ctxt->key.append ( tmp_ctxt->tbkey.c_str (), tmp_ctxt->table_len );
    }

    tmp_ctxt->value.resize ( 0 );
    tmp_ctxt->value.append ( 1, ( char ) cmd_type );

    if ( ! is_rem )
    {
        uint32_t ver = md5db_info_by_inner_key ( user_key, user_key_len, inner_key, inner_key_len, block_id );
        if ( ver == 0 )
        {
            return 0;
        }

        item_ctxt->key.append ( ( const char * ) & block_id, sizeof ( md5db::block_id_t ) );
        tmp_ctxt->value.append ( ( const char * ) & item_ctxt->inner_file_id, sizeof ( int ) );
    }
    else
    {
        item_ctxt->key.append ( inner_key, inner_key_len );
        tmp_ctxt->value.append ( user_key, user_key_len );
    }

    r = m_inner->m_data.put_from_binlog ( item_ctxt->key.c_str (),
                                         item_ctxt->key.size (),
                                         tmp_ctxt->value.c_str (),
                                         tmp_ctxt->value.size () );
    if ( 0 != r )
    {
        LOG_ERROR ( "[md5db][binlog]m_data.put_from_binlog error, return %d", r );
        return r;
    }

    return 0;
}

int kv_md5db_t::hash_info (
                            int                         user_file_id,
                            int &                       inner_file_id
                            )
{
    if ( unlikely ( NULL == m_inner ) )
    {
        inner_file_id = - 1;
        LOG_ERROR ( "[md5db][export]m_inner is NULL" );
        return EFAULT;
    }

    return m_inner->m_data.hash_info ( user_file_id, inner_file_id );
}

int kv_md5db_t::get_user_file_count ( )
{
    return m_inner ? m_inner->m_data.get_user_file_count () : 0;
}

i_kv_t * kv_md5db_t::get_conflict ( int file_id )
{
    if ( unlikely ( NULL == m_inner ) )
    {
        LOG_ERROR ( "[md5db][get_conflict]m_inner is NULL" );
        return NULL;
    }

    conflict_array_t & ca = m_inner->m_conflicts;

    if ( unlikely ( file_id < 0 || file_id >= ca.size () ) )
    {
        LOG_ERROR ( "[md5db][get_conflict]invalid file_id %d/%d", file_id, ca.size () );
        return NULL;
    }

    conflict_t * c = ca.item ( file_id );
    if ( unlikely ( NULL == c ) )
    {
        LOG_ERROR ( "[md5db][get_conflict]invalid item" );
        return NULL;
    }

    return c->get_kv ();
}

void kv_md5db_t::set_inner_ttl (
                                 uint32_t                ttl,
                                 conn_ctxt_t             conn
                                 )
{
    m_inner->m_query_ctxts[ conn.worker_id ].wttl = ttl;
}

uint32_t kv_md5db_t::get_inner_ttl (
                                     conn_ctxt_t         conn
                                     )
{
    return m_inner->m_query_ctxts[ conn.worker_id ].rttl;
}

void kv_md5db_t::set_inner_table (
                                   const char *          table,
                                   size_t                table_len,
                                   uint8_t               type,
                                   conn_ctxt_t           conn,
                                   const char *          added
                                   )
{
    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    tmp_ctxt->tbkey.resize ( 0 );
    tmp_ctxt->table_len = 0;

    if ( table && table_len > 0 )
    {
        tmp_ctxt->tbkey.append ( table, table_len );
        tmp_ctxt->tbkey.append ( 1, ( char ) type );
        tmp_ctxt->table_len = table_len + 1;

        if ( added )
        {
            int added_len = strlen ( added );
            tmp_ctxt->tbkey.append ( added, added_len );
            tmp_ctxt->table_len += added_len;
        }
    }
}

const char * kv_md5db_t::get_inner_tbkey (
                                           const char *      key,
                                           size_t &          key_len,
                                           conn_ctxt_t       conn
                                           )
{
    query_ctxt_t * tmp_ctxt;
    tmp_ctxt = & m_inner->m_query_ctxts[ conn.worker_id ];

    if ( ! key || key_len <= 0 || tmp_ctxt->tbkey.empty () )
    {
        return key;
    }

    if ( tmp_ctxt->table_len == tmp_ctxt->tbkey.size () )
    {
        tmp_ctxt->tbkey.append ( key, key_len );
    }

    key_len = tmp_ctxt->tbkey.size ();

    return tmp_ctxt->tbkey.c_str ();
}

static void export_md5db_record_callback (
                                           void *                  param,
                                           const char * &          key,
                                           size_t &                key_len,
                                           const char * &          val,
                                           size_t &                val_len,
                                           const char *            table,
                                           size_t                  table_len,
                                           uint32_t &              version,
                                           uint32_t &              ttl,
                                           std::string &           content,
                                           bool *                  ignore_this_record,
                                           bool *                  break_the_loop
                                           )
{
    if ( unlikely ( ! param ) )
    {
        LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! invalid callback param" );
        * ignore_this_record = true;
        return;
    }

    struct export_cb_param_t * cb_pm = ( struct export_cb_param_t * ) param;
    kv_md5db_t * db                  = ( kv_md5db_t * ) cb_pm->db;
    uint16_t     start               = cb_pm->start;
    uint16_t     end                 = cb_pm->end;
    bool         noval               = cb_pm->noval;

    if ( db->get_inner ()->m_contents.is_open () )
    {
        if ( NULL == val || val_len != sizeof ( data_item_for_content_db_t ) )
        {
            LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! invalid user key len" );
            * ignore_this_record = true;
            return;
        }

        data_item_for_content_db_t addr;
        memcpy ( & addr, val, sizeof ( data_item_for_content_db_t ) );
        if ( 0 == addr.data_len )
        {
            LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! invalid size 0" );
            * ignore_this_record = true;
            return;
        }

        try
        {
            content.resize ( addr.data_len );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! bad_alloc" );
            * ignore_this_record = true;
            return;
        }

        bool b;
        {
            b = db->get_inner ()->m_contents.get ( addr.file_id, addr.data_id, addr.data_len, & content[ 0 ] );
        }
        if ( ! b )
        {
            LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! get content failed" );
            * ignore_this_record = true;
            return;

        }

        val = content.c_str ();
        val_len = content.size ();
    }

    uint8_t idx_len = sizeof ( uint32_t ) * 2;
    if ( NULL == val || val_len <= idx_len )
    {
        LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! invalid user key len" );
        * ignore_this_record = true;
        return;
    }

    fast_memcpy ( & ttl, & val[ val_len - sizeof (uint32_t ) ], sizeof (uint32_t ) );

    uint32_t kl;
    fast_memcpy ( & kl, & val[ val_len - idx_len ], sizeof (uint32_t ) );
    if ( 0 == kl )
    {
        LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! invalid user key len" );
        * ignore_this_record = true;
        return;
    }

    const char * uk;
    if ( val_len < idx_len + kl )
    {
        LOG_ERROR ( "[md5db][export_md5db_record_callback]GOD! invalid data_len %d, ukey_len=%d", ( int ) val_len, ( int ) kl );
        * ignore_this_record = true;
        return;
    }
    uk = ( const char * ) & val[ val_len - idx_len - kl ];

    uint16_t hash = G_APPTOOL->bucket_hash ( uk, kl );
    if ( hash < start || hash >= end )
    {
        * ignore_this_record = true;
        return;
    }

    * ignore_this_record = false;

    uint32_t vl = val_len - idx_len - kl;

    key         = uk;
    key_len     = kl;
    val_len     = noval ? 0 : vl;

    block_id_t block_id;
    version = db->md5db_info_by_key ( key, key_len, table, table_len, block_id );
}

static void check_alive_callback (
                                   void * param
                                   )
{
    if ( unlikely ( ! param ) )
    {
        LOG_ERROR ( "[md5db][check_alive_callback]GOD! invalid callback param" );
        return;
    }

    struct check_alive_cb_param_t * cb_pm = ( struct check_alive_cb_param_t * ) param;
    kv_md5db_t * db                       = ( kv_md5db_t * ) cb_pm->db;
    std::map<std::string, char> * alives  = ( std::map<std::string, char> * ) cb_pm->alives;
    std::string * host                    = ( std::string * ) cb_pm->host;

    std::map<std::string, char>::iterator alive_it = alives->find ( * host );
    if ( alive_it == alives->end () )
    {
        db->get_inner ()->m_binlog.add_host ( * host );
        cb_pm->cursor_type = '-';
    }
    else
    {
        cb_pm->cursor_type = alive_it->second;
    }
}

static void binlog_task_callback (
                                   void * param
                                   )
{
    if ( unlikely ( ! param ) )
    {
        LOG_ERROR ( "[md5db][binlog_task_callback]GOD! invalid callback param" );
        return;
    }
    
    struct binlog_task_cb_param_t * cb_pm = ( struct binlog_task_cb_param_t * ) param;
    kv_md5db_t * db                       = ( kv_md5db_t * ) cb_pm->db;


    bool b = db->get_inner ()->m_binlog.add_task (
                                                  cb_pm->host,
                                                  cb_pm->host_len,
                                                  cb_pm->table,
                                                  cb_pm->table_len,
                                                  cb_pm->key,
                                                  cb_pm->key_len,
                                                  cb_pm->value,
                                                  cb_pm->value_len,
                                                  cb_pm->ver,
                                                  cb_pm->ttl,
                                                  cb_pm->cmd_type,
                                                  cb_pm->param
                                                  );
    
    cb_pm->cursor_type = b ? '+' : '-';
}

static void binlog_done_callback (
                                   void * param
                                   )
{
    if ( unlikely ( ! param ) )
    {
        LOG_ERROR ( "[md5db][binlog_done_callback]GOD! invalid callback param" );
        return;
    }

    struct binlog_done_cb_param_t * cb_pm = ( struct binlog_done_cb_param_t * ) param;
    kv_md5db_t * db                       = ( kv_md5db_t * ) cb_pm->db;

    int r = db->get_inner ()->m_data.del_from_binlog ( cb_pm->key, cb_pm->key_len );
    if ( unlikely ( r != 0 ) )
    {
        LOG_ERROR ( "[md5db][binlog_done_callback]GOD! del_from_binlog error, return %d", r );
    }
}
