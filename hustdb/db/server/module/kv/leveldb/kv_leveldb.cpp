#include "kv_leveldb.h"
#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/env.h"
#include "leveldb/filter_policy.h"
#include "bloom_filter.h"
#include "../../base.h"

class MyLogger : public leveldb::Logger
{
public:

    MyLogger ( )
    {
    }

    ~ MyLogger ( )
    {
    }

    void Logv ( const char * format, va_list ap )
    {
    }

    void set_path ( const std::string & path )
    {
        m_path = path;
    }

private:

    std::string m_path;

private:
    // disable
    MyLogger ( const MyLogger& );
    void operator= ( const MyLogger& ) ;
} ;

struct kv_leveldb_t::inner
{
    leveldb::DB *                   db;
    leveldb::Cache *                cache;
    MyLogger                        log;
    const leveldb::FilterPolicy *   filter_policy;
    md5_bloom_filter_t              my_bloom_filter;

    inner ( )
    : db ( NULL )
    , cache ( NULL )
    , log ( )
    , filter_policy ( NULL )
    , my_bloom_filter ( )
    {
    }
} ;

void kv_leveldb_t::kill_me ( )
{
    delete this;
}

kv_leveldb_t::kv_leveldb_t ( )
: m_file_id ( - 1 )
, m_path ( )
, m_config ( )
, m_inner ( NULL )
, m_perf_bloom_add ( )
, m_perf_bloom_hit ( )
, m_perf_bloom_not_hit ( )
, m_perf_not_found ( )
, m_perf_found ( )
, m_perf_put_ok ( )
, m_perf_put_fail ( )
{
}

kv_leveldb_t::~ kv_leveldb_t ( )
{
    destroy ();
}

void * kv_leveldb_t::get_internal_db ( )
{
    if ( m_inner )
    {
        return m_inner->db;
    }
    return NULL;
}

bool kv_leveldb_t::check_key (
                               const void *    key,
                               unsigned int    key_len
                               )
{
    if ( NULL == key || 0 == key_len || key_len > 1024 )
    {
        LOG_INFO ( "[ldb]invalid key" );
        return false;
    }

    return true;
}

void kv_leveldb_t::destroy ( )
{
    if ( m_inner )
    {

        if ( m_inner->db )
        {
            LOG_INFO ( "[ldb]DB closing[path=%s]", m_path.c_str () );
            try
            {
                delete m_inner->db;
                LOG_INFO ( "[ldb]DB close OK[path=%s]", m_path.c_str () );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[ldb]DB closeing exception[path=%s]", m_path.c_str () );
            }
            m_inner->db = NULL;
        }

        if ( m_inner->cache )
        {
            try
            {
                delete m_inner->cache;
            }
            catch ( ... )
            {
                LOG_ERROR ( "[ldb]cache closeing exception[path=%s]", m_path.c_str () );
            }
            m_inner->cache = NULL;
        }

        if ( m_inner->filter_policy )
        {
            try
            {
                delete m_inner->filter_policy;
            }
            catch ( ... )
            {
                LOG_ERROR ( "[ldb]bloom filter closeing exception[path=%s]", m_path.c_str () );
            }
            m_inner->filter_policy = NULL;
        }

        try
        {
            delete m_inner;
        }
        catch ( ... )
        {
            LOG_ERROR ( "delete m_inner exception" );
        }
        m_inner = NULL;
    }
}

void kv_leveldb_t::calc_my_bloom_filter_path ( const char * parent_dir, char * bloom_filter_path )
{
    strcpy ( bloom_filter_path, parent_dir );
    G_APPTOOL->path_to_os ( bloom_filter_path );

    int len;
    len = ( int ) strlen ( bloom_filter_path );
    if ( len <= 0 )
    {
        LOG_ERROR ( "parent_dir invalid" );
        return;
    }

    if ( S_PATH_SEP_C != bloom_filter_path[ len - 1 ] )
    {
        strcat ( bloom_filter_path, S_PATH_SEP );
    }

    strcat ( bloom_filter_path, "hustdb_bloom_filter.dat" );
}

