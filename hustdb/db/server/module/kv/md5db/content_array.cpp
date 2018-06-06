#include "content_array.h"
#include "../../base.h"
#include "../../perf_target.h"
#include <memory>

namespace md5db
{

    content_array_t::content_array_t ( )
    : m_contents ( )
    , m_token ( )
    , m_ok ( false )
    {
    }

    content_array_t::~ content_array_t ( )
    {
        close ();
    }

    void content_array_t::close ( )
    {
        if ( ! m_contents.empty () )
        {
            for ( int i = 0; i < ( int ) m_contents.size (); ++ i )
            {
                content2_t * p = m_contents[ i ];
                m_contents[ i ] = NULL;
                if ( p )
                {
                    delete p;
                }
            }
            m_contents.resize ( 0 );
        }

        m_ok = false;
    }

    bool content_array_t::enable ( const char * storage_conf )
    {
        ini_t * ini = NULL;

        ini = G_APPINI->ini_create ( storage_conf );
        if ( NULL == ini )
        {
            LOG_ERROR ( "[md5db][content_db_array][enable][file=%s]open failed", 
                        storage_conf );
            return false;
        }
        int count = G_APPINI->ini_get_int ( ini, "contentdb", "count", 256 );
        G_APPINI->ini_destroy ( ini );

        return count > 0;
    }

    bool content_array_t::open (
                                 const char * path,
                                 const char * storage_conf
                                 )
    {
        ini_t * ini = NULL;

        ini = G_APPINI->ini_create ( storage_conf );
        if ( NULL == ini )
        {
            LOG_ERROR ( "[md5db][content_db_array][open][file=%s]open failed", 
                        storage_conf );
            return false;
        }

        bool b = open ( path, * ini );
        G_APPINI->ini_destroy ( ini );

        if ( ! b )
        {
            LOG_ERROR ( "[md5db][content_db_array][open]open failed" );
            return false;
        }

        return true;
    }

    bool content_array_t::open (
                                 const char *    path,
                                 ini_t &         ini
                                 )
    {
        int count = G_APPINI->ini_get_int ( & ini, "contentdb", "count", 256 );

        return open ( path, count );
    }

    bool content_array_t::open (
                                 const char *    path,
                                 int             count
                                 )
    {
        if ( ! m_contents.empty () )
        {
            LOG_ERROR ( "[md5db][content_db_array][open]contents not empty" );
            return false;
        }
        if ( NULL == path || '\0' == * path )
        {
            LOG_ERROR ( "[md5db][content_db_array][open]path empty" );
            return false;
        }
        if ( count <= 0 || count > 256 )
        {
            LOG_ERROR ( "[md5db][content_db_array][open][count=%d]invalid count", 
                        count );
            return false;
        }

        G_APPTOOL->make_dir ( path );

        int max_count_len = 0;
        {
            char buf[ 16 ];
            sprintf ( buf, "%d", count );
            max_count_len = ( int ) strlen ( buf );
        }
        if ( max_count_len > 9 )
        {
            LOG_ERROR ( "[md5db][content_db_array][open][count_string_len=%d]invalid count string len", 
                        count );
            return false;
        }

        char ph[ 260 ] = { };

        if ( m_contents.capacity () < ( size_t ) count )
        {
            try
            {
                m_contents.reserve ( count );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][content_db_array][open]bad_alloc" );
                return false;
            }
        }

        for ( int i = 0; i < count; i ++ )
        {
            strcpy ( ph, path );
            G_APPTOOL->path_to_os ( ph );
            if ( S_PATH_SEP_C != ph[ strlen ( ph ) - 1 ] )
            {
                strcat ( ph, S_PATH_SEP );
            }

            char t[ 32 ] = { };
            if ( 1 == max_count_len )
            {
                sprintf ( t, "%d", ( int ) i );
            }
            else if ( 2 == max_count_len )
            {
                sprintf ( t, "%02d", ( int ) i );
            }
            else if ( 3 == max_count_len )
            {
                sprintf ( t, "%03d", ( int ) i );
            }
            else if ( 4 == max_count_len )
            {
                sprintf ( t, "%04d", ( int ) i );
            }
            else if ( 5 == max_count_len )
            {
                sprintf ( t, "%05d", ( int ) i );
            }
            else if ( 6 == max_count_len )
            {
                sprintf ( t, "%06d", ( int ) i );
            }
            else if ( 7 == max_count_len )
            {
                sprintf ( t, "%07d", ( int ) i );
            }
            else if ( 8 == max_count_len )
            {
                sprintf ( t, "%08d", ( int ) i );
            }
            else if ( 9 == max_count_len )
            {
                sprintf ( t, "%09d", ( int ) i );
            }
            else
            {
                LOG_ERROR ( "[md5db][content_db_array][open][max_count_len=%d]invalid max_count_len", 
                            max_count_len );
                return false;
            }
            strcat ( ph, t );

            G_APPTOOL->make_dir ( ph );
            if ( ! G_APPTOOL->is_dir ( ph ) )
            {
                LOG_ERROR ( "[md5db][content_db_array][open][file=%s]create directory failed", 
                            ph );
                return false;
            }
            strcat ( ph, S_PATH_SEP );
            strcat ( ph, t );

            content2_t * p = NULL;
            try
            {
                p = new content2_t ();
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][content_db_array][open]bad_alloc" );
                return false;
            }

