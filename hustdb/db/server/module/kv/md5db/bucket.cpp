#include "bucket.h"

namespace md5db
{

    bucket_t::bucket_t ( )
    : m_fullkey ( NULL )
    , m_lock ( )
    {
        G_APPTOOL->fmap_init ( & m_data );
    }

    bucket_t::~ bucket_t ( )
    {
        close ();
    }

    size_t bucket_t::bucket_item_bytes ( ) const
    {
        return sizeof ( bucket_data_item_t );
    }

    size_t bucket_t::bucket_item_count ( ) const
    {
        return 1048576;
    }

    size_t bucket_t::bucket_bytes ( ) const
    {
        return bucket_item_count () * bucket_item_bytes ();
    }

    void bucket_t::close ( )
    {
        G_APPTOOL->fmap_close ( & m_data );
    }

    bool bucket_t::create ( const char * path )
    {
        std::string t;
        FILE *      fp;

        try
        {
            t.resize ( bucket_bytes (), '\0' );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[md5db][bucket][create]bad_alloc" );
            return false;
        }

        fp = fopen ( path, "wb" );
        if ( NULL == fp )
        {
            LOG_ERROR ( "[md5db][bucket][create][file=%s]fopen failed", 
                        path );
            return false;
        }
        
        if ( t.size () != fwrite ( t.c_str (), 1, t.size (), fp ) )
        {
            LOG_ERROR ( "[md5db][bucket][create][file=%s]fwrite failed", 
                        path );
            fclose ( fp );
            return false;
        }
        fclose ( fp );

        return true;
    }

    bool bucket_t::open_exist ( const char * path, bool read_write )
    {
        if ( BLOCK_ID_BYTES != sizeof ( block_id_t ) )
        {
            LOG_ERROR ( "[md5db][bucket][open_exist][sizoef( block_id_t )=%d]", 
                        ( int ) sizeof ( block_id_t ) );
            return false;
        }
        if ( BUCKET_DATA_ITEM_BYTES != sizeof ( bucket_data_item_t ) )
        {
            LOG_ERROR ( "[md5db][bucket][open_exist][sizeof( bucket_data_item_t )=%d]", 
                        ( int ) sizeof ( bucket_data_item_t ) );
            return false;
        }

#if S_LITTLE_ENDIAN
        {
            const unsigned char md5[] = {
                                         0xF1, 0x23, 0x45, 0x63, 0x44, 0x55, 0x66, 0x77,
                                         0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
            };

            uint32_t bit_id = 0x023456;
            unsigned char * p = ( unsigned char * ) & bit_id;
            if ( 0x56 != p[ 0 ] || 0x34 != p[ 1 ] || 0x02 != p[ 2 ] || 0x00 != p[ 3 ] )
            {
                LOG_ERROR ( "[md5db][bucket]invalid CPU, we can not support: %X %X %X %X %08X",
                            p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], bit_id );
                return false;
            }

            uint32_t bit_id2 = 0;
            unsigned char * p2 = ( unsigned char * ) & bit_id2;
            p2[ 2 ] = ( md5[ 1 ] & 0xF0 ) >> 4;
            p2[ 1 ] = ( ( md5[ 1 ] & 0x0F ) << 4 ) | ( ( md5[ 2 ] & 0xF0 ) >> 4 );
            p2[ 0 ] = ( ( md5[ 2 ] & 0x0F ) << 4 ) | ( ( md5[ 3 ] & 0xF0 ) >> 4 );
            if ( 0x56 != p2[ 0 ] || 0x34 != p2[ 1 ] || 0x02 != p2[ 2 ] || 0 != p2[ 3 ] )
            {
                LOG_ERROR ( "[md5db][bucket]invalid CPU, we can not support: %X %X %X %X %08X",
                            p2[ 0 ], p2[ 1 ], p2[ 2 ], p2[ 3 ], bit_id );
                return false;
            }
        }
#endif