bool kv_leveldb_t::open ( const char * path, const kv_config_t & config, int file_id )
{
    if ( NULL == path || '\0' == * path )
    {
        LOG_ERROR ( "[ldb]empty path" );
        return false;
    }

    if ( NULL == m_inner )
    {
        try
        {
            m_inner = new inner ();
        }
        catch ( ... )
        {
            LOG_ERROR ( "[ldb]new inner() exception" );
            return false;
        }
    }

    m_file_id       = file_id;
    m_config        = config;
    try
    {
        m_path      = path;
        m_inner->log.set_path ( m_path );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[ldb]bad_alloc" );
        return false;
    }

    int64_t cache_size = ( int64_t ) - 1;
    if ( config.cache_size_m > 0 )
    {
        cache_size = config.cache_size_m * 1024 * 1024;
    }

    int64_t write_buffer = ( int64_t ) - 1;
    if ( config.write_buffer_m > 0 )
    {
        write_buffer = config.write_buffer_m * 1024 * 1024;
    }

    leveldb::Options options;
    if ( ! config.is_readonly )
    {
        options.create_if_missing = true;
    }
    if ( config.disable_compression )
    {
        options.compression = leveldb::kNoCompression;
    }
    else
    {
        options.compression = leveldb::kSnappyCompression;
    }

    if ( write_buffer > 0 )
    {
        options.write_buffer_size = ( size_t ) write_buffer;
    }

    if ( cache_size > 0 )
    {
        assert ( NULL == m_inner->cache );

        try
        {
            m_inner->cache = leveldb::NewLRUCache ( ( size_t ) cache_size );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[ldb][cache]exception" );
            m_inner->cache = NULL;
        }
        if ( m_inner->cache )
        {
            LOG_INFO ( "[ldb][cache]cache opened OK[path=%s]", path );
        }
        else
        {
            LOG_ERROR ( "[ldb][cache]cache open FAILED[path=%s]", path );
            return false;
        }
        options.block_cache = ( leveldb::Cache * )m_inner->cache;
    }

    int ldb_bloom_filter_bits = config.ldb_bloom_filter_bits;
    if ( ldb_bloom_filter_bits > 0 )
    {
        try
        {
            m_inner->filter_policy = leveldb::NewBloomFilterPolicy ( ldb_bloom_filter_bits );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[ldb][bloom_filter]bloom filter open FAILED[path=%s]", path );
            return false;
        }
        options.filter_policy = m_inner->filter_policy;
    }
    else
    {
        LOG_INFO ( "[ldb][bloom_filter]DISABLED google bloom filter." );
    }

    options.info_log = & m_inner->log;

    G_APPTOOL->make_dir ( path );

    if ( MD5_BLOOM_DISABLED != ( md5_bloom_mode_t ) config.my_bloom_filter_type )
    {
        char bloom_filter_path[ 260 ] = "";
        calc_my_bloom_filter_path ( path, bloom_filter_path );
        if ( '\0' == bloom_filter_path[ 0 ] )
        {
            LOG_ERROR ( "[ldb][bloom_filter][type=%d]calc_my_bloom_filter_path FAILED. [parent_dir=%s]",
                       config.my_bloom_filter_type, path );
            return false;
        }
        bool b = m_inner->my_bloom_filter.open ( bloom_filter_path, ( md5_bloom_mode_t ) config.my_bloom_filter_type );
        if ( ! b )
        {
            LOG_ERROR ( "[ldb][bloom_filter][type=%d]hustdb_bloom_filter open FAILED. [path=%s]",
                       config.my_bloom_filter_type, bloom_filter_path );
            return false;
        }
        LOG_INFO ( "[ldb][bloom_filter][type=%d]hustdb_bloom_filter opened.",
                  config.my_bloom_filter_type, bloom_filter_path );
    }
    else
    {
        LOG_INFO ( "[ldb][bloom_filter]DISABLED hustdb_bloom_filter." );
    }

    leveldb::Status status;
    LOG_INFO ( "[ldb]DB opening[path=%s]", path );
    try
    {
        status = leveldb::DB::Open ( options, path, & m_inner->db );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[ldb]DB open exception[path=%s]", path );
        if ( m_inner->cache )
        {
            delete m_inner->cache;
            m_inner->cache = NULL;
        }
        return false;
    }
    if ( unlikely ( ! status.ok () ) )
    {
        std::string s;
        try
        {
            s = status.ToString ();
        }
        catch ( ... )
        {
        }
        LOG_INFO ( "[ldb]DB open FAILED: %s[path=%s]", s.c_str (), path );
        return false;
    }

#if ENABLE_DEBUG
    if ( ! test () )
    {
        LOG_ERROR ( "[ldb]DB test FAILED: [path=%s]", path );
        return false;
    }
    LOG_INFO ( "[ldb]DB test PASSED: [path=%s]", path );
#endif

    LOG_INFO ( "[ldb]DB opened OK[path=%s]", path );

    return true;
}

