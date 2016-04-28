#include "conflict_array.h"
#include "../leveldb/bloom_filter.h"
#include <memory>

namespace md5db
{

    conflict_array_t::conflict_array_t ( )
    : m_conflicts ( )
    {
    }

    conflict_array_t::~ conflict_array_t ( )
    {
        close ();
    }

    void conflict_array_t::close ( )
    {
        if ( ! m_conflicts.empty () )
        {
            for ( int i = 0; i < ( int ) m_conflicts.size (); ++ i )
            {
                conflict_t * p = m_conflicts[ i ];
                m_conflicts[ i ] = NULL;
                if ( p )
                {
                    delete p;
                }
            }
            m_conflicts.resize ( 0 );
        }
    }

    bool conflict_array_t::open (
                                  const char * path,
                                  const char * storage_conf
                                  )
    {
        ini_t * ini = NULL;

        ini = G_APPINI->ini_create ( storage_conf );
        if ( NULL == ini )
        {
            LOG_ERROR ( "[conflict]open %s failed", storage_conf );
            return false;
        }

        bool b = open ( path, * ini );
        G_APPINI->ini_destroy ( ini );

        if ( ! b )
        {
            LOG_ERROR ( "[conflict]open failed" );
            return false;
        }

        return true;
    }

    bool conflict_array_t::open (
                                  const char *    path,
                                  ini_t &         ini
                                  )
    {
        int count                = G_APPINI->ini_get_int ( & ini, "conflictdb", "count", 2 );
        if ( count <= 0 || count > 10 )
        {
            LOG_ERROR ( "[conflict]invalid count: %d", count );
            return false;
        }

        int cache_size_m         = G_APPINI->ini_get_int ( & ini, "conflictdb", "cache", 128 );
        int write_buffer_m       = G_APPINI->ini_get_int ( & ini, "conflictdb", "write_buffer", 128 );
        int bloom_filter_bits    = G_APPINI->ini_get_int ( & ini, "conflictdb", "bloom_filter_bits", 0 );
        bool disable_compression = G_APPINI->ini_get_bool ( & ini, "conflictdb", "disable_compression", true );
        const char * s           = G_APPINI->ini_get_string ( & ini, "conflictdb", "md5_bloom_filter", "large" );
        if ( NULL == s )
        {
            LOG_ERROR ( "[conflict]invalid [conflict]md5_bloom_filter" );
            return false;
        }

        md5_bloom_mode_t mode;
        if ( 0 == strcmp ( s, "none" ) )
        {
            mode = MD5_BLOOM_DISABLED;
        }
        else if ( 0 == strcmp ( s, "large" ) )
        {
            mode = MD5_BLOOM_LARGE;
        }
        else if ( 0 == strcmp ( s, "small" ) )
        {
            mode = MD5_BLOOM_SMALL;
        }
        else
        {
            LOG_ERROR ( "[conflict]invalid [conflict]md5_bloom_filter '%s'", s );
            return false;
        }

        return open ( path, count, cache_size_m, write_buffer_m, bloom_filter_bits, disable_compression, ( int ) mode );
    }

    bool conflict_array_t::open (
                                  const char *    path,
                                  int             count,
                                  int             cache_m,
                                  int             write_buffer_m,
                                  int             bloom_filter_bits,
                                  bool            disable_compression,
                                  int             md5_bloom_filter_type
                                  )
    {
        if ( ! m_conflicts.empty () )
        {
            LOG_ERROR ( "[md5db][conflict]m_conflicts not empty" );
            return false;
        }
        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][conflict]path empty" );
            return false;
        }
        if ( count <= 0 )
        {
            LOG_ERROR ( "[md5db][conflict]invalid count %d", count );
            return false;
        }
        if ( cache_m < 0 )
        {
            cache_m = CONFLICT_CACHE_M;
        }
        if ( write_buffer_m < 0 )
        {
            write_buffer_m = CONFLICT_WRITE_BUFFER_M;
        }
        if ( bloom_filter_bits < 0 )
        {
            bloom_filter_bits = 0;
        }

        G_APPTOOL->make_dir ( path );

        char ph[ 260 ];

        if ( m_conflicts.capacity () < ( size_t ) count )
        {
            try
            {
                m_conflicts.reserve ( count );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][conflict]bad_alloc" );
                return false;
            }
        }
        for ( int i = 0; i < count; ++ i )
        {
            strcpy ( ph, path );
            G_APPTOOL->path_to_os ( ph );
            if ( S_PATH_SEP_C != ph[ strlen ( ph ) - 1 ] )
            {
                strcat ( ph, S_PATH_SEP );
            }

            char t[ 32 ];
            sprintf ( t, "%02d", ( int ) i );
            strcat ( ph, t );
            strcat ( ph, ".conflict" );

            std::auto_ptr< conflict_t > o;
            try
            {
                o.reset ( new conflict_t () );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][conflict]bad_alloc" );
                return false;
            }

            kv_config_t config;
            memset ( & config, 0, sizeof ( kv_config_t ) );
            config.cache_size_m            = cache_m;
            config.write_buffer_m          = write_buffer_m;
            config.ldb_bloom_filter_bits   = bloom_filter_bits;
            config.my_bloom_filter_type    = ( md5_bloom_mode_t ) md5_bloom_filter_type;
            config.is_readonly             = false;
            config.disable_compression     = disable_compression;
            if ( ! o->open ( ph, config, i ) )
            {
                LOG_ERROR ( "[md5db][conflict]open %s failed", ph );
                return false;
            }

            m_conflicts.push_back ( o.release () );
        }

        return true;
    }

    conflict_t & conflict_array_t::get_conflict ( const void * inner_key, size_t inner_key_len )
    {
        assert ( inner_key_len == 16 );
        assert ( ! m_conflicts.empty () );
        uint32_t hash_id = * ( ( const uint32_t * ) inner_key );
        return * m_conflicts[ hash_id % m_conflicts.size () ];
    }

    int conflict_array_t::del (
                                const void *        inner_key,
                                unsigned int        inner_key_len
                                )
    {
        conflict_t & c = get_conflict ( inner_key, inner_key_len );
        return c.del ( inner_key, inner_key_len );
    }

    int conflict_array_t::put (
                                const void *        inner_key,
                                unsigned int        inner_key_len,
                                const block_id_t &  block_id,
                                uint32_t            version
                                )
    {
        conflict_t & c = get_conflict ( inner_key, inner_key_len );
        return c.put ( inner_key, inner_key_len, block_id, version );
    }

    int conflict_array_t::get (
                                const void *        inner_key,
                                unsigned int        inner_key_len,
                                block_id_t &        block_id,
                                uint32_t &          version
                                )
    {
        conflict_t & c = get_conflict ( inner_key, inner_key_len );
        return c.get ( inner_key, inner_key_len, block_id, version );
    }

    void conflict_array_t::info (
                                  std::stringstream & ss
                                  )
    {
        size_t count = m_conflicts.size ();
        for ( size_t i = 0; i < count; ++ i )
        {
            conflict_t * p = m_conflicts[ i ];
            
            if ( p )
            {
                p->info ( ss );

                if ( i < count - 1 )
                {
                    ss << ",";
                }
            }
        }
    }

    conflict_t * conflict_array_t::item (
                                          int index
                                          )
    {
        if ( index < 0 || ( size_t ) index > m_conflicts.size () )
        {
            return NULL;
        }

        return m_conflicts[ index ];
    }

} // namespace md5db
