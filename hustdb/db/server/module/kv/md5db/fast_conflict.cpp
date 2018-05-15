#include "fast_conflict.h"
#include "bucket.h"
#include "../../base.h"
#include "../../perf_target.h"

namespace md5db
{

#define DEFAULT_ITEM_COUNT  80000
#define EXPAND_ITEM_COUNT   80000

    typedef fast_conflict_header_t  header_t;

    fast_conflict_t::fast_conflict_t ( )
    : m_file_id ( - 1 )
    , m_file ( NULL )
    , m_conflict_count ( - 1 )
    {
        G_APPTOOL->fmap_init ( & m_data );
        m_path[ 0 ] = '\0';
    }

    fast_conflict_t::~ fast_conflict_t ( )
    {
        close ();
    }

    void fast_conflict_t::close ( )
    {
        G_APPTOOL->fmap_close ( & m_data );

        if ( m_file )
        {
            fclose ( m_file );
            m_file = NULL;
        }
    }

    bool fast_conflict_t::open ( const char * path, int file_id, int conflict_count )
    {
        if (   file_id < 0
             || file_id >= FAST_CONFLICT_FILE_COUNT
             || conflict_count < 3 )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open][conflict_count=%d]error",
                        conflict_count );
            return false;
        }

        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open]file path error" );
            return false;
        }

        char ph[ 260 ];
        strcpy ( ph, path );
        G_APPTOOL->path_to_os ( ph );
        strcpy ( m_path, ph );

        if ( ! G_APPTOOL->is_file ( path ) )
        {
            std::string header;
            std::string data;

            try
            {
                header.resize ( conflict_count * sizeof ( bucket_data_item_t ), '\0' );
                data.resize ( conflict_count * sizeof ( bucket_data_item_t ), '\0' );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][fast_conflict][open]bad_alloc" );
                return false;
            }

            header_t * hdr = ( header_t * ) & header[ 0 ];
            hdr->reset ();

            FILE * fp = fopen ( ph, "wb" );
            if ( NULL == fp )
            {
                LOG_ERROR ( "[md5db][fast_conflict][open][file=%s]fopen failed", 
                            ph );
                return false;
            }

            bool ok = false;
            do
            {
                if ( header.size () != fwrite ( header.c_str (), 1, header.size (), fp ) )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][open]write header failed" );
                    break;
                }

                int i;

                std::string t;
                try
                {
                    t.resize ( DEFAULT_ITEM_COUNT * data.size (), '\0' );
                    if ( t.size () != fwrite ( & t[ 0 ], 1, t.size (), fp ) )
                    {
                        LOG_ERROR ( "[md5db][fast_conflict][open][data_len=%d]write data failed", 
                                    ( int ) t.size () );
                        break;
                    }
                }
                catch ( ... )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][open]bad_alloc, try to write without buffer" );

                    for ( i = 0; i < DEFAULT_ITEM_COUNT; ++ i )
                    {
                        if ( data.size () != fwrite ( data.c_str (), 1, data.size (), fp ) )
                        {
                            LOG_ERROR ( "[md5db][fast_conflict][open][i=%d]write data failed", 
                                        i );
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
            LOG_ERROR ( "[md5db][fast_conflict][open][file=%s]fopen failed", 
                        ph );
            return false;
        }

        bool read_write = true;
        if ( ! G_APPTOOL->fmap_open ( & m_data, path, 0, 0, read_write ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open]fmap_open failed" );
            return false;
        }
        if ( m_data.ptr_len < ( conflict_count * sizeof ( bucket_data_item_t ) ) * ( DEFAULT_ITEM_COUNT + 1 ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open]error" );
            return false;
        }
        if ( 0 != ( m_data.ptr_len % ( conflict_count * sizeof ( bucket_data_item_t ) ) ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open]error" );
            return false;
        }

        m_file_id           = file_id;
        m_conflict_count    = conflict_count;

        return true;
    }

    bool fast_conflict_t::generate_addr (
                                          block_id_t &    addr,
                                          uint32_t        file_id,
                                          uint32_t        index
                                          )
    {
        if ( unlikely ( file_id > 0xFF || 0 == index ) )
        {
            return false;
        }

        addr.set ( ( byte_t ) file_id, index );
        return true;
    }

    bool fast_conflict_t::parse_addr (
                                       const block_id_t &  addr,
                                       uint32_t &          file_id,
                                       uint32_t &          index
                                       )
    {
        file_id = addr.bucket_id ();
        index   = addr.data_id ();
        if ( unlikely ( 0 == index ) )
        {
            file_id = 0;
            index   = 0;
            return false;
        }

        return true;
    }