bool kv_leveldb_t::test ( )
{
    leveldb::Status         status;
    leveldb::WriteOptions   options;

    const char key[] = {
                        0x00, 0x38
    };
    int key_len = ( int ) sizeof ( key );
    const char data[] = "spider|hustmq|hustdb|hustngx|hustlog|wise|rocket";
    int data_len = ( int ) sizeof ( data ) - 1;

    try
    {
        leveldb::Slice k ( key, ( size_t ) key_len );
        leveldb::Slice v ( data, ( size_t ) data_len );
        status = m_inner->db->Put ( options, k, v );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }
    if ( ! status.ok () )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }

    std::string     value;
    try
    {
        leveldb::Slice k ( key, ( size_t ) key_len );
        status = m_inner->db->Get (
                                   leveldb::ReadOptions (),
                                   k,
                                   & value
                                   );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }
    if ( ! status.ok () )
    {
        std::string msg;
        try
        {
            msg = status.ToString ();
        }
        catch ( ... )
        {
        }
        LOG_ERROR ( "[test] test failed, key_len=%d, data_len=%d, msg=%s", key_len, data_len, msg.c_str () );
        return false;
    }
    if ( ! mem_equal ( value.c_str (), data, data_len ) )
    {
        LOG_ERROR ( "[test] test failed, value.size()=%d, data_len=%d", ( int ) value.size (), data_len );
        return false;
    }

    try
    {
        leveldb::Slice k ( key, ( size_t ) key_len );
        status = m_inner->db->Delete ( options, k );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }
    if ( ! status.ok () )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }

    value.resize ( 0 );
    try
    {
        leveldb::Slice k ( key, ( size_t ) key_len );
        status = m_inner->db->Get (
                                   leveldb::ReadOptions (),
                                   k,
                                   & value
                                   );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }
    if ( status.ok () || ! status.IsNotFound () )
    {
        LOG_ERROR ( "[test] test failed" );
        return false;
    }

    return true;
}

const char * kv_leveldb_t::get_path ( ) const
{
    return m_path.c_str ();
}

void kv_leveldb_t::info (
                          std::stringstream & ss
                          )
{
    write_perf_info ( ss, get_id (), "found",             m_perf_found );
    write_perf_info ( ss, get_id (), "not_found",         m_perf_not_found );
    write_perf_info ( ss, get_id (), "put_ok",            m_perf_put_ok );
    write_perf_info ( ss, get_id (), "put_fail",          m_perf_put_fail );
    write_perf_info ( ss, get_id (), "bloom_hit",         m_perf_bloom_hit );
    write_perf_info ( ss, get_id (), "bloom_not_hit",     m_perf_bloom_not_hit );
    write_perf_info ( ss, get_id (), "bloom_add",         m_perf_bloom_add, true );
}

int kv_leveldb_t::flush ( )
{
    return EINVAL;
}

int kv_leveldb_t::del (
                        const void *        key,
                        unsigned int        key_len
                        )
{
    if ( ! check_key ( key, key_len ) )
    {
        LOG_INFO ( "[ldb]invalid key[path=%s]", m_path.c_str () );
        return EINVAL;
    }
    if ( unlikely ( NULL == m_inner->db ) )
    {
        LOG_INFO ( "[ldb]m_inner->db is NULL[path=%s]", m_path.c_str () );
        return EFAULT;
    }

    leveldb::Status         status;
    leveldb::WriteOptions   options;

    try
    {
        leveldb::Slice k ( ( const char * ) key, ( size_t ) key_len );
        status = m_inner->db->Delete ( options, k );
    }
    catch ( ... )
    {
        LOG_INFO ( "[ldb]db->Delete exception[path=%s]", m_path.c_str () );
        return EFAULT;
    }

    if ( unlikely ( ! status.ok () ) )
    {
        if ( status.IsNotFound () )
        {
            return ENOENT;
        }
        else
        {
            std::string msg;
            try
            {
                msg = status.ToString ();
            }
            catch ( ... )
            {
            }
            LOG_INFO ( "[ldb]db->Delete error: %s[path=%s]", m_path.c_str (), msg.c_str () );
            return EFAULT;
        }
    }

    return 0;
}

