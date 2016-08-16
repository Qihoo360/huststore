#include "rdb.h"

#define CHECK_KEY(key)             ( ! key || key##_len <= 0 || key##_len > RDB_KEY_LEN )
#define CHECK_VAL(val)             ( ! val || val##_len <= 0 || val##_len > RDB_VAL_LEN )

void rdb_t::kill_me ( )
{
    delete this;
}

rdb_t::rdb_t ( )
: m_buffers ( )
, m_ok ( false )
, m_rdb ( NULL )
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

    if ( rdb_init ( size ) != 0 )
    {
        return false;
    }

    m_rdb = createClient ( RDB_KEY_LEN + RDB_VAL_LEN );

    m_ok = true;

    return true;
}

int rdb_t::exist_or_del (
                          const char *        key,
                          size_t              key_len,
                          bool                is_exist,
                          conn_ctxt_t         conn
                          )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( key )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 2 ];

    cmds[ 0 ].cmd = ( char * ) ( is_exist ? "exists" : "del" );
    cmds[ 0 ].len = is_exist ? 6 : 3;
    cmds[ 1 ].cmd = ( char * ) key;
    cmds[ 1 ].len = key_len;

    std::string * rsp = m_buffers[ conn.worker_id ];
    size_t rsp_len = 0;

    int r = processInput ( m_rdb, 2, cmds, & rsp_len, & ( * rsp ) [ 0 ] );
    if ( unlikely ( r || strncmp (rsp->c_str (), "0", rsp_len) == 0 ) )
    {
        return ENOENT;
    }

    return 0;
}

int rdb_t::get_or_ttl (
                        const char *        key,
                        size_t              key_len,
                        std::string * &     rsp,
                        int *               rsp_len,
                        bool                is_get,
                        conn_ctxt_t         conn
                        )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( key )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 2 ];

    cmds[ 0 ].cmd = ( char * ) ( is_get ? "get" : "ttl" );
    cmds[ 0 ].len = 3;
    cmds[ 1 ].cmd = ( char * ) key;
    cmds[ 1 ].len = key_len;

    std::string * rspi = m_buffers[ conn.worker_id ];

    int r = processInput ( m_rdb, 2, cmds, ( size_t * ) rsp_len, & ( * rspi ) [ 0 ] );
    if ( unlikely ( r || ! * rsp_len ) )
    {
        return ENOENT;
    }

    rsp = rspi;
    
    return 0;
}

int rdb_t::set_or_append (
                           const char *        key,
                           size_t              key_len,
                           const char *        val,
                           size_t              val_len,
                           const char *        ttl,
                           size_t              ttl_len,
                           bool                is_set,
                           conn_ctxt_t         conn
                           )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( key ) ||
                   CHECK_VAL ( val )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 4 ];

    cmds[ 0 ].cmd = ( char * ) ( is_set ? "set" : "append" );
    cmds[ 0 ].len = is_set ? 3 : 6;
    cmds[ 1 ].cmd = ( char * ) key;
    cmds[ 1 ].len = key_len;
    cmds[ 2 ].cmd = ( char * ) val;
    cmds[ 2 ].len = val_len;
    cmds[ 3 ].cmd = ( char * ) ttl;
    cmds[ 3 ].len = ttl_len;

    std::string * rsp = m_buffers[ conn.worker_id ];
    size_t rsp_len = 0;

    int r = processInput ( m_rdb, ( ttl && ttl_len > 0 && is_set ) ? 4 : 3, cmds, & rsp_len, & ( * rsp ) [ 0 ] );
    if ( unlikely ( r ) )
    {
        return ENOENT;
    }

    return 0;
}

int rdb_t::expire_or_persist (
                               const char *        key,
                               size_t              key_len,
                               const char *        ttl,
                               size_t              ttl_len,
                               bool                is_expire,
                               conn_ctxt_t         conn
                               )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( key ) ||
                   ( is_expire && ( ttl_len <= 0 || ! ttl ) )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 3 ];

    cmds[ 0 ].cmd = ( char * ) ( is_expire ? "expire" : "persist" );
    cmds[ 0 ].len = is_expire ? 6 : 7;
    cmds[ 1 ].cmd = ( char * ) key;
    cmds[ 1 ].len = key_len;
    cmds[ 2 ].cmd = ( char * ) ttl;
    cmds[ 2 ].len = ttl_len;

    std::string * rsp = m_buffers[ conn.worker_id ];
    size_t rsp_len = 0;

    int r = processInput ( m_rdb, ( ttl && ttl_len > 0 && is_expire ) ? 3 : 2, cmds, & rsp_len, & ( * rsp ) [ 0 ] );
    if ( unlikely ( r || ! rsp_len ) )
    {
        return ENOENT;
    }

    return 0;
}

