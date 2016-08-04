#ifndef _rdb_h_
#define _rdb_h_

#include "db_stdinc.h"
#include "../../../include/i_server_kv.h"
#include "lib/redis.h"
#include <vector>
#include <string>

#define RDB_KEY_LEN 65536
#define RDB_VAL_LEN 8388608

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

public:

    int exist_or_del (
                       const char * key,
                       size_t key_len,
                       bool is_exist,
                       conn_ctxt_t conn
                       );

    int get_or_ttl (
                     const char * key,
                     size_t key_len,
                     std::string * & rsp,
                     int * rsp_len,
                     bool is_get,
                     conn_ctxt_t conn
                     );

    int set_or_append (
                        const char * key,
                        size_t key_len,
                        const char * val,
                        size_t val_len,
                        const char * ttl,
                        size_t ttl_len,
                        bool is_set,
                        conn_ctxt_t conn
                        );

    int expire_or_persist (
                            const char * key,
                            size_t key_len,
                            const char * ttl,
                            size_t ttl_len,
                            bool is_expire,
                            conn_ctxt_t conn
                            );
    
public:

    int hexist_or_hdel (
                         const char * table,
                         size_t table_len,
                         const char * key,
                         size_t key_len,
                         bool is_hexist,
                         conn_ctxt_t conn
                         );

    int hget (
               const char * table,
               size_t table_len,
               const char * key,
               size_t key_len,
               std::string * & rsp,
               int * rsp_len,
               conn_ctxt_t conn
               );

    int hset (
               const char * table,
               size_t table_len,
               const char * key,
               size_t key_len,
               const char * val,
               size_t val_len,
               conn_ctxt_t conn
               );

    int hincrby_or_hincrbyfloat (
                                  const char * table,
                                  size_t table_len,
                                  const char * key,
                                  size_t key_len,
                                  const char * val,
                                  size_t val_len,
                                  std::string * & rsp,
                                  int * rsp_len,
                                  bool is_hincrby,
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

    typedef std::vector< std::string * > rdb_buffers_t;

    rdb_buffers_t m_buffers;
    bool m_ok;
    client * m_rdb;

private:
    // disable
    rdb_t ( const rdb_t & );
    const rdb_t & operator= ( const rdb_t & );
};

#endif // #ifndef _rdb_h_
