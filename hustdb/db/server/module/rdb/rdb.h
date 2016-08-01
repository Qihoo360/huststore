#ifndef _rdb_h_
#define _rdb_h_

#include "db_stdinc.h"
#include "../../../include/i_server_kv.h"
#include <vector>
#include <string>

#define RDB_KEY_LEN 255
#define RDB_VAL_LEN 1048572

class rdb_t
{
public:
    rdb_t ( );
    ~rdb_t ( );

    void kill_me ( );

    bool open (
                int count,
                int size
                );

    void close ( );

    bool ok ( )
    {
        return m_ok;
    }

    std::string * buffer (
                           conn_ctxt_t conn
                           );

    static void * operator new( size_t size )
    {
        void * p = malloc ( size );
        if ( NULL == p )
        {
            throw std::bad_alloc ( );
        }
        return p;
    }

    static void operator delete( void * p )
    {
        free ( p );
    }

private:

    typedef std::vector< std::string * > mdb_buffers_t;

    mdb_buffers_t m_buffers;
    bool m_ok;

private:
    // disable
    rdb_t ( const rdb_t & );
    const rdb_t & operator= ( const rdb_t & );
};

#endif // #ifndef _rdb_h_
