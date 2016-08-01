#include "rdb.h"
#include "lib/rdb_in.h"

void rdb_t::kill_me ( )
{
    delete this;
}

rdb_t::rdb_t ( )
: m_buffers ( )
, m_ok ( false )
{
}

rdb_t::~ rdb_t ( )
{
    close ();
}

bool rdb_t::open (
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
            s->reserve ( RDB_VAL_LEN + 16 );
            s->resize ( 0 );

            m_buffers.push_back ( s );
        }
    }
    catch ( ... )
    {
        return false;
    }

    if ( size <= 0 )
    {
        size = 64;
    }

    if ( init ( size ) != 0 )
    {
        return false;
    }

    m_ok = true;

    return true;
}

void rdb_t::close ( )
{
    for ( mdb_buffers_t::iterator it = m_buffers.begin (); it != m_buffers.end (); ++ it )
    {
        delete ( * it );
    }

    m_ok = false;
}

std::string * rdb_t::buffer (
                              conn_ctxt_t    conn
                              )
{
    if ( unlikely ( ! m_ok ) )
    {
        return NULL;
    }

    return m_buffers[ conn.worker_id ];
}
