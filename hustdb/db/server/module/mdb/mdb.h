#ifndef _mdb_h_
#define _mdb_h_

#include "db_stdinc.h"
#include "../../../include/i_server_kv.h"
#include <vector>
#include <string>

#define MDB_KEY_LEN 255
#define MDB_VAL_LEN 1048571

class mdb_t
{
    
public:
    
    mdb_t ( );
    ~mdb_t ( );

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

    int del (
              const char * key,
              size_t key_len
              );

    int put (
              const char * key,
              size_t key_len,
              const char * val,
              size_t val_len,
              uint32_t ttl
              );

    int get (
              const char * key,
              size_t key_len,
              conn_ctxt_t conn,
              std::string * & rsp,
              int * rsp_len
              );

    std::string * buffer (
                           conn_ctxt_t conn
                           );

    void set_mdb_timestamp ( time_t timestamp );

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
    mdb_t ( const mdb_t & );
    const mdb_t & operator= ( const mdb_t & );
};

#endif // #ifndef _mdb_h_
