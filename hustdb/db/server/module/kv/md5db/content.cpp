#include <math.h>

#include "content.h"
#include "../../base.h"
#include "../../perf_target.h"

namespace md5db
{

    static inline
    long get_file_size ( const char * path )
    {
        FILE * h;

        h = fopen ( path, "rb" );
        if ( NULL == h )
        {
            LOG_ERROR ( "fopen( %s ) failed", path );
            return 0;
        }

        long r = 0;

        do
        {

            if ( 0 != fseek ( h, 0, SEEK_END ) )
            {
                LOG_ERROR ( "fseek( %s ) failed", path );
                break;
            }

            r = ftell ( h );
            if ( - 1L == r )
            {
                LOG_ERROR ( "ftell( %s ) failed", path );
                r = 0;
                break;
            }

        }
        while ( 0 );

        fclose ( h );

        return r;
    }

    content_t::content_t ( )
    : m_file_id ( - 1 )
    , m_file_size ( 0 )
    , m_free_size ( 0 )
    , m_cache_free_size ( 0 )
    , m_parse_time ( 0.0 )
    , m_cache_parse_time ( 0.0 )
    , m_merge_valve ( 0 )
    , m_cache_merge_valve ( 0 )
    , m_cache_file_length ( 0 )
    , m_cache_file_bitmap_length ( 0 )
    , m_cache_file_bitmap_bit_length ( 0 )
    , m_data_file ( NULL )
    , m_lock ( )
    {
        G_APPTOOL->fmap_init ( & m_bitmap );
        G_APPTOOL->fmap_init ( & m_cache_bitmap );
        G_APPTOOL->fmap_init ( & m_cache_file );
        memset ( m_tail, 0, PAGE );
    }

    content_t::~ content_t ( )
    {
        close ();
    }

    void content_t::close ( )
    {
        G_APPTOOL->fmap_close ( & m_bitmap );
        G_APPTOOL->fmap_close ( & m_cache_bitmap );
        G_APPTOOL->fmap_close ( & m_cache_file );

        if ( m_data_file )
        {
            fclose ( m_data_file );
            m_data_file = NULL;
        }
    }

