#include "fullkey_array.h"
#include "../../base.h"

namespace md5db
{

    fullkey_array_t::fullkey_array_t ( )
    {
    }

    fullkey_array_t::~ fullkey_array_t ( )
    {
        close ();
    }

    void fullkey_array_t::close ( )
    {
        for ( int i = 0; i < COUNT_OF ( m_fullkeys ); ++ i )
        {
            m_fullkeys[ i ].close ();
        }
    }

    bool fullkey_array_t::open (
                                 const char * path
                                 )
    {
        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid path" );
            return false;
        }

        G_APPTOOL->make_dir ( path );

        char ph[ 260 ];

        for ( int i = 0; i < COUNT_OF ( m_fullkeys ); ++ i )
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
            strcat ( ph, ".fullkey" );

            if ( ! m_fullkeys[ i ].open ( ph, i ) )
            {
                LOG_ERROR ( "[md5db][fullkey]open( %s ) failed", ph );
                return false;
            }
        }

        return true;
    }

    fullkey_t & fullkey_array_t::get_fullkey (
                                               const void * inner_key,
                                               size_t inner_key_len
                                               )
    {
        assert ( inner_key_len >= 16 );
        unsigned char c = * ( ( const unsigned char * ) inner_key );
        return m_fullkeys[ c ];
    }

    void fullkey_array_t::info (
                                 std::stringstream & ss
                                 )
    {
        size_t count = size ();
        for ( size_t i = 0; i < count; ++ i )
        {
            fullkey_t & p = item ( i );
            p.info ( ss );
            
            if ( i < count - 1  )
            {
                ss << ",";
            }
        }
    }

} // namespace md5db
