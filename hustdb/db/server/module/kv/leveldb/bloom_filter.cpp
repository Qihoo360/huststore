#include "bloom_filter.h"

#define SMALL_BITS  ( 256 * 256 * 256 )
#define SMALL_BYTES ( SMALL_BITS / 8 )

#define LARGE_BITS  ( 256 * 256 * 256 * 16 )
#define LARGE_BYTES ( LARGE_BITS / 8 )

static const unsigned char BIT_MASK_MAP[ CHAR_BIT ] = {
                                                       0x80,       // 10000000
                                                       0x40,       // 01000000
                                                       0x20,       // 00100000
                                                       0x10,       // 00010000
                                                       0x08,       // 00001000
                                                       0x04,       // 00000100
                                                       0x02,       // 00000010
                                                       0x01        // 00000001
};

static inline
unsigned char byte_set_bit ( unsigned char src, unsigned char witch_bit, bool v )
{
    assert ( witch_bit < CHAR_BIT );
    if ( ! v )
        return src & ~ BIT_MASK_MAP[ witch_bit ];
    else
        return src | BIT_MASK_MAP[ witch_bit ];
}

static inline
bool byte_get_bit ( unsigned char src, unsigned char witch_bit )
{
    assert ( witch_bit < CHAR_BIT );
    return ! ( ! ( src & BIT_MASK_MAP[ witch_bit ] ) ) ? true : false;
}

static inline
void binary_set_bit ( unsigned char * src, size_t witch_bit, bool v )
{
    size_t byte_off = witch_bit / CHAR_BIT;
    assert ( src );
    src[ byte_off ] = byte_set_bit ( src[ byte_off ], ( unsigned char ) ( witch_bit % CHAR_BIT ), v );
}

static inline
bool binary_get_bit ( const unsigned char * src, size_t witch_bit )
{
    assert ( src );
    return byte_get_bit ( src[ ( witch_bit / CHAR_BIT ) ], ( unsigned char ) ( witch_bit % CHAR_BIT ) );
}

md5_bloom_filter_t::md5_bloom_filter_t ( )
: m_data ( NULL )
{
    G_APPTOOL->fmap_init ( & m_fmap );
}

md5_bloom_filter_t::~ md5_bloom_filter_t ( )
{
    close ();
}

bool md5_bloom_filter_t::open ( const char * path, md5_bloom_mode_t type )
{
#if S_LITTLE_ENDIAN
    {
        uint32_t bit_id = 0xF123456;
        unsigned char * p = ( unsigned char * ) & bit_id;
        if ( 0x56 != p[ 0 ] || 0x34 != p[ 1 ] || 0x12 != p[ 2 ] || 0x0F != p[ 3 ] )
        {
            LOG_ERROR ( "[bloom]invalid CPU, we can not support: %X %X %X %X %08X",
                       p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], bit_id );
            return false;
        }

        const unsigned char md5[] = {
                                     0xF1, 0x23, 0x45, 0x63, 0x44, 0x55, 0x66, 0x77,
                                     0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
        };
        {
            // 0F 12 34 56
            uint32_t bit_id2 = 0;
            unsigned char * p2 = ( unsigned char * ) & bit_id2;
            p2[ 3 ] = ( md5[ 0 ] & 0xF0 ) >> 4;
            p2[ 2 ] = ( ( md5[ 0 ] & 0x0F ) << 4 ) | ( ( md5[ 1 ] & 0xF0 ) >> 4 );
            p2[ 1 ] = ( ( md5[ 1 ] & 0x0F ) << 4 ) | ( ( md5[ 2 ] & 0xF0 ) >> 4 );
            p2[ 0 ] = ( ( md5[ 2 ] & 0x0F ) << 4 ) | ( ( md5[ 3 ] & 0xF0 ) >> 4 );
            if ( 0x56 != p2[ 0 ] || 0x34 != p2[ 1 ] || 0x12 != p2[ 2 ] || 0x0F != p2[ 3 ] )
            {
                LOG_ERROR ( "[bloom]invalid CPU, we can not support: %X %X %X %X %08X",
                           p2[ 0 ], p2[ 1 ], p2[ 2 ], p2[ 3 ], bit_id );
                return false;
            }
        }
        {
            // 0F 12
            uint16_t bit_id2 = 0;
            unsigned char * p2 = ( unsigned char * ) & bit_id2;
            p2[ 1 ] = ( md5[ 0 ] & 0xF0 ) >> 4;
            p2[ 0 ] = ( ( md5[ 0 ] & 0x0F ) << 4 ) | ( ( md5[ 1 ] & 0xF0 ) >> 4 );
            if ( 0x12 != p2[ 0 ] || 0x0F != p2[ 1 ] )
            {
                LOG_ERROR ( "[bloom]invalid CPU, we can not support: %X %X %04X",
                           p2[ 0 ], p2[ 1 ], bit_id );
                return false;
            }
        }
    }
