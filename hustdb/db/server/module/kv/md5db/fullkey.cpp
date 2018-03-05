#include "fullkey.h"
#include "bucket.h"
#include "../../base.h"
#include "../../perf_target.h"

namespace md5db
{

#define FULLKEY_LEN         ( 13 + sizeof( uint16_t ) + 2 )
#define DEFAULT_ITEM_COUNT  400000
#define EXPAND_ITEM_COUNT   400000

#pragma pack( push, 1 )

    struct fullkey_header_t
    {
        uint32_t    file_version;
        uint32_t    count;
        uint32_t    free_list_id;
        char data[ 5 ];
    } ;

    struct fullkey_data_t
    {
        char        md5_suffix[ 13 ];
        uint16_t    user_key_len_mod_0xff;
        uint16_t    user_key_crc32_mod_0xff;
    } ;

#pragma pack( pop )

    typedef fullkey_data_t      data_t;
    typedef fullkey_header_t    header_t;

    fullkey_t::fullkey_t ( )
    : m_file_id ( - 1 )
    , m_file ( NULL )
    {
        G_APPTOOL->fmap_init ( & m_data );
        m_path[ 0 ] = '\0';
    }

    fullkey_t::~ fullkey_t ( )
    {
        close ();
    }

    void fullkey_t::close ( )
    {
        G_APPTOOL->fmap_close ( & m_data );

        if ( m_file )
        {
            fclose ( m_file );
            m_file = NULL;
        }
    }

    bool fullkey_t::open ( const char * path, int file_id )
    {
        if (   file_id < 0
             || file_id >= FULLKEY_FILE_COUNT
             || FULLKEY_LEN != sizeof ( data_t )
             || sizeof ( header_t ) != sizeof ( data_t ) )
        {
            LOG_ERROR ( "[md5db][fullkey][header_t=%d][data_t=%d][FULLKEY_LEN=%d]error",
                       ( int ) sizeof ( header_t ), ( int ) sizeof ( data_t ), FULLKEY_LEN );
            return false;
        }

        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][fullkey]error" );
            return false;
        }

        char ph[ 260 ];
        strcpy ( ph, path );
        G_APPTOOL->path_to_os ( ph );
        strcpy ( m_path, ph );

        if ( ! G_APPTOOL->is_file ( path ) )
        {
            header_t    header;
            data_t      data;

            memset ( & data, 0, sizeof ( data ) );
            memset ( & header, 0, sizeof ( header ) );

            header.count        = 0;
            header.file_version = 0;
            header.free_list_id = 0;

            FILE * fp = fopen ( ph, "wb" );
            if ( NULL == fp )
            {
                LOG_ERROR ( "[md5db][fullkey]fopen %s failed", ph );
                return false;
            }

            bool ok = false;
            do
            {
                if ( sizeof ( header ) != fwrite ( & header, 1, sizeof ( header ), fp ) )
                {
                    LOG_ERROR ( "[md5db][fullkey]write header failed" );
                    break;
                }

                int i;

                std::string t;
                try
                {
                    t.resize ( DEFAULT_ITEM_COUNT * sizeof ( data_t ), '\0' );
                    if ( t.size () != fwrite ( & t[ 0 ], 1, t.size (), fp ) )
                    {
                        LOG_ERROR ( "[md5db][fullkey]write data %d bytes failed", ( int ) t.size () );
                        break;
                    }
                }
                catch ( ... )
                {
                    LOG_ERROR ( "[md5db][fullkey]maybe bad_alloc, try to write without buffer" );
                    for ( i = 0; i < DEFAULT_ITEM_COUNT; ++ i )
                    {
                        if ( sizeof ( data ) != fwrite ( & data, 1, sizeof ( data ), fp ) )
                        {
                            LOG_ERROR ( "[md5db][fullkey]write data #%d failed", i );
                            break;
                        }
                    }
                    if ( i < DEFAULT_ITEM_COUNT )
                    {
                        break;
                    }
                }

                ok = true;
            }
            while ( 0 );

            fclose ( fp );

            if ( ! ok )
            {
                remove ( ph );
                return false;
            }
        }

        m_file = fopen ( ph, "ab" );
        if ( NULL == m_file )
        {
            LOG_ERROR ( "[md5db][fullkey]fopen %s failed", ph );
            return false;
        }

        bool read_write = true;
        if ( ! G_APPTOOL->fmap_open ( & m_data, path, 0, 0, read_write ) )
        {
            LOG_ERROR ( "[md5db][fullkey]fmap_open failed" );
            return false;
        }
        if ( m_data.ptr_len < sizeof ( header_t ) + sizeof ( data_t ) * DEFAULT_ITEM_COUNT )
        {
            LOG_ERROR ( "[md5db][fullkey]error" );
            return false;
        }
        if ( 0 != ( m_data.ptr_len % sizeof ( data_t ) ) )
        {
            LOG_ERROR ( "[md5db][fullkey]error" );
            return false;
        }

        m_file_id = file_id;
        return true;
    }

    bool fullkey_t::generate_block_id (
                                        block_id_t &    block_id,
                                        uint32_t        file_id,
                                        uint32_t        index
                                        )
    {
        if ( unlikely ( file_id > 0xFF || 0 == index ) )
        {
            return false;
        }

        block_id.set ( ( byte_t ) file_id, index );
        return true;
    }

    bool fullkey_t::parse_block_id (
                                     const block_id_t &  block_id,
                                     uint32_t &          file_id,
                                     uint32_t &          index
                                     )
    {
        file_id = block_id.bucket_id ();
        index   = block_id.data_id ();
        if ( unlikely ( 0 == index ) )
        {
            file_id = 0;
            index   = 0;
            return false;
        }

        return true;
    }