            if ( ! p->open ( ph, i ) )
            {
                LOG_ERROR ( "[md5db][content_db_array][open][file=%s]open failed", 
                            ph );
                return false;
            }

            try
            {
                m_contents.push_back ( p );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[md5db][content_db_array][open]bad_alloc" );
                return false;
            }
        }

        m_ok = true;

        LOG_INFO ( "[md5db][content_db_array][open][file=%s][count=%d]create success", 
                    path, count );

        return true;
    }

    bool content_array_t::update (
                                   uint32_t &          file_id,
                                   uint32_t &          data_id,
                                   uint32_t            old_data_len,
                                   const char *        data,
                                   uint32_t            data_len
                                   )
    {
        if ( unlikely ( ! m_ok || 
                        file_id >= m_contents.size () 
                        ) 
            )
        {
            LOG_ERROR ( "[md5db][content_db_array][update][file_id=%d]invalid file_id", 
                        file_id );
            return false;
        }

        content2_t * p = m_contents[ file_id ];
        if ( unlikely ( NULL == p ) )
        {
            LOG_ERROR ( "[md5db][content_db_array][update][file_id=%d]file is NULL", 
                        file_id );
            return false;
        }

        if ( unlikely ( ! p->update ( data_id, old_data_len, data, data_len ) ) )
        {
            LOG_ERROR ( "[md5db][content_db_array][update][file_id=%d][data_id=%d][old_data_len=%d][data_len=%d]update failed", 
                        file_id, data_id, old_data_len, data_len );
            return false;
        }

        return true;
    }

    bool content_array_t::write (
                                  const char *        data,
                                  uint32_t            data_len,
                                  uint32_t &          file_id,
                                  uint32_t &          data_id
                                  )
    {
        content2_t *  p       = NULL;
        uint32_t      size    = 0;
        uint32_t      offset  = 0;
        
        if ( unlikely ( ! m_ok ) )
        {
            return false;
        }

        file_id = 0;
        data_id = 0;
        size    = m_contents.size ();
        
        for ( int i = 0; i < size; i ++ )
        {
            m_token.increment ();
            file_id = m_token.get () % size;

            p = m_contents[ file_id ];
            
            if ( unlikely ( NULL == p ) )
            {
                LOG_ERROR ( "[md5db][content_db_array][write][file_id=%d]file is NULL",
                            file_id );
                file_id = 0;
                continue;
            }

            if ( unlikely ( ! p->write ( data, data_len, offset ) ) )
            {
                LOG_ERROR ( "[md5db][content_db_array][write][file_id=%d][data_len=%d]write failed", 
                            file_id, data_len );
                file_id = 0;
                continue;
            }

            data_id = offset;
            
            return true;
        }
        
        return false;
    }

    bool content_array_t::get (
                                uint32_t            file_id,
                                uint32_t            data_id,
                                uint32_t            data_len,
                                char *              result
                                )
    {
        if ( unlikely ( ! m_ok || 
                        file_id >= m_contents.size () 
                       ) 
            )
        {
            LOG_ERROR ( "[md5db][content_db_array][get][file_id=%d]invalid file_id", 
                        file_id );
            return false;
        }

        content2_t * p = m_contents[ file_id ];
        if ( unlikely ( NULL == p ) )
        {
            LOG_ERROR ( "[md5db][content_db_array][get][file_id=%d]file is NULL", 
                        file_id );
            return false;
        }

        return p->get ( data_id, data_len, result );
    }

    bool content_array_t::del (
                                uint32_t            file_id,
                                uint32_t            data_id,
                                uint32_t            data_len
                                )
    {
        if ( unlikely ( ! m_ok || 
                        file_id >= m_contents.size () 
                       ) 
            )
        {
            LOG_ERROR ( "[md5db][content_db_array][del][file_id=%d]invalid file_id", 
                        file_id );
            return false;
        }

        content2_t * p = m_contents[ file_id ];
        if ( unlikely ( NULL == p ) )
        {
            LOG_ERROR ( "[md5db][content_db_array][del][file_id=%d]file is NULL", 
                        file_id );
            return false;
        }

        return p->del ( data_id, data_len );
    }

    void content_array_t::info (
                                 std::stringstream & ss
                                 )
    {
        size_t count = m_contents.size ();
        for ( size_t i = 0; i < count; ++ i )
        {
            content2_t * p = m_contents[ i ];
            if ( p )
            {
                p->info ( ss );
            }
        }
    }

} // namespace md5db
