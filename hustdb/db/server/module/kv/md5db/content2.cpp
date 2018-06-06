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

    content2_t::content2_t ( )
    : m_file_id ( - 1 )
    , m_data_fd ( - 1 )
    , m_data_file_size ( 0 )
    , m_free_block_max ( 0 )
    , m_free_block_file_size ( 0 )
    , m_data_file ( NULL )
    , m_free_block_file ( NULL )
    , m_lock ( )
    {
        G_APPTOOL->fmap_init ( & m_free_block_cache );
        memset ( m_free_block_path, 0, sizeof ( m_free_block_path ) );
        memset ( m_tail, 0, PAGE );
    }

    content2_t::~ content2_t ( )
    {
        close ();
    }

    void content2_t::close ( )
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

    bool content2_t::open ( const char * path, int file_id )
    {
        if ( file_id < 0 )
        {
            LOG_ERROR ( "[md5db][content_db][open]invalid file_id" );
            return false;
        }

        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][content_db][open]path error" );
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
                LOG_ERROR ( "[md5db][content_db][open][file=%s]fopen failed", 
                            ph );
                return false;
            }

            bool ok = true;
            char buf[ PAGE ];
            memset ( buf, 0, sizeof ( buf ) );
            if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
            {
                ok = false;
            }

            fflush ( fp );

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
            LOG_ERROR ( "[md5db][content_db][open][file=%s]fopen failed", 
                        ph );
            return false;
        }

        int r = fseek ( m_data_file, 0, SEEK_END );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][content_db][open]seek end failed" );
            return false;
        }

        m_data_file_size = ftell ( m_data_file );
        if ( m_data_file_size < 0 )
        {
            LOG_ERROR ( "[md5db][content_db][open]ftell failed" );
            return false;
        }

        uint32_t tail = m_data_file_size % PAGE;
        if ( tail > 0 )
        {
            tail = PAGE - tail;

            if ( tail != fwrite ( m_tail, 1, tail, m_data_file ) )
            {
                LOG_ERROR ( "[md5db][content_db][open]fwrite failed" );
                return false;
            }

            fflush ( m_data_file );

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
                LOG_ERROR ( "[md5db][content_db][open][file=%s]fopen failed", 
                            ph );
                return false;
            }

            bool ok = true;
            char buf[ PAGE ];
            memset ( buf, 0, sizeof ( buf ) );
            for ( int i = 0; i < FREE_BLOCK_PAGE; i ++ )
            {
                if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
                {
                    ok = false;
                    break;
                }
            }

            fflush ( fp );

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
            LOG_ERROR ( "[md5db][content_db][open]fopen failed", 
                        ph );
            return false;
        }

        r = fseek ( m_free_block_file, 0, SEEK_END );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][content_db][open]seek end failed" );
            return false;
        }

        m_free_block_file_size = ftell ( m_free_block_file );
        if ( m_free_block_file_size < 0 )
        {
            LOG_ERROR ( "[md5db][content_db][open]ftell failed" );
            return false;
        }

        uint32_t incr = m_free_block_file_size % FREE_BLOCK_INCR;
        if ( incr > 0 )
        {
            incr = FREE_BLOCK_INCR - incr;

            char buf[ PAGE ];
            memset ( buf, 0, sizeof ( buf ) );

            int incr_i = incr / PAGE;
            int incr_j = incr % PAGE;
            for ( int i = 0; i < incr_i; i ++ )
            {
                if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), m_free_block_file ) )
                {
                    LOG_ERROR ( "[md5db][content_db][open]fwrite failed" );
                    return false;
                }
            }

            if ( incr_j > 0 && incr != fwrite ( buf, 1, incr_j, m_free_block_file ) )
            {
                LOG_ERROR ( "[md5db][content_db][open]fwrite failed" );
                return false;
            }

            fflush ( m_free_block_file );

            m_free_block_file_size += incr;
        }

        if ( ! G_APPTOOL->fmap_open ( & m_free_block_cache, ph, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db][open]fmap_open failed" );
            return false;
        }

        if ( m_free_block_cache.ptr_len != m_free_block_file_size )
        {
            LOG_ERROR ( "[md5db][content_db][open][size=%d]mmap file size error", 
                        m_free_block_cache.ptr_len );
            return false;
        }

        m_file_id = file_id;
        strcpy ( m_free_block_path, ph );

        parse_free_block ();

        LOG_INFO ( "[md5db][content_db][open][path=%s][file_id=%d]create success", 
                    path, file_id );

        return true;
    }

    void content2_t::parse_free_block ( )
    {
        uint32_t              i;
        uint32_t              step;
        uint32_t              prev_i;
        struct free_block_t * header;
        struct free_block_t * block;
        struct free_block_t * prev;
        struct free_block_t * curr;

        step   = sizeof ( struct free_block_t );
        i      = m_free_block_file_size - step;
        header = ( struct free_block_t * ) m_free_block_cache.ptr;

        if ( header->offset == 0 )
        {
            header->offset = FREE_BLOCK_FILE_MAX_SIZE;
        }

        m_free_block_merge.clear();

        while ( i >= step )
        {
            block = ( struct free_block_t * ) & m_free_block_cache.ptr[ i ];

            if ( block->offset > 0 && block->size > 0 )
            {
                m_free_block_merge.insert ( std::pair < struct free_block_t *, uint32_t >( block, i ) );
            }
            else if ( block->offset == 0 )
            {
                block->offset  = header->offset;
                header->offset = i;
            }

            i -= step;
        }

        prev = NULL;
        m_free_block_list.clear();

        for ( free_block_list_t::iterator it = m_free_block_merge.begin (); it != m_free_block_merge.end (); it ++ )
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
                prev->size    += curr->size;

                curr->size     = 0;
                curr->offset   = header->offset;
                header->offset = it->second;
            }
            else
            {
                m_free_block_list.insert ( std::pair < struct free_block_t *, uint32_t >( prev, prev_i ) );

                prev   = curr;
                prev_i = it->second;
            }
        }

        if ( prev != NULL )
        {
            m_free_block_list.insert ( std::pair < struct free_block_t *, uint32_t >( prev, prev_i ) );
        }
        
        m_free_block_merge.clear();
    }

    bool content2_t::incr_free_block_file ()
    {
        if ( unlikely ( ! m_free_block_file || 
                        m_free_block_cache.ptr_len < FREE_BLOCK_INCR ||
                        m_free_block_file_size >= FREE_BLOCK_FILE_MAX_SIZE
                        ) 
            )
        {
            LOG_ERROR( "[md5db][content_db][incr_free_block_file]initial conditions error" );
            return false;
        }

        int r = fseek ( m_free_block_file, 0, SEEK_END );
        if ( unlikely ( 0 != r ) )
        {
            LOG_ERROR ( "[md5db][content_db][incr_free_block_file]seek end failed" );
            return false;
        }

        char buf[ PAGE ];
        memset ( buf, 0, sizeof ( buf ) );
        for ( int i = 0; i < FREE_BLOCK_PAGE; i ++ )
        {
            if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), m_free_block_file ) )
            {
                LOG_ERROR ( "[md5db][content_db][incr_free_block_file][i=%d]incr fwrite failed", 
                            i );
                return false;
            }
        }

        fflush ( m_free_block_file );

        m_free_block_file_size += FREE_BLOCK_INCR;

        G_APPTOOL->fmap_close ( & m_free_block_cache );
        
        if ( ! G_APPTOOL->fmap_open ( & m_free_block_cache, m_free_block_path, 0, 0, true ) )
        {
            LOG_ERROR ( "[md5db][content_db][incr_free_block_file]fmap_open failed" );
            return false;
        }

        if ( m_free_block_cache.ptr_len != m_free_block_file_size )
        {
            LOG_ERROR ( "[md5db][content_db][incr_free_block_file][size=%d]mmap file size error", 
                        m_free_block_cache.ptr_len );
            return false;
        }

        parse_free_block ();

        return true;
    }

    bool content2_t::use_free_block ( uint32_t size, uint32_t & offset )
    {
        if ( unlikely ( m_free_block_max < size ) )
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
        struct free_block_t * header   = ( struct free_block_t * ) m_free_block_cache.ptr;
        
        offset = block->offset;

        if ( size == block->size )
        {
            block->size    = 0;
            block->offset  = header->offset;
            header->offset = block_i;
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

        if ( ! m_free_block_list.empty () )
        {
            free_block_list_t::reverse_iterator rit = m_free_block_list.rbegin(); 
            m_free_block_max = rit->first->size;
        }
        else
        {
            m_free_block_max = 0;
        }

        return true;
    }

    bool content2_t::add_free_block ( uint32_t size, uint32_t offset )
    {
        struct free_block_t * block   = NULL;
        uint32_t              block_i = 0;
        struct free_block_t * header  = ( struct free_block_t * ) m_free_block_cache.ptr;
        
        if ( header->offset == FREE_BLOCK_FILE_MAX_SIZE )
        {
            if ( ! incr_free_block_file () )
            {
                LOG_ERROR ( "[md5db][content_db][add_free_block]add free block failed" );
                return false;
            }
        
            header = ( struct free_block_t * ) m_free_block_cache.ptr;
        }

        block          = ( struct free_block_t * ) & m_free_block_cache.ptr[ header->offset ];
        block_i        = header->offset;
        header->offset = block->offset;

        block->offset  = offset;
        block->size    = size;
        
        m_free_block_list.insert ( std::pair < struct free_block_t *, uint32_t >( block, block_i ) );

        if ( m_free_block_max < size )
        {
            m_free_block_max = size;   
        }

        return true;
    }

    bool content2_t::write (
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


        if ( unlikely ( m_data_file_size > DATA_FILE_MAX_SIZE &&
                        m_free_block_max < size
                        ) 
        )
        {
            return false;
        }

        scope_wlock_t lock ( m_lock );

        return write_inner ( data, data_len, offset, size, tail );
    }

    bool content2_t::write_inner (
                                  const char * data,
                                  uint32_t data_len,
                                  uint32_t & offset,
                                  uint32_t size,
                                  uint32_t tail
                                  )
    {
        ssize_t  pw       = 0;
        bool     seek_end = false;

        bool r = use_free_block ( size, offset );
        if ( ! r )
        {
            offset = m_data_file_size / PAGE;

            pw = pwrite ( m_data_fd, data, data_len, m_data_file_size );
            if ( unlikely ( data_len != ( uint32_t ) pw ) )
            {
                LOG_ERROR ( "[md5db][content_db][write_inner]pwrite data failed" );
                return false;
            }

            m_data_file_size += data_len;

            pw = pwrite ( m_data_fd, m_tail, tail, m_data_file_size );
            if ( tail != ( uint32_t ) pw )
            {
                LOG_ERROR ( "[md5db][content_db][write_inner]pwrite tail failed" );
                return false;
            }

            m_data_file_size += tail;
        }
        else
        {
            uint64_t real_offset = ( uint64_t ) offset * PAGE;

            pw = pwrite ( m_data_fd, data, data_len, real_offset );
            if ( unlikely ( data_len != ( uint32_t ) pw ) )
            {
                LOG_ERROR ( "[md5db][content_db][write_inner]pwrite data failed" );
                return false;
            }
        }

        return true;
    }

    bool content2_t::update (
                             uint32_t &          offset,
                             uint32_t            old_data_len,
                             const char *        data,
                             uint32_t            data_len
                             )
    {
        ssize_t  pw       = 0;
        uint32_t old_size = 0;
        
        old_size = old_data_len / PAGE;
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

        if ( old_size >= size )
        {
            uint64_t real_offset = ( uint64_t ) offset * PAGE;
            
            pw = pwrite ( m_data_fd, data, data_len, real_offset );
            if ( unlikely ( data_len != ( uint32_t ) pw ) )
            {
                LOG_ERROR ( "[md5db][content_db][update]pwrite data failed" );
                return false;
            }

            if ( old_size > size )
            {
                if ( ! add_free_block ( old_size - size, offset + size ) )
                {
                    LOG_ERROR ( "[md5db][content_db][update]add free block failed" );
                    return false;
                }
            }

            return true;
        }

        if ( unlikely ( m_data_file_size > DATA_FILE_MAX_SIZE &&
                        m_free_block_max < size
                        ) 
        )
        {
            return false;
        }

        if ( ! add_free_block ( old_size, offset ) )
        {
            LOG_ERROR ( "[md5db][content_db][update]add free block failed" );
            return false;
        }

        return write_inner ( data, data_len, offset, size, tail );
    }

    bool content2_t::get (
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
            LOG_ERROR ( "[md5db][content_db][get][offset=%d][data_len=%d]pread failed", 
                        real_offset, data_len );
            return false;
        }

        return true;
    }

    bool content2_t::del (
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
            LOG_ERROR ( "[md5db][content_db][del]add free block failed" );
            return false;
        }

        return true;
    }

    void content2_t::info (
                           std::stringstream & ss
                           )
    {
    }

} // namespace md5db