#include "conflict.h"
#include "bucket.h"
#include "../../base.h"

namespace md5db
{

#pragma pack( push, 1 )

    struct conflict_item_t
    {
        uint32_t    version     : 30;
        uint32_t    _reserved   :  2;

        block_id_t  block_id;
    } ;

#pragma pack( pop )

    conflict_t::conflict_t ( )
    : m_db ( NULL )
    {

    }

    conflict_t::~ conflict_t ( )
    {
        close ();
    }

    bool conflict_t::open (
                            const char * path,
                            const kv_config_t & config,
                            int file_id
                            )
    {
        if ( m_db )
        {
            LOG_ERROR ( "[md5db][conflict]already opened, %s", path );
            return false;
        }

        m_db = create_kv ( );
        if ( NULL == m_db )
        {
            LOG_ERROR ( "[md5db][conflict]create_kv failed" );
            return false;
        }

        if ( ! m_db->open ( path, config, file_id ) )
        {
            LOG_ERROR ( "[md5db][conflict]open( %s ) failed", path );
            m_db->kill_me ();
            m_db = NULL;
            return false;
        }

        return true;
    }

    void conflict_t::close ( )
    {
        if ( m_db )
        {
            m_db->kill_me ();
            m_db = NULL;
        }
    }

    int conflict_t::del (
                          const void *        inner_key,
                          unsigned int        inner_key_len
                          )
    {
        return m_db->del ( inner_key, inner_key_len );
    }

    int conflict_t::put (
                          const void *        inner_key,
                          unsigned int        inner_key_len,
                          const block_id_t &  block_id,
                          uint32_t            version
                          )
    {
        if ( 16 != inner_key_len || NULL == inner_key || version > BUCKET_DATA_MAX_VERSION )
        {
            LOG_ERROR ( "[md5db][conflict]invalid params" );
            return EINVAL;
        }

        conflict_item_t data;
        data._reserved  = 0;
        data.version    = version;
        data.block_id   = block_id;

        int r = m_db->put ( inner_key, inner_key_len, & data, ( unsigned int ) sizeof ( data ) );

        return r;
    }

    int conflict_t::get (
                          const void *        inner_key,
                          unsigned int        inner_key_len,
                          block_id_t &        block_id,
                          uint32_t &          version
                          )
    {
        block_id.reset ();
        version = 0;

        if ( unlikely ( NULL == m_db ) )
        {
            LOG_ERROR ( "[md5db][conflict]m_db is NULL" );
            return EFAULT;
        }
        if ( 16 != inner_key_len || NULL == inner_key )
        {
            LOG_ERROR ( "[md5db][conflict]invalid key" );
            return EINVAL;
        }

        std::string s;
        int r = m_db->get ( inner_key, inner_key_len, s );
        if ( 0 != r )
        {
            if ( ENOENT != r )
            {
                LOG_ERROR ( "[md5db][conflict]get return %d", r );
            }
            return r;
        }
        if ( sizeof ( conflict_item_t ) != s.size () )
        {
            LOG_ERROR ( "[md5db][conflict]%d invalid size", ( int ) s.size () );
            return EFAULT;
        }

        const conflict_item_t * item = ( const conflict_item_t * ) s.c_str ();
        block_id = item->block_id;
        version  = item->version;
        if ( version > BUCKET_DATA_MAX_VERSION )
        {
            LOG_ERROR ( "[md5db][conflict]%u invalid version", version );
            return EFAULT;
        }

        return 0;
    }

    void conflict_t::info (
                            std::stringstream & ss
                            )
    {
        if ( m_db )
        {
            m_db->info ( ss );
        }
    }

} // namespace md5db
