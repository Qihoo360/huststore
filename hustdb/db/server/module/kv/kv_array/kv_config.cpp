#include "kv_config.h"
#include "key_hash.h"
#include "../leveldb/bloom_filter.h"

config_t::config_t ( )
: m_key_hash ( NULL )
, m_readonly ( true )
, m_disable_compression ( true )
, m_md5_bloom_filter ( MD5_BLOOM_DISABLED )
, m_key_len ( 0 )
, m_ldb_bloom_filter_bits ( 0 )
, m_record_bytes ( 0 )
, m_files ( )
, m_base_dir ( )
, m_cache_size_m ( - 1 )
, m_write_buffer_m ( - 1 )
{
    memset ( & m_kv_config, 0, sizeof ( m_kv_config ) );
}

config_t::~ config_t ( )
{
}

const char * config_t::get_file_path ( int i )
{
    if ( unlikely ( i < 0 || 
                    i >= ( int ) m_files.size () 
                ) 
        )
    {
        LOG_ERROR ( "[config][get_file_path][file=%d][file_size=%d]failed", 
                    i, ( int ) m_files.size () );
        return NULL;
    }

    return m_files[ i ].c_str ();
}

bool config_t::open (
                      const char *    config_path,
                      int             server_id,
                      const char *    base_dir,
                      key_hash_t &    hash,
                      kv_array_t &    files
                      )
{
    if ( ! config_path || ! * config_path )
    {
        LOG_ERROR ( "[config][open]param error" );
        return false;
    }

    char ph[ MAX_PATH ];
    strncpy ( ph, config_path, MAX_PATH );
    ph[ MAX_PATH - 1 ] = '\0';
    G_APPTOOL->path_to_os ( ph );

    std::string path;

    if ( NULL == base_dir || '\0' == * base_dir )
    {
        const char * p = ( const char * ) strrchr ( ph, S_PATH_SEP_C );
        if ( NULL == p )
        {
            LOG_ERROR ( "[config][open][%s in %s]can not find", 
                        S_PATH_SEP, ph );
            return false;
        }

        path.assign ( ( const char * ) ph, ( const char * ) p );
        if ( ! G_APPTOOL->is_dir ( path.c_str () ) )
        {
            LOG_ERROR ( "[config][open][file=%s]is not valid directory", 
                        path.c_str () );
            return false;
        }

        m_base_dir = path;
    }
    else
    {
        m_base_dir = base_dir;
    }

    path = ph;

    ini_t * ini = NULL;

    ini = G_APPINI->ini_create ( path.c_str () );
    if ( NULL == ini )
    {
        LOG_ERROR ( "[config][open][file=%s]open failed", 
                    path.c_str () );
        return false;
    }

    bool b = open ( * ini, server_id, hash, files );
    G_APPINI->ini_destroy ( ini );

    if ( ! b )
    {
        LOG_ERROR ( "[config][open]open failed" );
        return false;
    }

    return true;
}

bool config_t::open_memory (
                             const char *    base_dir,
                             const char *    content,
                             int             server_id,
                             size_t          content_bytes,
                             key_hash_t &    hash,
                             kv_array_t &    files
                             )
{
    if ( NULL == base_dir || '\0' == * base_dir )
    {
        LOG_ERROR ( "[config][open_memory]error" );
        return false;
    }

    m_base_dir = base_dir;

    ini_t * ini = NULL;

    ini = G_APPINI->ini_create_memory ( content, content_bytes );
    if ( NULL == ini )
    {
        LOG_ERROR ( "[config][open_memory]open ini failed" );
        return false;
    }

    bool b = open ( *ini, server_id, hash, files );
    G_APPINI->ini_destroy ( ini );

    if ( ! b )
    {
        LOG_ERROR ( "[config][open_memory]open failed" );
        return false;
    }

    return true;
}

int config_t::get_max_file_count ( )
{
    if ( NULL == m_key_hash )
    {
        LOG_ERROR ( "[config][get_max_file_count]key_hash is NULL" );
        return 0;
    }

    return m_key_hash->get_data_file_count ();
}