#endif // #if S_LITTLE_ENDIAN

    if ( NULL == path || '\0' == * path )
    {
        LOG_ERROR ( "[bloom]error" );
        return false;
    }

    if ( ! G_APPTOOL->is_file ( path ) )
    {

        char            ph[ 256 ];
        FILE *          fp;
        std::string     buf;

        if ( strlen ( path ) >= sizeof ( ph ) )
        {
            LOG_ERROR ( "[bloom]error" );
            return false;
        }
        strcpy ( ph, path );
        G_APPTOOL->path_to_os ( ph );

        try
        {
            if ( MD5_BLOOM_LARGE == type )
            {
                buf.resize ( LARGE_BYTES + 1, '\0' );
            }
            else if ( MD5_BLOOM_SMALL == type )
            {
                buf.resize ( SMALL_BYTES + 1, '\0' );
            }
            else
            {
                LOG_ERROR ( "[bloom]invalid bloom_filter type %d", ( int ) type );
                return false;
            }
        }
        catch ( ... )
        {
            LOG_ERROR ( "[bloom]error" );
            return false;
        }

        fp = fopen ( ph, "wb" );
        if ( NULL == fp )
        {
            LOG_ERROR ( "[bloom]error" );
            return false;
        }

        if ( buf.size () != fwrite ( buf.c_str (), 1, buf.size (), fp ) )
        {
            LOG_ERROR ( "[bloom]error" );
            fclose ( fp );
            return false;
        }

        fclose ( fp );
    }

    if ( ! G_APPTOOL->fmap_open ( & m_fmap, path, 0, 0, true ) )
    {
        LOG_ERROR ( "[bloom]fmap_open( %s ) failed", path );
        return false;
    }
    if ( MD5_BLOOM_LARGE == type )
    {
        if ( LARGE_BYTES + 1 != m_fmap.ptr_len )
        {
            LOG_ERROR ( "[bloom]error" );
            G_APPTOOL->fmap_close ( & m_fmap );
            return false;
        }
    }
    else
    {
        if ( SMALL_BYTES + 1 != m_fmap.ptr_len )
        {
            LOG_ERROR ( "[bloom]error" );
            G_APPTOOL->fmap_close ( & m_fmap );
            return false;
        }
    }

    m_data  = ( unsigned char * ) m_fmap.ptr;
    m_type  = type;
    return true;
}

void md5_bloom_filter_t::close ( )
{
    m_data = NULL;
    G_APPTOOL->fmap_close ( & m_fmap );
}