#define GET_MAX()   (uint32_t)( ( m_data.ptr_len - sizeof ( header_t ) ) / sizeof( data_t ) )

    fullkey_data_t * fullkey_t::alloc_item ( fullkey_header_t * & header )
    {
        header = ( header_t * ) m_data.ptr;
        if ( unlikely ( NULL == header ) )
        {
            LOG_ERROR ( "[md5db][fullkey]m_data.ptr is NULL" );
            return NULL;
        }

        if ( 0 != header->free_list_id )
        {
            do
            {
                if ( unlikely ( header->free_list_id > header->count ) )
                {
                    LOG_ERROR ( "[md5db][fullkey][free_list=%u][count=%u] "
                               "file format bad !!!!!!!!!!!!!!!!!!!!, ignore free list. path=%s",
                               header->free_list_id, header->count, m_path );
                    break;
                }

                data_t * d = ( data_t * ) header;
                data_t * item = & d[ header->free_list_id ];
                uint32_t * free_item = ( uint32_t * ) item;
                header->free_list_id = * free_item;
                * free_item = 0;

                if ( unlikely ( header->free_list_id > header->count ) )
                {
                    LOG_ERROR ( "[md5db][fullkey][free_list=%u][count=%u] file format bad !!!!!!!!!!!!!!!!!!!!, ignore free list. path=%s",
                               header->free_list_id, header->count, m_path );
                    header->free_list_id = 0;
                }

                return item;

            }
            while ( 0 );
        }

        if ( header->count > GET_MAX () )
        {
            LOG_ERROR ( "[md5db][fullkey]bad file" );
            return NULL;
        }

        uint32_t item_max_per_file = ( uint32_t ) FILEKEY_BYTES_PER_FILE / ( uint32_t )sizeof ( data_t );
        if ( header->count == GET_MAX () )
        {

            int i;

            data_t      data;
            memset ( & data, 0, sizeof ( data ) );

            int expand_count = EXPAND_ITEM_COUNT;
            if ( header->count + 1 + ( uint32_t ) expand_count > item_max_per_file )
            {
                expand_count = item_max_per_file - 1 - header->count;
                if ( expand_count <= 0 )
                {
                    LOG_ERROR ( "[md5db][fullkey]FILE SIZE LIMIT!" );
                    return NULL;
                }
            }

            std::string t;
            try
            {
                t.resize ( expand_count * sizeof ( data_t ), '\0' );
                if ( t.size () != fwrite ( & t[ 0 ], 1, t.size (), m_file ) )
                {
                    LOG_ERROR ( "[md5db][fullkey]write data %d bytes failed", ( int ) t.size () );
                    return NULL;
                }
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][fullkey]maybe bad_alloc, try to write without buffer" );

                for ( i = 0; i < expand_count; ++ i )
                {
                    if ( sizeof ( data ) != fwrite ( & data, 1, sizeof ( data ), m_file ) )
                    {
                        LOG_ERROR ( "[md5db][fullkey]expand data #%d failed", i );
                        break;
                    }
                }
                if ( i < expand_count )
                {
                    LOG_ERROR ( "[md5db][fullkey]expand failed: %d, %d", i, expand_count );
                    return NULL;
                }
            }

            fflush ( m_file );

            bool read_write = true;
            fmap_t      m;
            if ( ! G_APPTOOL->fmap_open ( & m, m_path, 0, 0, read_write ) )
            {
                LOG_ERROR ( "[md5db][fullkey]fmap_open failed" );
                return NULL;
            }

            fmap_t      old_m;
            memcpy ( & old_m, & m_data, sizeof ( fmap_t ) );
            memcpy ( & m_data, & m, sizeof ( fmap_t ) );
            G_APPTOOL->fmap_close ( & old_m );

            header = ( header_t * ) m_data.ptr;

            if ( header->count >= GET_MAX () )
            {
                LOG_ERROR ( "[md5db][fullkey]expand algorithm error" );
                return NULL;
            }
        }

        ++ header->count;

        data_t * d = ( data_t * ) header;
        data_t * item = & d[ header->count ];
        return item;
    }

    void fullkey_t::fill_user_key (
                                    const char *        user_key,
                                    unsigned int        user_key_len,
                                    fullkey_data_t *    item
                                    )
    {
        item->user_key_len_mod_0xff     = ( uint16_t ) ( user_key_len % 0xFF );
        item->user_key_crc32_mod_0xff   = ( uint16_t ) ( G_APPTOOL->fast_crc32 ( user_key, user_key_len ) );
    }

    bool fullkey_t::compare (
                              const block_id_t &  block_id,
                              const char *        inner_key,
                              unsigned int        inner_key_len,
                              const char *        user_key,
                              unsigned int        user_key_len
                              )
    {
        uint32_t file_id;
        uint32_t index;
        if ( ! parse_block_id ( block_id, file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u",
                       block_id.data_id () );
            return false;
        }
        if (   file_id != m_file_id
             || 0 == index
             || index >= FILEKEY_BYTES_PER_FILE / ( uint32_t )sizeof ( data_t ) )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u, file=%u, index=%u",
                       block_id.data_id (), file_id, index );
            return false;
        }

        //scope_rlock_t lock( m_lock );

        header_t * header = ( header_t * ) m_data.ptr;
        if ( 0 == header->count || index > header->count )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u, index=%d, count=%d",
                       block_id.data_id (), index, header->count );
            return false;
        }

        const data_t * p = ( const data_t * ) ( m_data.ptr + index * sizeof ( data_t ) );

        if ( 0 != memcmp ( p->md5_suffix, & inner_key[ 3 ], 13 ) )
        {
            return false;
        }

        if ( user_key && user_key_len > 0 )
        {
            fullkey_data_t verify;
            fill_user_key ( user_key, user_key_len, & verify );

            if ( verify.user_key_len_mod_0xff != p->user_key_len_mod_0xff )
            {
                return false;
            }
            if ( verify.user_key_crc32_mod_0xff != p->user_key_crc32_mod_0xff )
            {
                return false;
            }
        }

        return true;
    }

    bool fullkey_t::write (
                            const char *    inner_key,
                            unsigned int    inner_key_len,
                            const char *    user_key,
                            unsigned int    user_key_len,
                            block_id_t &    block_id
                            )
    {
        block_id.reset ();

        if ( NULL == inner_key || 16 != inner_key_len )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid key" );
            return false;
        }

        //scope_wlock_t lock( m_lock );

        header_t * header;
        data_t * item = alloc_item ( header );
        if ( NULL == item )
        {
            LOG_ERROR ( "[md5db][fullkey]alloc_item failed" );
            return false;
        }

        uint32_t        offset;
        uint32_t item_max_per_file = ( uint32_t ) FILEKEY_BYTES_PER_FILE / ( uint32_t )sizeof ( data_t );

        offset = ( uint32_t ) ( ( char * ) item - ( char * ) header );
        if ( offset < ( uint32_t )sizeof (data_t ) || offset + ( uint32_t )sizeof (data_t ) > FILEKEY_BYTES_PER_FILE )
        {
            LOG_ERROR ( "[md5db][fullkey]offset=%u, out of %u",
                       offset, FILEKEY_BYTES_PER_FILE );
            return false;
        }

        memcpy ( item->md5_suffix, & inner_key[ 3 ], 13 );
        fill_user_key ( user_key, user_key_len, item );

        uint32_t index = offset / ( uint32_t )sizeof ( data_t );
        if ( ! generate_block_id ( block_id, m_file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fullkey]ofset=%u][file=%u]generate_block_id failed",
                       offset, m_file_id );
            return false;
        }

        return true;
    }

    bool fullkey_t::get (
                          const block_id_t &  block_id,
                          char *              resp_since_4
                          )
    {
        uint32_t file_id;
        uint32_t index;
        if ( ! parse_block_id ( block_id, file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u",
                       block_id.data_id () );
            return false;
        }
        if (   file_id != m_file_id
             || 0 == index
             || index >= FILEKEY_BYTES_PER_FILE / ( uint32_t )sizeof ( data_t ) )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u, file=%u, index=%u",
                       block_id.data_id (), file_id, index );
            return false;
        }

        //scope_rlock_t lock( m_lock );

        header_t * header = ( header_t * ) m_data.ptr;
        if ( 0 == header->count || index > header->count )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u, index=%d, count=%d",
                       block_id.data_id (), index, header->count );
            return false;
        }

        const char * p = ( const char * ) m_data.ptr + index * sizeof ( data_t );
        memcpy ( resp_since_4, p, sizeof (data_t ) );
        return true;
    }

    bool fullkey_t::del (
                          const block_id_t &  block_id
                          )
    {
        uint32_t file_id;
        uint32_t index;
        if ( ! parse_block_id ( block_id, file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u",
                       block_id.data_id () );
            return false;
        }
        if (   file_id != m_file_id
             || 0 == index
             || index >= FILEKEY_BYTES_PER_FILE / ( uint32_t )sizeof ( data_t ) )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u, file=%u, index=%u",
                       block_id.data_id (), file_id, index );
            return false;
        }

        //scope_rlock_t lock( m_lock );

        header_t * header = ( header_t * ) m_data.ptr;
        if ( 0 == header->count || index > header->count )
        {
            LOG_ERROR ( "[md5db][fullkey]invalid block_id=%u, index=%u, count=%d",
                       block_id.data_id (), index, header->count );
            return false;
        }

        uint32_t * p = ( uint32_t * ) ( m_data.ptr + index * sizeof ( data_t ) );
        memset ( p, 0, sizeof ( data_t ) );

        * p = header->free_list_id;
        header->free_list_id = index;

        return true;
    }

    void fullkey_t::info (
                           std::stringstream & ss
                           )
    {
        size_t capacity     = 0;
        size_t count        = 0;

        header_t * header   = ( header_t * ) m_data.ptr;
        if ( header )
        {
            count           = header->count;
            capacity        = GET_MAX ();
        }

        size_t item_max_per_file = ( uint32_t ) FILEKEY_BYTES_PER_FILE / ( uint32_t )sizeof ( data_t );

        write_single_count ( ss, m_file_id, "max",       item_max_per_file );
        write_single_count ( ss, m_file_id, "capacity",  capacity );
        write_single_count ( ss, m_file_id, "count",     count, true );
    }

} // namespace md5db