bool config_t::open (
                      ini_t &         ini,
                      int             server_id,
                      key_hash_t &    hash,
                      kv_array_t &    files
                      )
{
    int             r;
    const char *    s;

    do
    {

        if ( server_id < 0 )
        {
            LOG_ERROR ( "[config][open]invalid server_id" );
            return false;
        }

        m_key_hash = & hash;
        
        if ( ! hash.open ( DB_HASH_CONFIG, server_id, * this, files ) )
        {
            LOG_ERROR ( "[config][open][config=%s]hash.open failed", 
                        DB_HASH_CONFIG );
            r = - 99;
            break;
        }

        m_readonly            = G_APPINI->ini_get_bool ( & ini, "md5db", "read_only", false );
        m_disable_compression = G_APPINI->ini_get_bool ( & ini, "md5db", "disable_compression", true );

        s = G_APPINI->ini_get_string ( & ini, "md5db", "md5_bloom_filter", "none" );
        if ( NULL == s || '\0' == * s )
        {
            s = "large";
        }
        
        if ( 0 == stricmp ( "large", s ) )
        {
            m_md5_bloom_filter = ( int ) MD5_BLOOM_LARGE;
        }
        else if ( 0 == stricmp ( "small", s ) )
        {
            m_md5_bloom_filter = ( int ) MD5_BLOOM_SMALL;
        }
        else if ( 0 == stricmp ( "disable", s ) || 
                  0 == stricmp ( "disabled", s ) || 
                  0 == stricmp ( "no", s ) || 
                  0 == stricmp ( "none", s ) )
        {
            m_md5_bloom_filter = ( int ) MD5_BLOOM_DISABLED;
        }
        else
        {
            LOG_ERROR ( "[config][open][type=%s]invalid md5_bloom_filter", 
                        s );
            r = - 102;
            break;
        }

        m_record_bytes = G_APPINI->ini_get_int ( & ini, "md5db", "record_bytes", 0 );
        if ( m_record_bytes < 0 )
        {
            LOG_ERROR ( "[config][open][record_bytes=%d]invalid record_bytes", 
                        m_record_bytes );
            r = - 2;
            break;
        }
        else if ( 0 == m_record_bytes )
        {
            LOG_INFO ( "[config][open]record_bytes is 0, only limit by server" );
        }

        m_key_len = G_APPINI->ini_get_int ( & ini, "md5db", "key_len", 0 );
        if ( m_key_len < 0 )
        {
            LOG_ERROR ( "[config][open]invalid key_len" );
            r = - 3;
            break;
        }
        else if ( 0 == m_key_len )
        {
            LOG_INFO ( "[config][open]key_len is 0, only limit by server" );
        }

        m_ldb_bloom_filter_bits = G_APPINI->ini_get_int ( & ini, "md5db", "bloom_filter_bits", 10 );
        if ( m_ldb_bloom_filter_bits < 0 )
        {
            m_ldb_bloom_filter_bits = 0;
        }

        m_cache_size_m = G_APPINI->ini_get_int ( & ini, "md5db", "l1_cache", 256 );
        if ( m_cache_size_m < - 1 )
        {
            m_cache_size_m = - 1;
        }
        else if ( m_cache_size_m > 64 * 1024 )
        {
            LOG_ERROR ( "[config][open][size=%dMB]invalid cache_size", 
                        m_cache_size_m );
            r = - 45;
            break;
        }

        m_write_buffer_m = G_APPINI->ini_get_int ( & ini, "md5db", "write_buffer", 1024 );
        if ( m_write_buffer_m < - 1 )
        {
            m_write_buffer_m = - 1;
        }
        else if ( m_write_buffer_m > 2048 )
        {
            LOG_ERROR ( "[config][open][size=%dMB]invalid write_buffer", 
                        m_write_buffer_m );
            r = - 47;
            break;
        }

        int max_file_count = hash.get_data_file_count ();

        try
        {
            m_files.resize ( max_file_count + 2, "" );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[config][open][count=%d]invalid max_file_count too large", 
                        max_file_count );
            r = - 5;
            break;
        }

        std::string file;

        r = 0;

        for ( int i = 0; i < max_file_count + 2; ++ i )
        {
            char key[ 32 ];
            sprintf ( key, "%02d_%02d", i, i );

            file = DB_DATA_FILE;
            stdstr_replace ( file, "$(FILE_ID)", key );
            
            G_APPTOOL->make_dir ( file.c_str () );
            if ( ! G_APPTOOL->is_dir ( file.c_str () ) )
            {
                LOG_ERROR ( "[config][open][file=%s]make_dir failed", 
                            file.c_str () );
                r = - 52;
                break;
            }

            try
            {
                m_files[ i ] = file;
            }
            catch ( ... )
            {
                LOG_ERROR ( "[config][open][%d-%s]bad_alloc", 
                            i, file.c_str () );
                r = - 54;
                break;
            }
        }
        if ( 0 != r )
        {
            break;
        }

        m_kv_config.cache_size_m            = m_cache_size_m;
        m_kv_config.write_buffer_m          = m_write_buffer_m;
        m_kv_config.ldb_bloom_filter_bits   = m_ldb_bloom_filter_bits;
        m_kv_config.my_bloom_filter_type    = m_md5_bloom_filter;
        m_kv_config.is_readonly             = m_readonly;
        m_kv_config.disable_compression     = m_disable_compression;

    }
    while ( false );

    if ( 0 != r )
    {
        LOG_ERROR ( "[config][open][r=%d]open error", 
                    r );
        return false;
    }

    return true;
}

void config_t::close ( )
{
    m_record_bytes = 0;
    m_files.resize ( 0 );
    memset ( & m_kv_config, 0, sizeof ( m_kv_config ) );
}