int rdb_t::hexist_or_hdel (
                            const char *        table,
                            size_t              table_len,
                            const char *        key,
                            size_t              key_len,
                            bool                is_hexist,
                            conn_ctxt_t         conn
                            )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( table ) ||
                   CHECK_KEY ( key )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 3 ];

    cmds[ 0 ].cmd = ( char * ) ( is_hexist ? "hexists" : "hdel" );
    cmds[ 0 ].len = is_hexist ? 7 : 4;
    cmds[ 1 ].cmd = ( char * ) table;
    cmds[ 1 ].len = table_len;
    cmds[ 2 ].cmd = ( char * ) key;
    cmds[ 2 ].len = key_len;

    std::string * rsp = m_buffers[ conn.worker_id ];
    size_t rsp_len = 0;

    int r = processInput ( m_rdb, 3, cmds, & rsp_len, & ( * rsp ) [ 0 ] );
    if ( unlikely ( r || strncmp (rsp->c_str (), "0", rsp_len) == 0 ) )
    {
        return ENOENT;
    }

    return 0;
}

int rdb_t::hget (
                  const char *        table,
                  size_t              table_len,
                  const char *        key,
                  size_t              key_len,
                  std::string * &     rsp,
                  int *               rsp_len,
                  conn_ctxt_t         conn
                  )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( table ) ||
                   CHECK_KEY ( key )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 3 ];

    cmds[ 0 ].cmd = ( char * ) "hget";
    cmds[ 0 ].len = 4;
    cmds[ 1 ].cmd = ( char * ) table;
    cmds[ 1 ].len = table_len;
    cmds[ 2 ].cmd = ( char * ) key;
    cmds[ 2 ].len = key_len;

    std::string * rspi = m_buffers[ conn.worker_id ];

    int r = processInput ( m_rdb, 3, cmds, ( size_t * ) rsp_len, & ( * rspi ) [ 0 ] );
    if ( unlikely ( r || ! * rsp_len ) )
    {
        return ENOENT;
    }
    
    rsp = rspi;

    return 0;
}

int rdb_t::hset (
                  const char *        table,
                  size_t              table_len,
                  const char *        key,
                  size_t              key_len,
                  const char *        val,
                  size_t              val_len,
                  conn_ctxt_t         conn
                  )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( table ) ||
                   CHECK_KEY ( key ) ||
                   CHECK_VAL ( val )
                   )
         )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 4 ];

    cmds[ 0 ].cmd = ( char * ) "hset";
    cmds[ 0 ].len = 4;
    cmds[ 1 ].cmd = ( char * ) table;
    cmds[ 1 ].len = table_len;
    cmds[ 2 ].cmd = ( char * ) key;
    cmds[ 2 ].len = key_len;
    cmds[ 3 ].cmd = ( char * ) val;
    cmds[ 3 ].len = val_len;

    std::string * rsp = m_buffers[ conn.worker_id ];
    size_t rsp_len = 0;

    int r = processInput ( m_rdb, 4, cmds, & rsp_len, & ( * rsp ) [ 0 ] );
    if ( unlikely ( r ) )
    {
        return ENOENT;
    }

    return 0;
}

int rdb_t::hincrby_or_hincrbyfloat (
                                     const char *        table,
                                     size_t              table_len,
                                     const char *        key,
                                     size_t              key_len,
                                     const char *        val,
                                     size_t              val_len,
                                     std::string * &     rsp,
                                     int *               rsp_len,
                                     bool                is_hincrby,
                                     conn_ctxt_t         conn
                                     )
{
    if ( unlikely ( ! m_ok ||
                   CHECK_KEY ( table ) ||
                   CHECK_KEY ( key ) ||
                   CHECK_KEY ( val )
                   )
         )
    {
        return EINVAL;
    }
    
    RdbCommand cmds[ 4 ];

    cmds[ 0 ].cmd = ( char * ) ( is_hincrby ? "hincrby" : "hincrbyfloat" );
    cmds[ 0 ].len = is_hincrby ? 7 : 12;
    cmds[ 1 ].cmd = ( char * ) table;
    cmds[ 1 ].len = table_len;
    cmds[ 2 ].cmd = ( char * ) key;
    cmds[ 2 ].len = key_len;
    cmds[ 3 ].cmd = ( char * ) val;
    cmds[ 3 ].len = val_len;

    std::string * rspi = m_buffers[ conn.worker_id ];

    int r = processInput ( m_rdb, 4, cmds, ( size_t * ) rsp_len, & ( * rspi ) [ 0 ] );
    if ( unlikely ( r || ! * rsp_len ) )
    {
        return ENOENT;
    }
    
    rsp = rspi;

    return 0;
}

int rdb_t::info (
                  std::string * &     rsp,
                  int *               rsp_len,
                  conn_ctxt_t         conn
                  )
{
    if ( unlikely ( ! m_ok ) )
    {
        return EINVAL;
    }

    RdbCommand cmds[ 2 ];

    cmds[ 0 ].cmd = ( char * ) "info";
    cmds[ 0 ].len = 4;
    cmds[ 1 ].cmd = ( char * ) "all";
    cmds[ 1 ].len = 3;

    std::string * rspi = m_buffers[ conn.worker_id ];

    int r = processInput ( m_rdb, 2, cmds, ( size_t * ) rsp_len, & ( * rspi ) [ 0 ] );
    if ( unlikely ( r || ! * rsp_len ) )
    {
        return ENOENT;
    }

    rsp = rspi;

    return 0;
}

void rdb_t::close ( )
{
    for ( rdb_buffers_t::iterator it = m_buffers.begin (); it != m_buffers.end (); ++ it )
    {
        delete ( * it );
    }

    m_ok = false;
}