        if ( ! G_APPTOOL->fmap_open ( & m_data, path, 0, 0, read_write ) )
        {
            LOG_ERROR ( "[md5db][bucket][open_exist][file=%s]fmap_open failed", path );
            return false;
        }
        if ( m_data.ptr_len != bucket_bytes () )
        {
            LOG_ERROR ( "[md5db][bucket][open_exist][data.ptr_len=%d][bucket_bytes=%d]",
                        ( int ) m_data.ptr_len, ( int ) bucket_bytes () );
            return false;
        }

        return true;
    }

    void bucket_t::key_to_bytes3 (
                                   const char *    inner_key,
                                   size_t          inner_key_len,
                                   unsigned char   rsp[ 3 ]
                                   )
    {
        //assert ( inner_key_len >= 16 );

        const unsigned char * md5 = ( const unsigned char * ) inner_key;

#if S_LITTLE_ENDIAN
        rsp[ 0 ] = ( md5[ 1 ] & 0xF0 ) >> 4;
        rsp[ 1 ] = ( ( md5[ 1 ] & 0x0F ) << 4 ) | ( ( md5[ 2 ] & 0xF0 ) >> 4 );
        rsp[ 2 ] = ( ( md5[ 2 ] & 0x0F ) << 4 ) | ( ( md5[ 3 ] & 0xF0 ) >> 4 );
#else
#error unsupport CPU 
#endif
    }

    size_t bucket_t::key_to_item (
                                   const void *    inner_key,
                                   size_t          inner_key_len
                                   )
    {
        //assert ( inner_key_len >= 16 );

        const unsigned char * md5 = ( const unsigned char * ) inner_key;

        uint32_t r = 0;
        unsigned char * p = ( unsigned char * ) & r;
#if S_LITTLE_ENDIAN
        p[ 2 ] = ( md5[ 1 ] & 0xF0 ) >> 4;
        p[ 1 ] = ( ( md5[ 1 ] & 0x0F ) << 4 ) | ( ( md5[ 2 ] & 0xF0 ) >> 4 );
        p[ 0 ] = ( ( md5[ 2 ] & 0x0F ) << 4 ) | ( ( md5[ 3 ] & 0xF0 ) >> 4 );
#else
#error unsupport CPU 
#endif

        return r;
    }

    int bucket_t::find (
                        const void *            inner_key,
                        size_t                  inner_key_len,
                        bucket_data_item_t * &  result
                        )
    {
        //assert ( inner_key_len >= 16 );

        size_t index = key_to_item ( inner_key, inner_key_len );
        if ( index >= bucket_item_count () )
        {
            LOG_ERROR ( "[md5db][bucket][find][bucket_item_count=%d][index=%d]key_to_item", 
                        ( int ) bucket_item_count (), index );
            result = NULL;
            return EFAULT;
        }

        result = ( bucket_data_item_t * ) (
                                            m_data.ptr + index * sizeof ( bucket_data_item_t )
                                            );
        switch ( result->type () )
        {
            case BUCKET_DIRECT_DATA:
                LOG_DEBUG ( "[md5db][bucket][find][type=BUCKET_DIRECT_DATA][bucket_id=%u][version=%u]",
                            result->block_id.bucket_id (), result->version () );
                return 0;

            case BUCKET_CONFLICT_DATA:
                LOG_DEBUG ( "[md5db][bucket][find][type=BUCKET_CONFLICT_DATA][count=%u][unused.version=%u]",
                            result->block_id.bucket_id (), result->version () );
                return 0;

            case BUCKET_NO_DATA:
                LOG_DEBUG ( "[md5db][bucket][find][type=BUCKET_NO_DATA][block_id=%u][version=%u]",
                            result->block_id.bucket_id (), result->version () );
                return 0;

            default:
                LOG_DEBUG ( "[md5db][bucket][find][type=%d][bucket=%u][version=%u]",
                            ( int ) result->type (), result->block_id.bucket_id (), result->version () );
                return EFAULT;
        }
    }

} // namespace md5db