int kv_leveldb_t::put (
                        const void *        key,
                        unsigned int        key_len,
                        const void *        data,
                        unsigned int        data_len
                        )
{
    if ( ! check_key ( key, key_len ) )
    {
        LOG_INFO ( "[ldb]invalid key[path=%s]", m_path.c_str () );
        return EINVAL;
    }
    if ( unlikely ( NULL == data || 0 == data_len ) )
    {
        LOG_INFO ( "[ldb]invalid data[path=%s]", m_path.c_str () );
        return EINVAL;
    }
    if ( unlikely ( NULL == m_inner->db ) )
    {
        LOG_INFO ( "[ldb]m_inner->db is NULL[path=%s]", m_path.c_str () );
        return EFAULT;
    }

    leveldb::Status         status;
    leveldb::WriteOptions   options;
    //options.sync = true;

    leveldb::Slice k ( ( const char * ) key, ( size_t ) key_len );

    scope_perf_target_t check_put_ok ( m_perf_put_ok );
    scope_perf_target_t check_put_fail ( m_perf_put_fail );

    try
    {
        leveldb::Slice v ( ( const char * ) data, ( size_t ) data_len );
        status = m_inner->db->Put ( options, k, v );
    }
    catch ( ... )
    {
        LOG_INFO ( "[ldb]db->Put exception[path=%s]", m_path.c_str () );
        return EFAULT;
    }

    if ( unlikely ( ! status.ok () ) )
    {
        check_put_ok.cancel ();
        std::string msg;
        try
        {
            msg = status.ToString ();
        }
        catch ( ... )
        {
        }
        LOG_INFO ( "[ldb]db->Put error: %s[path=%s]", m_path.c_str (), msg.c_str () );
        return EFAULT;
    }

    if ( MD5_BLOOM_DISABLED != ( md5_bloom_mode_t ) m_config.my_bloom_filter_type )
    {
        bloomfilter_add ( key, key_len );
    }

    check_put_fail.cancel ();

    return 0;
}

void kv_leveldb_t::bloomfilter_add (
                                     const void *        key,
                                     unsigned int        key_len
                                     )
{
    scope_perf_target_t check_add ( m_perf_bloom_add );

    char md5_val[ 16 ];
    G_APPTOOL->md5 ( key, key_len, md5_val );

    m_inner->my_bloom_filter.add_key ( md5_val );
}

bool kv_leveldb_t::bloomfilter_is_not_found (
                                              const void *        key,
                                              unsigned int        key_len
                                              )
{
    scope_perf_target_t check_hit ( m_perf_bloom_hit );
    scope_perf_target_t check_not_hit ( m_perf_bloom_not_hit );

    char md5_val[ 16 ];
    G_APPTOOL->md5 ( key, key_len, md5_val );

    bool b = m_inner->my_bloom_filter.not_found ( md5_val );
    if ( b )
    {
        check_not_hit.cancel ();
    }
    else
    {
        check_hit.cancel ();
    }

    return b;
}

int kv_leveldb_t::get (
                        const void *        key,
                        unsigned int        key_len,
                        std::string &       value
                        )
{
    value.resize ( 0 );

    if ( ! check_key ( ( const char * ) key, key_len ) )
    {
        LOG_INFO ( "[ldb]invalid key[path=%s]", m_path.c_str () );
        return EINVAL;
    }
    if ( unlikely ( NULL == m_inner->db ) )
    {
        LOG_INFO ( "[ldb]m_inner->db is NULL[path=%s]", m_path.c_str () );
        return EFAULT;
    }

    leveldb::Status status;

    scope_perf_target_t check_not_found ( m_perf_not_found );
    scope_perf_target_t check_found ( m_perf_not_found );

    if ( MD5_BLOOM_DISABLED != ( md5_bloom_mode_t ) m_config.my_bloom_filter_type )
    {
        if ( bloomfilter_is_not_found ( ( const char * ) key, ( size_t ) key_len ) )
        {
            return ENOENT;
        }
    }

    try
    {
        leveldb::Slice k ( ( const char * ) key, ( size_t ) key_len );
        status = m_inner->db->Get (
                                   leveldb::ReadOptions (),
                                   k,
                                   & value
                                   );
    }
    catch ( ... )
    {
        LOG_INFO ( "[ldb]db->Get exception[path=%s]", m_path.c_str () );
        value.resize ( 0 );
        return EFAULT;
    }

    if ( unlikely ( ! status.ok () ) )
    {
        value.resize ( 0 );
        if ( status.IsNotFound () )
        {
            check_found.cancel ();
            return ENOENT;
        }
        else
        {
            check_found.cancel ();
            check_not_found.cancel ();
            std::string msg;
            try
            {
                msg = status.ToString ();
            }
            catch ( ... )
            {
            }
            LOG_INFO ( "[ldb]db->Get error: %s[path=%s]", msg.c_str (), m_path.c_str () );
            return EFAULT;
        }
    }

    check_not_found.cancel ();
    return 0;
}

i_iterator_t * kv_leveldb_t::iterator ( )
{
    kv_iterator_t * it = new kv_iterator_t ();
    if ( ! it->create ( * this ) )
    {
        LOG_ERROR ( "[leveldb][iterator] create failed" );
        delete it;
        return NULL;
    }

    return it;
}

