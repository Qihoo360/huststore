#include "mdb.h"
#include "lib/memcached.h"

void mdb_t::kill_me ( )
{
    delete this;
}

mdb_t::mdb_t ( )
: m_buffers ( )
, m_ok ( false )
{
}

mdb_t::~ mdb_t ( )
{
    close ();
}

bool mdb_t::open ( int count, int size, int sys_mem_threshold, int proc_mem_threshold )
{
    try
    {
        m_buffers.reserve ( count );
        for ( int i = 0; i < count; ++ i )
        {
            std::string * s = new std::string ();
            s->reserve ( MDB_VAL_LEN + 16 );
            s->resize ( 0 );

            m_buffers.push_back ( s );
        }
    }
    catch ( ... )
    {
        return false;
    }

    if ( sys_mem_threshold < 0 || sys_mem_threshold >= 100 || proc_mem_threshold < 0 || proc_mem_threshold >= 100 )
    {
        return false;
    }

    if ( size <= 0 )
    {
        size = 32;
    }

    set_mem_threshold ( sys_mem_threshold, proc_mem_threshold );

    if ( init ( size ) != 0 )
    {
        return false;
    }

    m_ok = true;

    return true;
}

void mdb_t::close ( )
{
    for ( mdb_buffers_t::iterator it = m_buffers.begin (); it != m_buffers.end (); ++ it )
    {
        delete ( * it );
    }

    m_ok = false;
}

int mdb_t::get (
                 const char *        user_key,
                 size_t              user_key_len,
                 conn_ctxt_t         conn,
                 std::string * &     rsp,
                 int *               rsp_len
                 )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    rsp = m_buffers[ conn.worker_id ];

    int r = process_get_command ( ( char * ) user_key, user_key_len, ( char * ) & ( * rsp ) [ 0 ] , rsp_len );
    if ( unlikely ( ! r ) )
    {
        return ENOENT;
    }

    return 0;
}

int mdb_t::put (
                 const char *        user_key,
                 size_t              user_key_len,
                 const char *        val,
                 size_t              val_len,
                 uint32_t            ttl
                 )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    int r = process_update_command ( ( char * ) user_key, user_key_len, 0, ( char * ) val, val_len, ttl, 2 );
    if ( unlikely ( ! r ) )
    {
        return EINVAL;
    }

    return 0;
}

int mdb_t::del (
                 const char *        user_key,
                 size_t              user_key_len
                 )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    int r = process_delete_command ( ( char * ) user_key, user_key_len );
    if ( unlikely ( ! r ) )
    {
        return EINVAL;
    }

    return 0;
}

uint32_t mdb_t::current_timestamp ( )
{
    return mdb_timestamp ( );
}

bool mdb_t::is_memory_threshold ( )
{
    return is_memory_over_threshold ( );
}

std::string * mdb_t::buffer (
                              conn_ctxt_t    conn
                              )
{
    if ( unlikely ( ! m_ok ) )
    {
        return NULL;
    }

    return m_buffers[ conn.worker_id ];
}
