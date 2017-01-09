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

bool mdb_t::open (
                   int count,
                   int size
                   )
{
    try
    {
        m_buffers.reserve ( count );
        for ( int i = 0; i < count; ++ i )
        {
            std::string * s = new std::string ();
            s->resize ( MDB_VAL_LEN + 16 );

            m_buffers.push_back ( s );
        }
    }
    catch ( ... )
    {
        return false;
    }

    if ( size <= 0 )
    {
        m_ok = false;
        
        return true;
    }

    if ( mdb_init ( size ) != 0 )
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
                 const char *        key,
                 size_t              key_len,
                 conn_ctxt_t         conn,
                 std::string * &     rsp,
                 int *               rsp_len
                 )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    std::string * rspi = m_buffers[ conn.worker_id ];

    int r = process_get_command ( ( char * ) key, key_len, ( char * ) & ( * rspi ) [ 0 ], rsp_len );
    if ( unlikely ( ! r ) )
    {
        return ENOENT;
    }
    
    rsp = rspi;

    return 0;
}

int mdb_t::put (
                 const char *        key,
                 size_t              key_len,
                 const char *        val,
                 size_t              val_len,
                 uint32_t            ttl
                 )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    int r = process_update_command ( ( char * ) key, key_len, 0, ( char * ) val, val_len, ttl, 2 );
    if ( unlikely ( ! r ) )
    {
        return EINVAL;
    }

    return 0;
}

int mdb_t::del (
                 const char *        key,
                 size_t              key_len
                 )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    int r = process_delete_command ( ( char * ) key, key_len );
    if ( unlikely ( ! r ) )
    {
        return EINVAL;
    }

    return 0;
}

void mdb_t::set_mdb_timestamp ( time_t timestamp )
{
    if ( m_ok )
    {
        set_current_time ( timestamp );
    }
}

std::string * mdb_t::buffer (
                              conn_ctxt_t    conn
                              )
{
    return m_buffers[ conn.worker_id ];
}
