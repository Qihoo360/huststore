#include "bucket_array.h"
#include "fullkey_array.h"
#include "../../base.h"

namespace md5db
{

    bucket_array_t::bucket_array_t ( )
    {
    }

    bucket_array_t::~ bucket_array_t ( )
    {
        close ();
    }

    void bucket_array_t::set_fullkeys ( fullkey_array_t * p )
    {
        for ( int i = 0; i < COUNT_OF ( m_buckets ); ++ i )
        {
            m_buckets[ i ].set_fullkey ( & p->item ( i ) );
        }
    }

    void bucket_array_t::close ( )
    {
        for ( int i = 0; i < COUNT_OF ( m_buckets ); ++ i )
        {
            m_buckets[ i ].close ();
        }
    }

    bool bucket_array_t::create_buckets ( const char * path )
    {
        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][bucket_array][create_buckets]path is empty" );
            return false;
        }

        G_APPTOOL->make_dir ( path );

        char ph[ 260 ];

        for ( int i = 0; i < COUNT_OF ( m_buckets ); ++ i )
        {
            strcpy ( ph, path );
            G_APPTOOL->path_to_os ( ph );
            if ( S_PATH_SEP_C != ph[ strlen ( ph ) - 1 ] )
            {
                strcat ( ph, S_PATH_SEP );
            }

            char t[ 32 ];
            sprintf ( t, "%02X", ( int ) i );
            strcat ( ph, t );
            strcat ( ph, ".bucket" );

            if ( ! m_buckets[ i ].create ( ph ) )
            {
                LOG_ERROR ( "[md5db][bucket_array][create_buckets][file=%s][i=%d]create failed", 
                            ph, i );
                return false;
            }
        }

        return true;
    }

    bool bucket_array_t::open_exist_buckets ( const char * path, bool read_write )
    {
        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][bucket_array][open_exist_buckets]invalid path" );
            return false;
        }

        char ph[ 260 ];

        for ( int i = 0; i < COUNT_OF ( m_buckets ); ++ i )
        {
            strcpy ( ph, path );
            G_APPTOOL->path_to_os ( ph );
            if ( S_PATH_SEP_C != ph[ strlen ( ph ) - 1 ] )
            {
                strcat ( ph, S_PATH_SEP );
            }

            char t[ 32 ];
            sprintf ( t, "%02X", ( int ) i );
            strcat ( ph, t );
            strcat ( ph, ".bucket" );

            if ( ! G_APPTOOL->is_file ( ph ) )
            {
                return false;
            }
            if ( ! m_buckets[ i ].open_exist ( ph, read_write ) )
            {
                LOG_ERROR ( "[md5db][bucket_array][open_exist_buckets][file=%s][i=%d]open_exist failed", 
                            ph, i );
                return false;
            }
        }

        return true;
    }

    bool bucket_array_t::open ( const char * path )
    {
        bool read_write = true;

        if ( ! open_exist_buckets ( path, read_write ) )
        {
            if ( ! create_buckets ( path ) )
            {
                LOG_ERROR ( "[md5db]bucket_array][open]create failed" );
                return false;
            }
            if ( ! open_exist_buckets ( path, read_write ) )
            {
                LOG_ERROR ( "[md5db][bucket_array][open]open_exist failed" );
                return false;
            }
        }

        return true;
    }

    bucket_t & bucket_array_t::get_bucket ( const void * inner_key, size_t inner_key_len )
    {
        //assert ( inner_key_len == 16 );
        unsigned char c = * ( ( const unsigned char * ) inner_key );
        return m_buckets[ c ];
    }

} // namespace md5db