    bool content_t::open ( const char * path, int file_id, int cache )
    {
        if ( file_id < 0 )
        {
            LOG_ERROR ( "[md5db][content_db]invalid file_id" );
            return false;
        }

        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][content_db]error" );
            return false;
        }

        m_cache_file_length = cache * 1024 * 1024;
        m_cache_file_bitmap_bit_length = cache * 1024;
        m_cache_file_bitmap_length = cache * 128;

        char ph[ 260 ] = { };
        strcpy ( ph, path );
        strcat ( ph, ".content" );
        if ( ! G_APPTOOL->is_file ( ph ) )
        {
            FILE * fp = fopen ( ph, "wb" );
            if ( ! fp )
            {
                LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
                return false;
            }

            bool ok = true;
            char buf[ PAGE ];
            memset ( buf, 0, sizeof ( buf ) );
            if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
            {
                ok = false;
            }

            fclose ( fp );
            if ( ! ok )
            {
                remove ( ph );
                return false;
            }
        }

        m_data_file = fopen ( ph, "rb+" );
        if ( ! m_data_file )
        {
            LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
            return false;
        }

        int r = fseek ( m_data_file, 0, SEEK_END );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][content_db]seek end failed" );
            return false;
        }

        m_file_size = ( uint32_t ) ftell ( m_data_file );
        if ( m_file_size < 0 )
        {
            LOG_ERROR ( "[md5db][content_db]ftell failed" );
            return false;
        }

        uint32_t tail = m_file_size % PAGE;
        if ( tail > 0 )
        {
            if ( tail != fwrite ( m_tail, 1, tail, m_data_file ) )
            {
                LOG_ERROR ( "[md5db][content_db]fwrite failed" );
                return false;
            }

            m_file_size += tail;
            fflush ( m_data_file );
        }

        memset ( ph, 0, sizeof ( ph ) );
        strcpy ( ph, path );
        strcat ( ph, ".bitmap" );
        if ( ! G_APPTOOL->is_file ( ph ) )
        {
            FILE * fp = fopen ( ph, "wb" );
            if ( ! fp )
            {
                LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
                return false;
            }

            bool ok = true;
            char buf[ BITMAP ];
            memset ( buf, 0, sizeof ( buf ) );
            if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
            {
                ok = false;
            }

            fclose ( fp );
            if ( ! ok )
            {
                remove ( ph );
                return false;
            }
        }

        if ( ! G_APPTOOL->fmap_open ( & m_bitmap, ph, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db]fmap_open failed" );
            return false;
        }

        if ( m_bitmap.ptr_len != BITMAP )
        {
            LOG_ERROR ( "[md5db][content_db]error" );
            return false;
        }

        memset ( ph, 0, sizeof ( ph ) );
        strcpy ( ph, path );
        strcat ( ph, ".cache.bitmap" );
        if ( ! G_APPTOOL->is_file ( ph ) )
        {
            FILE * fp = fopen ( ph, "wb" );
            if ( ! fp )
            {
                LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
                return false;
            }

            bool ok = true;
            char buf[ PAGE ];
            memset ( buf, 0, sizeof ( buf ) );
            uint32_t cycle = m_cache_file_bitmap_length / PAGE;

            for ( uint32_t i = 0; i < cycle; i ++ )
            {
                if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
                {
                    ok = false;
                    break;
                }
            }

            fclose ( fp );
            if ( ! ok )
            {
                remove ( ph );
                return false;
            }
        }

        if ( ! G_APPTOOL->fmap_open ( & m_cache_bitmap, ph, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db]fmap_open failed" );
            return false;
        }

        if ( m_cache_bitmap.ptr_len != m_cache_file_bitmap_length )
        {
            LOG_ERROR ( "[md5db][content_db]error" );
            return false;
        }

        memset ( ph, 0, sizeof ( ph ) );
        strcpy ( ph, path );
        strcat ( ph, ".cache.file" );
        if ( ! G_APPTOOL->is_file ( ph ) )
        {
            FILE * fp = fopen ( ph, "wb" );
            if ( ! fp )
            {
                LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
                return false;
            }

            bool ok = true;
            char buf[ PAGE512 ];
            memset ( buf, 0, sizeof ( buf ) );
            uint32_t cycle = m_cache_file_length / PAGE512;

            for ( uint32_t i = 0; i < cycle; i ++ )
            {
                if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
                {
                    ok = false;
                    break;
                }
            }

            fclose ( fp );
            if ( ! ok )
            {
                remove ( ph );
                return false;
            }
        }

        if ( ! G_APPTOOL->fmap_open ( & m_cache_file, ph, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db]fmap_open failed" );
            return false;
        }

        if ( m_cache_file.ptr_len != m_cache_file_length )
        {
            LOG_ERROR ( "[md5db][content_db]error" );
            return false;
        }

        parse_free_list ();
        parse_cache_free_list ();

        m_file_id = file_id;

        LOG_INFO ( "[md5db][content_db]create success, path: %s, file_id: %d, cache: %d", path, file_id, cache );

        return true;
    }

    void content_t::parse_free_list ( )
    {
        m_merge_valve = 0;
        m_free_size = 0;
        m_free_list.clear ();

        BITSET1 ( 0 )

        uint32_t i = 0;
        uint32_t offset = BITMAP_MAX;

        while ( i < BITMAP_BIT )
        {
            if ( BITGET ( i ) )
            {
                if ( offset != BITMAP_MAX )
                {
                    m_free_size += i - offset;
                    m_free_list.insert ( std::pair < uint32_t, uint32_t >( i - offset, offset ) );
                }

                offset = BITMAP_MAX;
            }
            else if ( offset == BITMAP_MAX )
            {
                offset = i;
            }

            i ++;
        }

        if ( offset != BITMAP_MAX )
        {
            m_free_size += i - offset;
            m_free_list.insert ( std::pair < uint32_t, uint32_t >( i - offset, offset ) );
        }
    }

    void content_t::parse_cache_free_list ( )
    {
        m_cache_merge_valve = 0;
        m_cache_free_size = 0;
        m_cache_free_list.clear ();

        CACHE_BITSET1 ( 0 )

        uint32_t i = 0;
        uint32_t offset = BITMAP_MAX;

        while ( i < m_cache_file_bitmap_bit_length )
        {
            if ( CACHE_BITGET ( i ) )
            {
                if ( offset != BITMAP_MAX )
                {
                    m_cache_free_size += i - offset;
                    m_cache_free_list.insert ( std::pair < uint32_t, uint32_t >( i - offset, offset ) );
                }

                offset = BITMAP_MAX;
            }
            else if ( offset == BITMAP_MAX )
            {
                offset = i;
            }

            i ++;
        }

        if ( offset != BITMAP_MAX )
        {
            m_cache_free_size += i - offset;
            m_cache_free_list.insert ( std::pair < uint32_t, uint32_t >( i - offset, offset ) );
        }
    }

    bool content_t::find_free_block ( uint32_t block, uint32_t & offset )
    {
        if ( unlikely ( m_free_size < block ) )
        {
            return false;
        }

        if ( m_merge_valve >= block * 10 )
        {
            parse_free_list ( );
        }

        free_list_t::iterator it = m_free_list.upper_bound ( block - 1 );
        if ( unlikely ( it == m_free_list.end () ) )
        {
            if ( m_merge_valve >= block )
            {
                struct timeval tp = { 0 };
                gettimeofday (&tp, NULL);
                double t = ( double ) ( tp.tv_sec + tp.tv_usec / MICRO_IN_SEC );
                if ( t - m_parse_time > 300 )
                {
                    m_parse_time = t;

                    parse_free_list ( );

                    it = m_free_list.upper_bound ( block - 1 );
                    if ( unlikely ( it == m_free_list.end () ) )
                    {
                        return false;
                    }
                }
            }
            else
            {
                return false;
            }
        }

        offset = it->second;

        uint32_t new_block = it->first - block;
        uint32_t new_offset = it->second + block;

        m_free_list.erase ( it );

        if ( new_block > 0 )
        {
            m_free_list.insert (std::pair<uint32_t, uint32_t>( new_block, new_offset ));
        }

        return true;
    }

    bool content_t::find_cache_free_block ( uint32_t block, uint32_t & offset )
    {
        if ( unlikely ( m_cache_free_size < block ) )
        {
            return false;
        }

        free_list_t::iterator it = m_cache_free_list.upper_bound ( block - 1 );
        if ( unlikely ( it == m_cache_free_list.end () ) )
        {
            if ( m_cache_merge_valve >= block )
            {
                struct timeval tp = { 0 };
                gettimeofday (&tp, NULL);
                double t = ( double ) ( tp.tv_sec + tp.tv_usec / MICRO_IN_SEC );
                if ( t - m_cache_parse_time > 300 )
                {
                    m_cache_parse_time = t;

                    parse_cache_free_list ( );

                    it = m_cache_free_list.upper_bound ( block - 1 );
                    if ( unlikely ( it == m_cache_free_list.end () ) )
                    {
                        return false;
                    }
                }
            }
            else
            {
                return false;
            }
        }

        offset = it->second;

        uint32_t new_block = it->first - block;
        uint32_t new_offset = it->second + block;

        m_cache_free_list.erase ( it );

        if ( new_block > 0 )
        {
            m_cache_free_list.insert (std::pair<uint32_t, uint32_t>( new_block, new_offset ));
        }

        return true;
    }

    bool content_t::merge_free_block ( uint32_t block, uint32_t offset )
    {
        m_merge_valve += block;
        m_free_size += block;
        m_free_list.insert (std::pair<uint32_t, uint32_t>( block, offset ));

        for ( uint32_t i = offset, j = 0; j < block; i ++, j ++ )
        {
            BITSET0 ( i )
        }

        return true;
    }

    bool content_t::merge_cache_free_block ( uint32_t block, uint32_t offset )
    {
        m_cache_merge_valve += block;
        m_cache_free_size += block;
        m_cache_free_list.insert (std::pair<uint32_t, uint32_t>( block, offset ));

        for ( uint32_t i = offset, j = 0; j < block; i ++, j ++ )
        {
            CACHE_BITSET0 ( i )
        }

        return true;
    }

    bool content_t::write (
                            const char *        data,
                            uint32_t            data_len,
                            uint32_t &          offset
                            )
    {
        uint32_t block = data_len / PAGE;
        uint32_t tail = data_len % PAGE;
        if ( tail > 0 )
        {
            tail = PAGE - tail;
            block ++;
        }

        scope_wlock_t lock ( m_lock );

        return write_inner ( data, data_len, offset, block, tail );
    }

    bool content_t::write_inner (
                                  const char * data,
                                  uint32_t data_len,
                                  uint32_t & offset,
                                  uint32_t block,
                                  uint32_t tail
                                  )
    {
        data_index_t index;
        uint32_t offset_blk;

        index.id = 0;
        bool r = find_cache_free_block ( block, offset_blk );
        if ( ! r )
        {
            index.id = 1;
            r = find_free_block ( block, offset_blk );
            if ( ! r )
            {
                LOG_ERROR ( "[md5db][content_db]find_free_block failed, data_len: %d, block: %d, tail: %d",
                           data_len, block, tail );
                return false;
            }
        }

        index.offset = offset_blk * PAGE;
        fast_memcpy ( & offset, & index, 4 );

        if ( ! index.id )
        {
            fast_memcpy ( m_cache_file.ptr + index.offset, data, data_len );

            for ( uint32_t i = offset_blk, j = 0; j < block; i ++, j ++ )
            {
                CACHE_BITSET1 ( i )
            }

            m_cache_free_size -= block;

            return true;
        }

        if ( index.offset >= m_file_size )
        {
            int r = fseek ( m_data_file, 0, SEEK_END );
            if ( unlikely ( 0 != r ) )
            {
                LOG_ERROR ( "[md5db][content_db]seek end failed" );
                offset = 0;
                return false;
            }

            if ( data_len != fwrite ( data, 1, data_len, m_data_file ) )
            {
                LOG_ERROR ( "[md5db][content_db]fwrite data failed" );
                offset = 0;
                return false;
            }

            fflush ( m_data_file );

            r = fseek ( m_data_file, 0, SEEK_END );
            if ( unlikely ( 0 != r ) )
            {
                LOG_ERROR ( "[md5db][content_db]seek end failed" );
                offset = 0;
                return false;
            }

            if ( tail > 0 && tail != fwrite ( m_tail, 1, tail, m_data_file ) )
            {
                LOG_ERROR ( "[md5db][content_db]fwrite tail failed" );
                offset = 0;
                return false;
            }

            fflush ( m_data_file );

            m_file_size += data_len + tail;
        }
        else
        {
            int fd = fileno ( m_data_file );
            ssize_t w = pwrite ( fd, data, data_len, index.offset );
            if ( data_len != ( size_t ) w )
            {
                LOG_ERROR ( "[md5db][content_db]pwrite failed" );
                offset = 0;
                return false;
            }

            if ( index.offset + data_len + tail > m_file_size )
            {
                int r = fseek ( m_data_file, 0, SEEK_END );
                if ( 0 != r )
                {
                    LOG_ERROR ( "[md5db][content_db]seek end failed" );
                    offset = 0;
                    return false;
                }

                if ( tail > 0 && tail != fwrite ( m_tail, 1, tail, m_data_file ) )
                {
                    LOG_ERROR ( "[md5db][content_db]fwrite tail failed" );
                    offset = 0;
                    return false;
                }

                m_file_size = index.offset + data_len + tail;
            }

            fflush ( m_data_file );
        }

        m_free_size -= block;

        for ( uint32_t i = offset_blk, j = 0; j < block; i ++, j ++ )
        {
            BITSET1 ( i )
        }

        return true;
    }

    bool content_t::update (
                             uint32_t &          offset,
                             uint32_t            old_data_len,
                             const char *        data,
                             uint32_t            data_len
                             )
    {
        data_index_t index;
        fast_memcpy ( & index, & offset, 4 );

        uint32_t old_block = old_data_len / PAGE;
        if ( old_data_len % PAGE > 0 )
        {
            old_block ++;
        }

        uint32_t block = data_len / PAGE;
        uint32_t tail = data_len % PAGE;
        if ( tail > 0 )
        {
            tail = PAGE - tail;
            block ++;
        }
        
        scope_wlock_t lock ( m_lock );

        if ( old_block == block )
        {
            if ( ! index.id )
            {
                fast_memcpy ( m_cache_file.ptr + index.offset, data, data_len );
            }
            else
            {
                int fd = fileno ( m_data_file );
                ssize_t w = pwrite ( fd, data, data_len, index.offset );
                if ( data_len != ( size_t ) w )
                {
                    LOG_ERROR ( "[md5db][content_db]pwrite failed offset:%d, data_len:%d", index.offset, data_len );
                    return false;
                }
            }

            return true;
        }

        if ( unlikely ( index.offset % PAGE > 0 ) )
        {
            LOG_ERROR ( "[md5db][content_db]offset for update error" );
            return false;
        }

        if ( ! index.id )
        {
            if ( unlikely ( ! merge_cache_free_block ( old_block, index.offset / PAGE ) ) )
            {
                LOG_ERROR ( "[md5db][content_db]merge_cache_free_block error" );
                return false;
            }
        }
        else
        {
            if ( unlikely ( ! merge_free_block ( old_block, index.offset / PAGE ) ) )
            {
                LOG_ERROR ( "[md5db][content_db]merge_free_block error" );
                return false;
            }
        }

        return write_inner ( data, data_len, offset, block, tail );
    }

    bool content_t::get (
                          uint32_t            offset,
                          uint32_t            data_len,
                          char *              result
                          )
    {
        data_index_t index;
        fast_memcpy ( & index, & offset, 4 );
        
        scope_rlock_t lock ( m_lock );

        if ( ! index.id )
        {
            fast_memcpy ( result, m_cache_file.ptr + index.offset, data_len );
        }
        else
        {
            int fd = fileno ( m_data_file );
            ssize_t r = pread ( fd, result, data_len, index.offset );
            if ( data_len != ( size_t ) r )
            {
                LOG_ERROR ( "[md5db][content_db]pread failed offset:%d, data_len:%d, result:%s", index.offset, data_len, result );
                return false;
            }
        }

        return true;
    }

    bool content_t::del (
                          uint32_t            offset,
                          uint32_t            data_len
                          )
    {
        data_index_t index;
        fast_memcpy ( & index, & offset, 4 );

        uint32_t block = data_len / PAGE;
        if ( data_len % PAGE > 0 )
        {
            block ++;
        }

        if ( unlikely ( index.offset % PAGE > 0 ) )
        {
            LOG_ERROR ( "[md5db][content_db]offset for del error" );
            return false;
        }

        scope_wlock_t lock ( m_lock );

        if ( ! index.id )
        {
            return merge_cache_free_block ( block, index.offset / PAGE );
        }
        else
        {
            return merge_free_block ( block, index.offset / PAGE );
        }
    }

    void content_t::info (
                           std::stringstream & ss
                           )
    {
    }

} // namespace md5db