struct kv_iterator_t::inner_t
{
    leveldb::Iterator * m_iterator;

    int                 m_status;

    inner_t ( )
    : m_iterator ( NULL )
    , m_status ( ENOENT )
    {
    }

    ~ inner_t ( )
    {
        if ( m_iterator )
        {
            delete m_iterator;
            m_iterator = NULL;
        }
    }
} ;

void kv_iterator_t::kill_me ( )
{
    delete this;
}

kv_iterator_t::kv_iterator_t ( )
: m_inner ( NULL )
{
}

kv_iterator_t::~ kv_iterator_t ( )
{
    destroy ();
}

void kv_iterator_t::destroy ( )
{
    if ( m_inner )
    {
        delete m_inner;
        m_inner = NULL;
    }
}

bool kv_iterator_t::create ( kv_leveldb_t & db )
{
    if ( NULL == m_inner )
    {
        try
        {
            m_inner = new inner_t ();
        }
        catch ( ... )
        {
            LOG_ERROR ( "[leveldb][iterator]bad_alloc" );
            return false;
        }
    }

    m_inner->m_status = EFAULT;
    leveldb::DB * p = ( leveldb::DB * )db.get_internal_db ();
    if ( NULL == p )
    {
        LOG_ERROR ( "[leveldb][iterator]db NULL" );
        return false;
    }

    leveldb::ReadOptions options;
    m_inner->m_iterator = p->NewIterator ( options );
    if ( NULL == m_inner->m_iterator )
    {
        LOG_ERROR ( "[leveldb][iterator]NewIterator return NULL" );
        return false;
    }

    m_inner->m_status = ENOENT;
    return true;
}

bool kv_iterator_t::valid ( ) const
{
    return m_inner->m_iterator->Valid ();
}

int kv_iterator_t::status ( ) const
{
    return m_inner->m_status;
}

void kv_iterator_t::seek_first ( )
{
    m_inner->m_status = EFAULT;
    m_inner->m_iterator->SeekToFirst ();
    trans_errno ( __FUNCTION__ );
}

void kv_iterator_t::seek_last ( )
{
    m_inner->m_status = EFAULT;
    m_inner->m_iterator->SeekToLast ();
    trans_errno ( __FUNCTION__ );
}

void kv_iterator_t::seek ( const void * key, unsigned int key_len )
{
    m_inner->m_status = EFAULT;
    leveldb::Slice k ( ( const char * ) key, ( size_t ) key_len );
    m_inner->m_iterator->Seek ( k );
    trans_errno ( __FUNCTION__ );
}

void kv_iterator_t::next ( )
{
    m_inner->m_status = EFAULT;
    m_inner->m_iterator->Next ();
    trans_errno ( __FUNCTION__ );
}

void kv_iterator_t::prev ( )
{
    m_inner->m_status = EFAULT;
    m_inner->m_iterator->Prev ();
    trans_errno ( __FUNCTION__ );
}

const char * kv_iterator_t::key ( size_t * len ) const
{
    m_inner->m_status = EFAULT;
    leveldb::Slice slice = m_inner->m_iterator->key ();

    const char * r = slice.data ();
    size_t sz = slice.size ();
    if ( NULL == r || 0 == sz )
    {
        if ( len )
        {
            * len = 0;
        }
        m_inner->m_status = EINVAL;
        return NULL;
    }

    if ( NULL != len )
    {
        * len = sz;
    }
    m_inner->m_status = 0;
    return r;
}

const char * kv_iterator_t::value ( size_t * len ) const
{
    m_inner->m_status = EFAULT;
    leveldb::Slice slice = m_inner->m_iterator->value ();

    const char * r = slice.data ();
    size_t sz = slice.size ();
    if ( NULL == r )
    {
        if ( len )
        {
            * len = 0;
        }
        m_inner->m_status = EINVAL;
        return NULL;
    }

    if ( NULL != len )
    {
        * len = sz;
    }
    m_inner->m_status = 0;
    return r;
}

void kv_iterator_t::trans_errno ( const char * func_name )
{
    leveldb::Status r = m_inner->m_iterator->status ();

    if ( r.ok () )
    {
        m_inner->m_status = 0;
    }
    else if ( r.IsNotFound () )
    {
        m_inner->m_status = ENOENT;
    }
    else
    {
        m_inner->m_status = EFAULT;

        try
        {
            std::string s = r.ToString ();
            LOG_ERROR ( "[leveldb][iterator][%s]%s", func_name, s.c_str () );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[leveldb][iterator][%s] !!!bad_alloc!!!", func_name );
        }
    }
}
