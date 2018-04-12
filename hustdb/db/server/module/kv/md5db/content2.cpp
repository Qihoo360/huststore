#include <math.h>

#include "content2.h"
#include "../../perf_target.h"

namespace md5db
{

    static inline
    uint64_t get_file_size ( const char * path )
    {
        FILE * h;

        h = fopen ( path, "rb" );
        if ( NULL == h )
        {
            LOG_ERROR ( "fopen( %s ) failed", path );
            return 0;
        }

        uint64_t r = 0;

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
    , m_data_fd ( - 1 )
    , m_data_file_size ( 0 )
    , m_free_block_size ( 0 )
    , m_free_block_file_size ( 0 )
    , m_data_file ( NULL )
    , m_free_block_file ( NULL )
    , m_lock ( )
    {
        G_APPTOOL->fmap_init ( & m_free_block_cache );
        memset ( m_free_block_path, 0, sizeof ( m_free_block_path ) );
        memset ( m_tail, 0, PAGE );
    }

    content_t::~ content_t ( )
    {
        close ();
    }

    void content_t::close ( )
    {
        G_APPTOOL->fmap_close ( & m_free_block_cache );

        if ( m_free_block_file )
        {
            fclose ( m_free_block_file );
            m_free_block_file = NULL;
        }

        if ( m_data_file )
        {
            fclose ( m_data_file );
            m_data_file = NULL;
        }
    }

    bool content_t::open ( const char * path, int file_id )
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

        m_data_file_size = ftell ( m_data_file );
        if ( m_data_file_size < 0 )
        {
            LOG_ERROR ( "[md5db][content_db]ftell failed" );
            return false;
        }

        uint32_t tail = m_data_file_size % PAGE;
        if ( tail > 0 )
        {
            if ( tail != fwrite ( m_tail, 1, tail, m_data_file ) )
            {
                LOG_ERROR ( "[md5db][content_db]fwrite failed" );
                return false;
            }

            m_data_file_size += tail;
        }

        m_data_fd = fileno ( m_data_file );

        memset ( ph, 0, sizeof ( ph ) );
        strcpy ( ph, path );
        strcat ( ph, ".free.block" );
        if ( ! G_APPTOOL->is_file ( ph ) )
        {
            FILE * fp = fopen ( ph, "wb" );
            if ( ! fp )
            {
                LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
                return false;
            }

            bool ok = true;
            char buf[ FREE_BLOCK_INCR ];
            memset ( buf, 0, sizeof ( buf ) );
            if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
            {
                ok = false;
            }

            fflush ();

            fclose ( fp );
            if ( ! ok )
            {
                remove ( ph );
                return false;
            }
        }

        m_free_block_file = fopen ( ph, "rb+" );
        if ( ! m_free_block_file )
        {
            LOG_ERROR ( "[md5db][content_db]fopen %s failed", ph );
            return false;
        }

        int r = fseek ( m_free_block_file, 0, SEEK_END );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][content_db]seek end failed" );
            return false;
        }

        m_free_block_file_size = ftell ( m_free_block_file );
        if ( m_free_block_file_size < 0 )
        {
            LOG_ERROR ( "[md5db][content_db]ftell failed" );
            return false;
        }

        uint32_t incr = m_free_block_file_size % FREE_BLOCK_INCR;
        if ( incr > 0 )
        {
            char buf[ FREE_BLOCK_INCR ];
            memset ( buf, 0, sizeof ( buf ) );
            if ( incr != fwrite ( buf, 1, incr, m_free_block_file ) )
            {
                LOG_ERROR ( "[md5db][content_db]fwrite failed" );
                return false;
            }

            m_free_block_file_size += incr;
        }

        if ( ! G_APPTOOL->fmap_open ( & m_free_block_cache, ph, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db]fmap_open failed" );
            return false;
        }

        if ( m_free_block_cache.ptr_len != m_free_block_file_size )
        {
            LOG_ERROR ( "[md5db][content_db]mmap file size error, size: %d", m_free_block_cache.ptr_len );
            return false;
        }

        m_file_id = file_id;
        strcpy ( m_free_block_path, ph );

        parse_free_block ();

        LOG_INFO ( "[md5db][content_db]create success, path: %s, file_id: %d", path, file_id );

        return true;
    }

    void content_t::parse_free_block ( )
    {
        uint32_t              i;
        uint32_t              step;
        uint32_t              prev_i;
        struct free_block_t * head;
        struct free_block_t * block;
        struct free_block_t * prev;
        struct free_block_t * curr;

        step = sizeof ( struct free_block_t );
        i    = m_free_block_file_size - step;
        head = ( struct free_block_t * ) m_free_block_cache.ptr;

        m_free_block_merge.clear();

        while ( i > step )
        {
            block = ( struct free_block_t * ) & m_free_block_cache.ptr[ i ];

            if ( block->offset > 0 && block->size > 0 )
            {
                m_free_block_size += block->size;
                m_free_block_merge.insert ( std::pair < struct free_block_t *, uint32_t >( block, i ) );
            }
            else if ( block->offset == 0 )
            {
                block->offset = head->offset;
                head->offset  = i;
            }

            i -= step;
        }

        prev = NULL;
        m_free_block_list.clear();

        for ( free_block_list_t::iterator it = m_free_block_merge.begin(); it != m_free_block_merge.end (); it ++ )
        {
            curr = it->first;

            if ( unlikely ( ! prev ) )
            {
                prev   = curr;
                prev_i = it->second;
                continue;
            }

            if ( prev->offset + prev->size == curr->offset )
            {
                prev->size  += curr->size;

                curr->size   = 0;
                curr->offset = head->offset;
                head->offset = it->second;
            }
            else
            {
                m_free_block_list.insert ( std::pair < struct free_block_t *, uint32_t >( prev, prev_i ) );

                prev   = curr;
                prev_i = it->second;
            }
        }
        
        m_free_block_merge.clear();
    }

    bool content_t::incr_free_block_file ()
    {
        if ( unlikely ( ! m_free_block_file || m_free_block_cache.ptr_len < FREE_BLOCK_INCR ) )
        {
            LOG_ERROR( "[md5db][content_db]incr free block file, initial conditions error" );
            return false;
        }

        int r = fseek ( m_free_block_file, 0, SEEK_END );
        if ( unlikely ( 0 != r ) )
        {
            LOG_ERROR ( "[md5db][content_db]seek end failed" );
            return false;
        }

        char buf[ FREE_BLOCK_INCR ];
        memset ( buf, 0, sizeof ( buf ) );
        if ( FREE_BLOCK_INCR != fwrite ( buf, 1, FREE_BLOCK_INCR, m_free_block_file ) )
        {
            LOG_ERROR ( "[md5db][content_db]incr free block file, incr fwrite failed" );
            return false;
        }

        m_free_block_file_size += FREE_BLOCK_INCR;

        G_APPTOOL->fmap_close ( & m_free_block_cache );
        
        if ( ! G_APPTOOL->fmap_open ( & m_free_block_cache, m_free_block_path, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db]fmap_open failed" );
            return false;
        }

        if ( m_free_block_cache.ptr_len != m_free_block_file_size )
        {
            LOG_ERROR ( "[md5db][content_db]mmap file size error, size: %d", m_free_block_cache.ptr_len );
            return false;
        }

        parse_free_block ();

        return true;
    }

    bool content_t::use_free_block ( uint32_t size, uint32_t & offset )
    {
        if ( unlikely ( m_free_block_size < size ) )
        {
            return false;
        }

        struct free_block_t fb;
        fb.size = size - 1;

        free_block_list_t::iterator it = m_free_block_list.upper_bound ( & fb );
        if ( unlikely ( it == m_free_block_list.end () ) )
        {
            return false;
        }
        
        struct free_block_t * block    = it->first;
        uint32_t              block_i  = it->second;
        struct free_block_t * head     = ( struct free_block_t * ) m_free_block_cache.ptr;
        
        offset = block->offset;

        if ( size == block->size )
        {
            block->size   = 0;
            block->offset = head->offset;
            head->offset  = block_i;
        }
        else
        {
            block->offset = block->offset + size;
            block->size   = block->size - size;
        }

        m_free_block_list.erase ( it );

        if ( block->size > 0 )
        {
            m_free_block_list.insert ( std::pair < struct free_block_t *, uint32_t >( block, block_i ) );
        }

        m_free_block_size -= size;

        return true;
    }

    bool content_t::add_free_block ( uint32_t size, uint32_t offset )
    {
        struct free_block_t * block = NULL;
        struct free_block_t * head  = ( struct free_block_t * ) m_free_block_cache.ptr;
        
        if ( head->offset == 0 )
        {
            if ( ! incr_free_block_file () )
            {
                LOG_ERROR ( "[md5db][content_db]add free block failed" );
                return false;
            }
        
            head  = ( struct free_block_t * ) m_free_block_cache.ptr;
        }

        block         = ( struct free_block_t * ) & m_free_block_cache.ptr[ head->offset ];
        head->offset  = block->offset;

        block->offset = offset;
        block->size   = size;
        
        m_free_block_list.insert ( std::pair < struct free_block_t *, uint32_t >( block, head->offset ) );

        m_free_block_size += size;

        return true;
    }

    bool content_t::write (
                            const char *        data,
                            uint32_t            data_len,
                            uint32_t &          offset
                            )
    {
        uint32_t size = data_len / PAGE;
        uint32_t tail = data_len % PAGE;
        if ( tail > 0 )
        {
            tail = PAGE - tail;
            size ++;
        }

        scope_wlock_t lock ( m_lock );

        return write_inner ( data, data_len, offset, size, tail );
    }

    bool content_t::write_inner (
                                  const char * data,
                                  uint32_t data_len,
                                  uint32_t & offset,
                                  uint32_t size,
                                  uint32_t tail
                                  )
    {
        bool seek_end = false;

        bool r = use_free_block ( size, offset );
        if ( ! r )
        {
            offset = m_data_file_size / PAGE;

            int r = fseek ( m_data_file, 0, SEEK_END );
            if ( unlikely ( 0 != r ) )
            {
                LOG_ERROR ( "[md5db][content_db]seek end failed" );
                return false;
            }

            seek_end = true;
        }
        else
        {
            uint64_t real_offset = ( uint64_t ) offset * PAGE;

            int r = fseek ( m_data_file, real_offset, SEEK_SET );
            if ( unlikely ( 0 != r ) )
            {
                LOG_ERROR ( "[md5db][content_db]seek end failed" );
                return false;
            }
        }

        if ( data_len != fwrite ( data, 1, data_len, m_data_file ) )
        {
            LOG_ERROR ( "[md5db][content_db]fwrite data failed" );
            return false;
        }

        if ( seek_end && tail != fwrite ( m_tail, 1, tail, m_data_file )
        {
            LOG_ERROR ( "[md5db][content_db]fwrite tail failed" );
            return false;
        }

        m_data_file_size += data_len + tail;

        return true;
    }

    bool content_t::update (
                             uint32_t &          offset,
                             uint32_t            old_data_len,
                             const char *        data,
                             uint32_t            data_len
                             )
    {
        uint32_t old_size = old_data_len / PAGE;
        if ( old_data_len % PAGE > 0 )
        {
            old_size ++;
        }

        uint32_t size = data_len / PAGE;
        uint32_t tail = data_len % PAGE;
        if ( tail > 0 )
        {
            tail = PAGE - tail;
            size ++;
        }
        
        scope_wlock_t lock ( m_lock );

        if ( old_size == size )
        {
            uint64_t real_offset = ( uint64_t ) offset * PAGE;
            
            int r = fseek ( m_data_file, real_offset, SEEK_SET );
            if ( unlikely ( 0 != r ) )
            {
                LOG_ERROR ( "[md5db][content_db]seek end failed" );
                return false;
            }

            if ( data_len != fwrite ( data, 1, data_len, m_data_file ) )
            {
                LOG_ERROR ( "[md5db][content_db]fwrite data failed" );
                return false;
            }

            return true;
        }

        if ( ! add_free_block ( old_size, offset ) )
        {
            LOG_ERROR ( "[md5db][content_db]add free block failed" );
            return false;
        }

        return write_inner ( data, data_len, offset, size, tail );
    }

    bool content_t::get (
                          uint32_t            offset,
                          uint32_t            data_len,
                          char *              result
                          )
    {
        uint64_t real_offset = ( uint64_t ) offset * PAGE;
        
        scope_rlock_t lock ( m_lock );

        ssize_t r = pread ( m_data_fd, result, data_len, real_offset );
        if ( data_len != ( size_t ) r )
        {
            LOG_ERROR ( "[md5db][content_db]pread failed, offset: %d, data_len: %d", real_offset, data_len );
            return false;
        }

        return true;
    }

    bool content_t::del (
                          uint32_t            offset,
                          uint32_t            data_len
                          )
    {
        uint32_t size = data_len / PAGE;
        if ( data_len % PAGE > 0 )
        {
            size ++;
        }

        scope_wlock_t lock ( m_lock );

        if ( ! add_free_block ( size, offset ) )
        {
            LOG_ERROR ( "[md5db][content_db]add free block failed" );
            return false;
        }

        return true;
    }

    void content_t::info (
                           std::stringstream & ss
                           )
    {
    }

} // namespace md5db