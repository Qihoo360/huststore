#include "fast_conflict_array.h"
#include "bucket_array.h"
#include "fullkey.h"
#include "../../base.h"

namespace md5db
{

    fast_conflict_array_t::fast_conflict_array_t ( )
    : m_bucket_array ( NULL )
    {
    }

    fast_conflict_array_t::~ fast_conflict_array_t ( )
    {
        close ();
    }

    void fast_conflict_array_t::close ( )
    {
        for ( int i = 0; i < COUNT_OF ( m_fast_conflicts ); ++ i )
        {
            m_fast_conflicts[ i ].close ();
        }
    }

    bool fast_conflict_array_t::open ( const char * path, bucket_array_t * buckets, const char * storage_conf )
    {
        ini_t * ini = NULL;

        ini = G_APPINI->ini_create ( storage_conf );
        if ( NULL == ini )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open][file=%s]open config failed", 
                        storage_conf );
            return false;
        }

        int conflict_count = G_APPINI->ini_get_int ( ini, "fast_conflictdb", "count", 4 );
        if ( conflict_count <= 0 || conflict_count > 10 )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open][conflict_count=%d]open failed", 
                        conflict_count );
            return false;
        }

        bool b = open ( path, buckets, conflict_count );
        G_APPINI->ini_destroy ( ini );

        return b;
    }

    bool fast_conflict_array_t::open ( const char * path, bucket_array_t * buckets, int conflict_count )
    {
        if ( NULL == path || '\0' == * path || NULL == buckets )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open]invalid path" );
            return false;
        }
        if ( conflict_count < 3 )
        {
            conflict_count = 3;
            LOG_INFO ( "[md5db][fast_conflict][open][conflict_count=%d]", 
                        conflict_count );
        }
        else if ( conflict_count > 10 )
        {
            LOG_ERROR ( "[md5db][fast_conflict][open][conflict_count=%d]too large conflict_count", 
                        conflict_count );
            return false;
        }

        G_APPTOOL->make_dir ( path );

        char ph[ 260 ];

        for ( int i = 0; i < COUNT_OF ( m_fast_conflicts ); ++ i )
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
            strcat ( ph, ".fastconflict" );

            if ( ! m_fast_conflicts[ i ].open ( ph, i, conflict_count ) )
            {
                LOG_ERROR ( "[md5db][fast_conflict][open][file=%s]open failed", 
                            ph );
                return false;
            }
        }

        m_bucket_array = buckets;
        return true;
    }

    void fast_conflict_array_t::info (
                                       std::stringstream & ss
                                       )
    {
        size_t count = size ();
        for ( size_t i = 0; i < count; ++ i )
        {
            fast_conflict_t & p = item ( i );
            p.info ( ss );
            
            if ( i < count - 1  )
            {
                ss << ",";
            }
        }
    }

    fast_conflict_t & fast_conflict_array_t::get_fast_conflict (
                                                                 const void *    inner_key,
                                                                 size_t          inner_key_len
                                                                 )
    {
        //assert ( inner_key_len >= 16 );
        
        unsigned char c = * ( ( const unsigned char * ) inner_key );
        return m_fast_conflicts[ c ];
    }

    bool fast_conflict_array_t::write (
                                        const void *                first_inner_key,
                                        size_t                      first_inner_key_len,
                                        const block_id_t &          first_block_id,
                                        uint32_t                    first_version,
                                        const void *                second_inner_key,
                                        size_t                      second_inner_key_len,
                                        const block_id_t &          second_block_id,
                                        uint32_t                    second_version,
                                        block_id_t &                addr
                                        )
    {
        if ( first_version > BUCKET_DATA_MAX_VERSION || second_version > BUCKET_DATA_MAX_VERSION )
        {
            return false;
        }

        fast_conflict_t & c = get_fast_conflict ( first_inner_key, first_inner_key_len );

        bucket_data_item_t b1;
        bucket_data_item_t b2;

        b1.set_type ( BUCKET_CONFLICT_DATA );
        b1.set_version ( first_version );
        b1.m_block_id = first_block_id;

        b2.set_type ( BUCKET_CONFLICT_DATA );
        b2.set_version ( second_version );
        b2.m_block_id = second_block_id;

        if ( ! c.write ( b1, b2, addr ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][write]write failed" );
            return false;
        }

        return true;
    }

    int fast_conflict_array_t::get (
                                     const void *                inner_key,
                                     size_t                      inner_key_len,
                                     md5db::block_id_t &         block_id,
                                     uint32_t &                  version
                                     )
    {
        // 0:   found
        // > 0: error, need not search conflict
        // < 0: error, need search conflict;

        block_id.reset ();
        version = 0;

        if ( NULL == m_bucket_array )
        {
            return - EFAULT;
        }

        fast_conflict_t & c = get_fast_conflict ( inner_key, inner_key_len );
        bucket_t & b        = m_bucket_array->get_bucket ( inner_key, inner_key_len );

        int r;

        bucket_data_item_t * bucket_item = NULL;
        r = b.find ( inner_key, inner_key_len, bucket_item );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][fast_conflict][get][r=%d]find in bucket", 
                        r );
            return - abs ( r );
        }
        if ( NULL == bucket_item || BUCKET_CONFLICT_DATA != bucket_item->type () )
        {
            LOG_ERROR ( "[md5db][fast_conflict][get]invalid item in bucket" );
            return - EFAULT;
        }

        bucket_data_item_t * fast_conflict_item       = NULL;
        size_t               fast_conflict_item_count = 0;
        if ( ! c.get ( bucket_item->m_block_id, fast_conflict_item, & fast_conflict_item_count ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][get]find fast_conflict item failed" );
            return - EFAULT;
        }

        uint32_t valid_conflict_count = 0;
        for ( size_t i = 0; i < fast_conflict_item_count; ++ i )
        {
            bucket_data_item_t * item = & fast_conflict_item[ i ];
            if ( 0 == item->m_block_id.data_id () )
            {
                continue;
            }

            bool v = b.get_fullkey ()->compare ( item->m_block_id, ( const char * ) inner_key, inner_key_len );
            if ( ! v )
            {
                ++ valid_conflict_count;
                continue;
            }

            // found
            block_id = item->m_block_id;
            version  = item->version ();

            return 0;
        }

        uint32_t conflict_count = bucket_item->get_conflict_count ();
        if ( likely ( conflict_count == valid_conflict_count ) )
        {
            return ENOENT;
        }
        else if ( conflict_count > valid_conflict_count )
        {
            return - ENOENT;
        }

        LOG_ERROR ( "[md5db][fast_conflict][get][conflict_count=%u][valid_conflict_count=%u]", 
                    conflict_count, valid_conflict_count );
        
        return - EFAULT;
    }

    int fast_conflict_array_t::add (
                                     const void *                inner_key,
                                     size_t                      inner_key_len,
                                     const md5db::block_id_t &   block_id,
                                     uint32_t                    version
                                     )
    {
        if ( version > BUCKET_DATA_MAX_VERSION )
        {
            return EINVAL;
        }
        if ( NULL == m_bucket_array )
        {
            return - EFAULT;
        }

        int         r;
        block_id_t  bid;
        uint32_t    v;
        
        r = get ( inner_key, inner_key_len, bid, v );
        r = abs ( r );
        if ( 0 == r )
        {
            LOG_ERROR ( "[md5db][fast_conflict][add]already exist, MD5 conflict" );
            return EEXIST;
        }
        else if ( ENOENT != r )
        {
            LOG_ERROR ( "[md5db][fast_conflict][add][r=%d]get return", 
                        r );
            return - EFAULT;
        }


        fast_conflict_t & c = get_fast_conflict ( inner_key, inner_key_len );
        bucket_t & b        = m_bucket_array->get_bucket ( inner_key, inner_key_len );

        bucket_data_item_t * bucket_item = NULL;
        r = b.find ( inner_key, inner_key_len, bucket_item );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][fast_conflict][add][r=%d]find in bucket", 
                        r );
            return - abs ( r );
        }
        if ( NULL == bucket_item || BUCKET_CONFLICT_DATA != bucket_item->type () )
        {
            LOG_ERROR ( "[md5db][fast_conflict][add]invalid item in bucket" );
            return - EFAULT;
        }

        bucket_data_item_t * fast_conflict_item = NULL;
        size_t               fast_conflict_item_count = 0;
        if ( ! c.get ( bucket_item->m_block_id, fast_conflict_item, & fast_conflict_item_count ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][add]find fast_conflict item failed" );
            return - EFAULT;
        }

        for ( size_t i = 0; i < fast_conflict_item_count; ++ i )
        {
            bucket_data_item_t * item = & fast_conflict_item[ i ];
            if ( 0 == item->m_block_id.data_id () )
            {
                item->set_type ( BUCKET_CONFLICT_DATA );
                item->set_version ( version );
                item->m_block_id = block_id;
                return 0;
            }
        }

        return - 1;
    }

    int fast_conflict_array_t::update (
                                        const void *                inner_key,
                                        size_t                      inner_key_len,
                                        const md5db::block_id_t &   block_id,
                                        uint32_t                    version
                                        )
    {
        if ( version > BUCKET_DATA_MAX_VERSION )
        {
            return EINVAL;
        }
        if ( NULL == m_bucket_array )
        {
            return - EFAULT;
        }

        fast_conflict_t & c = get_fast_conflict ( inner_key, inner_key_len );
        bucket_t & b        = m_bucket_array->get_bucket ( inner_key, inner_key_len );

        int r;

        bucket_data_item_t * bucket_item = NULL;
        r = b.find ( inner_key, inner_key_len, bucket_item );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][fast_conflict][update][r=%d]find in bucket", 
                        r );
            return - abs ( r );
        }
        if ( NULL == bucket_item || BUCKET_CONFLICT_DATA != bucket_item->type () )
        {
            LOG_ERROR ( "[md5db][fast_conflict][update]invalid item in bucket" );
            return - EFAULT;
        }

        bucket_data_item_t * fast_conflict_item       = NULL;
        size_t               fast_conflict_item_count = 0;
        if ( ! c.get ( bucket_item->m_block_id, fast_conflict_item, & fast_conflict_item_count ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][update]find fast_conflict item failed" );
            return - EFAULT;
        }

        uint32_t valid_conflict_count = 0;
        for ( size_t i = 0; i < fast_conflict_item_count; ++ i )
        {
            bucket_data_item_t * item = & fast_conflict_item[ i ];
            if ( 0 == item->m_block_id.data_id () )
            {
                continue;
            }

            bool v;
            v = b.get_fullkey ()->compare ( item->m_block_id, ( const char * ) inner_key, inner_key_len );
            if ( ! v )
            {
                ++ valid_conflict_count;
                continue;
            }

            // found
            if ( item->m_block_id != block_id )
            {
                LOG_ERROR ( "[md5db][fast_conflict][update]block_id not match" );
                return EINVAL;
            }

            if ( item->version () != version )
            {
                item->set_version ( version );
            }

            return 0;
        }

        uint32_t conflict_count = bucket_item->get_conflict_count ();
        if ( likely ( conflict_count == valid_conflict_count ) )
        {
            return ENOENT;
        }
        else if ( conflict_count > valid_conflict_count )
        {
            return - ENOENT;
        }

        LOG_ERROR ( "[md5db][fast_conflict][update][conflict_count=%u][valid_conflict_count=%u]", 
                    conflict_count, valid_conflict_count );

        return - EFAULT;
    }

    int fast_conflict_array_t::del (
                                     const void *                inner_key,
                                     size_t                      inner_key_len
                                     )
    {
        if ( NULL == m_bucket_array )
        {
            return - EFAULT;
        }

        fast_conflict_t & c = get_fast_conflict ( inner_key, inner_key_len );
        bucket_t & b        = m_bucket_array->get_bucket ( inner_key, inner_key_len );

        int r;

        bucket_data_item_t * bucket_item = NULL;
        r = b.find ( inner_key, inner_key_len, bucket_item );
        if ( 0 != r )
        {
            LOG_ERROR ( "[md5db][fast_conflict][del][r=%d]find in bucket", 
                        r );
            return - abs ( r );
        }
        if ( NULL == bucket_item || BUCKET_CONFLICT_DATA != bucket_item->type () )
        {
            LOG_ERROR ( "[md5db][fast_conflict][del]invalid item in bucket" );
            return - EFAULT;
        }

        bucket_data_item_t * fast_conflict_item = NULL;
        size_t               fast_conflict_item_count = 0;
        if ( ! c.get ( bucket_item->m_block_id, fast_conflict_item, & fast_conflict_item_count ) )
        {
            LOG_ERROR ( "[md5db][fast_conflict][del]find fast_conflict item failed" );
            return - EFAULT;
        }

        uint32_t valid_conflict_count = 0;
        for ( size_t i = 0; i < fast_conflict_item_count; ++ i )
        {
            bucket_data_item_t * item = & fast_conflict_item[ i ];
            if ( 0 == item->m_block_id.data_id () )
            {
                continue;
            }

            bool v;
            v = b.get_fullkey ()->compare ( item->m_block_id, ( const char * ) inner_key, inner_key_len );
            if ( ! v )
            {
                ++ valid_conflict_count;
                continue;
            }

            memset ( item, 0, sizeof ( bucket_data_item_t ) );
            
            return 0;
        }

        uint32_t conflict_count = bucket_item->get_conflict_count ();
        if ( likely ( conflict_count == valid_conflict_count ) )
        {
            return ENOENT;
        }
        else if ( conflict_count > valid_conflict_count )
        {
            return - ENOENT;
        }

        LOG_ERROR ( "[md5db][fast_conflict][del][conflict_count=%u][valid_conflict_count=%u]", 
                    conflict_count, valid_conflict_count );
        return - EFAULT;
    }

} // namespace md5db