bool md5_bloom_filter_t::not_found ( const char * md5 )
{
    assert ( NULL != md5 );

    if ( MD5_BLOOM_LARGE == m_type )
    {

        uint32_t bit_id;
        unsigned char * p = ( unsigned char * ) & bit_id;
#if S_LITTLE_ENDIAN
        p[ 3 ] = ( ( unsigned char ) md5[ 0 ] & 0xF0 ) >> 4;
        p[ 2 ] = ( ( ( unsigned char ) md5[ 0 ] & 0x0F ) << 4 ) | ( ( ( unsigned char ) md5[ 1 ] & 0xF0 ) >> 4 );
        p[ 1 ] = ( ( ( unsigned char ) md5[ 1 ] & 0x0F ) << 4 ) | ( ( ( unsigned char ) md5[ 2 ] & 0xF0 ) >> 4 );
        p[ 0 ] = ( ( ( unsigned char ) md5[ 2 ] & 0x0F ) << 4 ) | ( ( ( unsigned char ) md5[ 3 ] & 0xF0 ) >> 4 );
#else
#error unsupport CPU 
#endif
        if ( unlikely ( bit_id > LARGE_BITS ) )
        {
            LOG_ERROR ( "[bloom]algorithm for large bloom filter error!!!!!!!!!!!!!!!" );
            return false;
        }

        if ( ! binary_get_bit ( m_data, bit_id ) )
        {
            return true;
        }

        return false;
    }
    else
    {
        uint32_t bit_id;
        unsigned char * p = ( unsigned char * ) & bit_id;
#if S_LITTLE_ENDIAN
        p[ 0 ] = ( unsigned char ) md5[ 2 ];
        p[ 1 ] = ( unsigned char ) md5[ 1 ];
        p[ 2 ] = ( unsigned char ) md5[ 0 ];
        p[ 3 ] = 0;
#else
#error unsupport CPU 
#endif
        if ( unlikely ( bit_id > SMALL_BITS ) )
        {
            LOG_ERROR ( "[bloom]algorithm for small bloom filter error!!!!!!!!!!!!!!!" );
            return false;
        }

        if ( ! binary_get_bit ( m_data, bit_id ) )
        {
            return true;
        }

        return false;
    }
}

void md5_bloom_filter_t::add_key ( const char * md5 )
{
    assert ( NULL != md5 );

    if ( MD5_BLOOM_LARGE == m_type )
    {

        uint32_t bit_id;
        unsigned char * p = ( unsigned char * ) & bit_id;
#if S_LITTLE_ENDIAN
        p[ 3 ] = ( ( unsigned char ) md5[ 0 ] & 0xF0 ) >> 4;
        p[ 2 ] = ( ( ( unsigned char ) md5[ 0 ] & 0x0F ) << 4 ) | ( ( ( unsigned char ) md5[ 1 ] & 0xF0 ) >> 4 );
        p[ 1 ] = ( ( ( unsigned char ) md5[ 1 ] & 0x0F ) << 4 ) | ( ( ( unsigned char ) md5[ 2 ] & 0xF0 ) >> 4 );
        p[ 0 ] = ( ( ( unsigned char ) md5[ 2 ] & 0x0F ) << 4 ) | ( ( ( unsigned char ) md5[ 3 ] & 0xF0 ) >> 4 );
#else
#error unsupport CPU 
#endif
        if ( unlikely ( bit_id > LARGE_BITS ) )
        {
            LOG_ERROR ( "[bloom]algorithm for large bloom filter error!!!!!!!!!!!!!!!" );
            return;
        }

        binary_set_bit ( m_data, bit_id, true );

    }
    else
    {

        uint32_t bit_id;
        unsigned char * p = ( unsigned char * ) & bit_id;
#if S_LITTLE_ENDIAN
        p[ 0 ] = ( unsigned char ) md5[ 2 ];
        p[ 1 ] = ( unsigned char ) md5[ 1 ];
        p[ 2 ] = ( unsigned char ) md5[ 0 ];
        p[ 3 ] = 0;
#else
#error unsupport CPU 
#endif
        if ( unlikely ( bit_id > SMALL_BITS ) )
        {
            LOG_ERROR ( "[bloom]algorithm for small bloom filter error!!!!!!!!!!!!!!!" );
            return;
        }

        binary_set_bit ( m_data, bit_id, true );
    }
}