#define GET_MAX()   ( uint32_t ) ( ( m_data.ptr_len - ( m_conflict_count * sizeof ( bucket_data_item_t ) ) ) / ( m_conflict_count * sizeof ( bucket_data_item_t ) ) )

    bucket_data_item_t * fast_conflict_t::alloc_item ( fast_conflict_header_t * & header )
    {
        header = ( header_t * ) m_data.ptr;
        if ( unlikely ( NULL == header ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][alloc_item]data.ptr is NULL" );
            return NULL;
        }

        if ( 0 != header->free_list_id )
        {
            do
            {
                if ( unlikely ( header->free_list_id > header->count ) )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][alloc_item][free_list=%u][count=%u][file=%s]file format bad, ignore free list",
                                header->free_list_id, header->count, m_path );
                    break;
                }

                char * d             = ( char * ) header;
                char * item          = d + ( header->free_list_id * ( m_conflict_count * sizeof ( bucket_data_item_t ) ) );
                uint32_t * free_item = ( uint32_t * ) item;

                header->free_list_id = * free_item;
                * free_item = 0;

                if ( unlikely ( header->free_list_id > header->count ) )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][alloc_item][free_list=%u][count=%u][file=%s]file format bad, ignore free list",
                                header->free_list_id, header->count, m_path );
                    header->free_list_id = 0;
                }

                return ( bucket_data_item_t * ) item;

            }
            while ( 0 );
        }

        if ( header->count > GET_MAX () )
        {
            LOG_ERROR ( "[md5db][fast_conflict][alloc_item]bad file" );
            return NULL;
        }

        uint32_t item_max_per_file = ( uint32_t ) FAST_CONFLICT_BYTES_PER_FILE / ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) );
        
        if ( header->count == GET_MAX () )
        {

            int          i;
            std::string  data;
            try
            {
                data.resize ( m_conflict_count * sizeof ( bucket_data_item_t ), '\0' );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][fast_conflict][alloc_item]bad_alloc" );
                return NULL;
            }

            int expand_count = EXPAND_ITEM_COUNT;
            if ( header->count + 1 + ( uint32_t ) expand_count > item_max_per_file )
            {
                expand_count = item_max_per_file - 1 - header->count;
                if ( expand_count <= 0 )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][alloc_item]file size limit" );
                    return NULL;
                }
            }

            std::string t;
            try
            {
                t.resize ( expand_count * data.size (), '\0' );
                if ( t.size () != fwrite ( & t[ 0 ], 1, t.size (), m_file ) )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][alloc_item][data_len=%d]write data failed", 
                                ( int ) t.size () );
                    return NULL;
                }
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][fast_conflict][alloc_item]bad_alloc, try to write without buffer" );

                for ( i = 0; i < expand_count; ++ i )
                {
                    if ( data.size () != fwrite ( data.c_str (), 1, data.size (), m_file ) )
                    {
                        LOG_ERROR ( "[md5db][fast_conflict][alloc_item][i=%d]expand data failed", 
                                    i );
                        break;
                    }
                }
                if ( i < expand_count )
                {
                    LOG_ERROR ( "[md5db][fast_conflict][alloc_item][i=%d][expand_count=%d]expand failed", 
                                i, expand_count );
                    return NULL;
                }
            }

            fflush ( m_file );

            bool read_write = true;
            fmap_t m;
            if ( ! G_APPTOOL->fmap_open ( & m, m_path, 0, 0, read_write ) )
            {
                LOG_ERROR ( "[md5db][fast_conflict][alloc_item]fmap_open failed" );
                return NULL;
            }

            fmap_t old_m;
            memcpy ( & old_m, & m_data, sizeof ( fmap_t ) );
            memcpy ( & m_data, & m, sizeof ( fmap_t ) );
            G_APPTOOL->fmap_close ( & old_m );

            header = ( header_t * ) m_data.ptr;

            if ( header->count >= GET_MAX () )
            {
                LOG_ERROR ( "[md5db][fast_conflict][alloc_item]expand algorithm error" );
                return NULL;
            }
        }

        header->count ++;

        char * d = ( char * ) header;
        char * item = d + ( header->count * ( m_conflict_count * sizeof ( bucket_data_item_t ) ) );
        
        return ( bucket_data_item_t * ) item;
    }

    bool fast_conflict_t::write (
                                  const bucket_data_item_t &  bucket_item_1,
                                  const bucket_data_item_t &  bucket_item_2,
                                  block_id_t &                addr
                                  )
    {
        addr.reset ();

        header_t * header;
        bucket_data_item_t * item = alloc_item ( header );
        if ( NULL == item )
        {
            LOG_ERROR ( "[md5db][fast_conflict][write]alloc_item failed" );
            return false;
        }

        uint32_t offset;
        uint32_t item_max_per_file = ( uint32_t ) FAST_CONFLICT_BYTES_PER_FILE / ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) );

        offset = ( uint32_t ) ( ( char * ) item - ( char * ) header );
        if ( offset < ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) ) || 
             offset + ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) ) > FAST_CONFLICT_BYTES_PER_FILE )
        {
            LOG_ERROR ( "[md5db][fast_conflict][write][offset=%u][out of %u]",
                        offset, FAST_CONFLICT_BYTES_PER_FILE );
            return false;
        }

        memcpy ( & item[ 0 ], & bucket_item_1, sizeof ( bucket_data_item_t ) );
        memcpy ( & item[ 1 ], & bucket_item_2, sizeof ( bucket_data_item_t ) );
        memset ( & item[ 2 ], 0, sizeof ( bucket_data_item_t ) * ( m_conflict_count - 2 ) );

        uint32_t index = offset / ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) );
        if ( ! generate_addr ( addr, m_file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][write][offset=%u][file=%u]generate_addr failed",
                        offset, m_file_id );
            return false;
        }

        return true;
    }

    bool fast_conflict_t::get (
                                const block_id_t &          addr,
                                bucket_data_item_t * &      result,
                                size_t *                    result_count
                                )
    {
        result          = NULL;
        * result_count  = 0;

        uint32_t file_id;
        uint32_t index;
        if ( ! parse_addr ( addr, file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][get][addr=%u]invalid addr",
                        addr.data_id () );
            return false;
        }

        if ( file_id != m_file_id || 
             0 == index || 
             index >= FAST_CONFLICT_BYTES_PER_FILE / ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][get][addr=%u][file=%u][index=%u]invalid data",
                        addr.data_id (), file_id, index );
            return false;
        }

        header_t * header = ( header_t * ) m_data.ptr;
        if ( 0 == header->count || index > header->count )
        {
            LOG_ERROR ( "[md5db][fast_conflict][get][addr=%u][index=%d][count=%d]invalid data",
                        addr.data_id (), index, header->count );
            return false;
        }

        const char * p  = ( const char * ) m_data.ptr + index * ( m_conflict_count * sizeof ( bucket_data_item_t ) );
        result          = ( bucket_data_item_t * ) p;
        * result_count  = m_conflict_count;

        return true;
    }

    bool fast_conflict_t::del (
                                const block_id_t &  addr
                                )
    {
        uint32_t file_id;
        uint32_t index;
        if ( ! parse_addr ( addr, file_id, index ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][del][addr=%u]invalid addr",
                        addr.data_id () );
            return false;
        }
        if (   file_id != m_file_id
             || 0 == index
             || index >= FAST_CONFLICT_BYTES_PER_FILE / ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][del][addr=%u][file=%u][index=%u]invalid data",
                        addr.data_id (), file_id, index );
            return false;
        }

        header_t * header = ( header_t * ) m_data.ptr;
        if ( 0 == header->count || index > header->count )
        {
            LOG_ERROR ( "[md5db][fast_conflict][del][addr=%u][index=%u][count=%d]invalid data",
                        addr.data_id (), index, header->count );
            return false;
        }

        uint32_t * p = ( uint32_t * ) ( m_data.ptr + index * ( m_conflict_count * sizeof ( bucket_data_item_t ) ) );
        memset ( p, 0, ( m_conflict_count * sizeof ( bucket_data_item_t ) ) );

        * p = header->free_list_id;
        header->free_list_id = index;

        return true;
    }

    void fast_conflict_t::info ( std::stringstream & ss )
    {
        size_t capacity     = 0;
        size_t count        = 0;

        header_t * header   = ( header_t * ) m_data.ptr;
        if ( header )
        {
            count           = header->count;
            capacity        = GET_MAX ();
        }

        size_t item_max_per_file = ( uint32_t ) FAST_CONFLICT_BYTES_PER_FILE / ( uint32_t ) ( m_conflict_count * sizeof ( bucket_data_item_t ) );

        write_single_count ( ss, m_file_id, "max",       item_max_per_file );
        write_single_count ( ss, m_file_id, "capacity",  capacity );
        write_single_count ( ss, m_file_id, "count",     count, true );
    }

} // namespace md5db
