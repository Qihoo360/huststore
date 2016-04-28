#ifndef _i_server_kv_h_
#define _i_server_kv_h_

#include "db_stdinc.h"
#include "i_kv.h"
#include <sstream>
#include <vector>

#define HASH_TB_LEN         80
#define EXPORT_DB_ALL       "db.all@"

enum kv_type_t
{
    NEW_KV = 0,
    EXIST_KV = 1,
    CONFLICT_KV = 2,
    ERROR_KV = 3
};

struct conn_ctxt_t
{
    uint32_t worker_id : 8;
    uint32_t _reserved : 24;

    conn_ctxt_t ( )
    : worker_id ( 0 )
    , _reserved ( 0 )
    {
    }
};

struct item_ctxt_t
{
    std::string key;
    std::string value;
    std::vector< int > hash_other_servers;
    int inner_file_id;
    int user_file_id;
    bool is_version_error;
    uint8_t kv_type;

    item_ctxt_t ( )
    : key ( )
    , value ( )
    , hash_other_servers ( )
    , inner_file_id ( -1 )
    , user_file_id ( -1 )
    , is_version_error ( false )
    , kv_type ( ERROR_KV )
    {
        key.reserve ( HASH_TB_LEN );
    }

    void reset ( )
    {
        key.resize ( 0 );
        value.resize ( 0 );
        hash_other_servers.resize ( 0 );
        inner_file_id = -1;
        user_file_id = -1;
        is_version_error = false;
        kv_type = ERROR_KV;
    }

} __attribute__ ( ( aligned ( 64 ) ) );

typedef void ( * export_record_callback_t )(
                                             void * param,
                                             const char * & key,
                                             size_t & key_len,
                                             const char * & val,
                                             size_t & val_len,
                                             const char * table,
                                             size_t table_len,
                                             uint32_t & version,
                                             uint32_t & ttl,
                                             std::string & content,
                                             bool * ignore_this_record,
                                             bool * break_the_loop
                                             );

struct export_cb_param_t
{
    void * db;
    uint16_t start;
    uint16_t end;
    uint32_t offset;
    uint32_t size;
    uint32_t total;
    uint64_t min;
    uint64_t max;
    int file_id;
    bool noval;
    bool async;

} __attribute__ ( ( aligned ( 64 ) ) );

class i_server_kv_t
{
public:

    virtual void kill_me ( ) = 0;

    virtual bool open ( ) = 0;

    virtual bool ok ( ) = 0;

    virtual void info (
                        std::stringstream & ss
                        ) = 0;

    virtual int get_user_file_count ( ) = 0;

    virtual int flush ( ) = 0;

    virtual int del (
                      const char * user_key,
                      size_t user_key_len,
                      uint32_t & version,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      ) = 0;

    virtual int put (
                      const char * user_key,
                      size_t use_key_len,
                      const char * val,
                      size_t val_len,
                      uint32_t & version,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      ) = 0;

    virtual int get (
                      const char * user_key,
                      size_t user_key_len,
                      uint32_t & version,
                      conn_ctxt_t conn,
                      std::string * & rsp,
                      item_ctxt_t * & ctxt
                      ) = 0;

    virtual int exists (
                         const char * user_key,
                         size_t user_key_len,
                         uint32_t & version,
                         conn_ctxt_t conn,
                         item_ctxt_t * & ctxt
                         ) = 0;

    virtual void hash (
                        const char * user_key,
                        size_t user_key_len,
                        conn_ctxt_t conn,
                        item_ctxt_t * & ctxt
                        ) = 0;

    virtual int export_db (
                            int file_id,
                            const char * path,
                            export_record_callback_t callback = NULL,
                            void * callback_param = NULL
                            ) = 0;

    virtual int export_db_mem (
                                conn_ctxt_t conn,
                                std::string * & rsp,
                                item_ctxt_t * & ctxt,
                                export_record_callback_t callback = NULL,
                                void * callback_param = NULL
                                ) = 0;

    virtual int hash_info (
                            int user_file_id,
                            int & inner_file_id
                            ) = 0;

    virtual void set_inner_ttl (
                                 uint32_t ttl,
                                 conn_ctxt_t conn
                                 ) = 0;

    virtual uint32_t get_inner_ttl (
                                     conn_ctxt_t conn
                                     ) = 0;

    virtual void set_inner_table (
                                   const char * table,
                                   size_t table_len,
                                   uint8_t type,
                                   conn_ctxt_t conn,
                                   const char * added = NULL
                                   ) = 0;

    virtual const char * get_inner_tbkey (
                                           const char * key,
                                           size_t & key_len,
                                           conn_ctxt_t conn
                                           ) = 0;

    virtual i_kv_t * get_conflict (
                                    int file_id
                                    )
    {
        return NULL;
    }

private:
    // disable
    static void operator delete( void * p );
};

i_server_kv_t * create_server_kv ( );

#endif
