#ifndef _kv_array_h_
#define _kv_array_h_

#include "i_kv.h"
#include "kv_config.h"
#include "../../../include/i_server_kv.h"
#include <vector>
#include <string>

namespace md5db
{
    class block_id_t;
}

class key_hash_t;

class kv_array_t : public i_server_kv_t
{
public:
    kv_array_t ( );
    ~kv_array_t ( );

    void kill_me ( );

    bool open ( );

    void close ( );

    bool ok ( )
    {
        return m_ok;
    }

    virtual int get_user_file_count ( );

    virtual int del (
                      const char * user_key,
                      size_t user_key_len,
                      uint32_t & version,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      )
    {
        return 0;
    }

    virtual int put (
                      const char * user_key,
                      size_t use_key_len,
                      const char * val,
                      size_t val_len,
                      uint32_t & version,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      )
    {
        return 0;
    }

    virtual int get (
                      const char * user_key,
                      size_t user_key_len,
                      uint32_t & version,
                      conn_ctxt_t conn,
                      std::string * & rsp,
                      item_ctxt_t * & ctxt
                      )
    {
        return 0;
    }

    virtual int exists (
                         const char * user_key,
                         size_t user_key_len,
                         uint32_t & version,
                         conn_ctxt_t conn,
                         item_ctxt_t * & ctxt
                         )
    {
        return 0;
    }

    virtual void hash (
                        const char * user_key,
                        size_t user_key_len,
                        conn_ctxt_t conn,
                        item_ctxt_t * & ctxt
                        )
    {}

    void info (
                std::stringstream & ss
                );

    i_kv_t * get_file (
                        unsigned int file_id
                        );

    unsigned int file_count ( );

    virtual int flush ( );

    int hash_with_md5db (
                          const char * key,
                          size_t key_len,
                          conn_ctxt_t conn,
                          item_ctxt_t * & ctxt
                          );

    int del_from_md5db (
                         const md5db::block_id_t & block_id,
                         uint32_t file_id,
                         const char * table,
                         size_t table_len,
                         item_ctxt_t * & ctxt
                         );

    int del_from_binlog (
                          const char * key,
                          size_t key_len
                          );

    int put_from_md5db (
                         const md5db::block_id_t & block_id,
                         uint32_t file_id,
                         const char * table,
                         size_t table_len,
                         const char * val,
                         size_t val_len,
                         uint32_t ttl,
                         item_ctxt_t * & ctxt
                         );

    int put_from_binlog (
                          const char * key,
                          size_t key_len,
                          const char * val,
                          size_t val_len
                          );

    int get_from_md5db (
                         const md5db::block_id_t & block_id,
                         uint32_t file_id,
                         const char * table,
                         size_t table_len,
                         std::string * & rsp,
                         item_ctxt_t * & ctxt
                         );

    virtual int export_db (
                            int file_id,
                            const char * path,
                            export_record_callback_t callback,
                            void * callback_param
                            );

    virtual int export_db_mem (
                                conn_ctxt_t conn,
                                std::string * & rsp,
                                item_ctxt_t * & ctxt,
                                export_record_callback_t callback,
                                void * callback_param
                                );

    virtual int ttl_scan (
                           export_record_callback_t callback,
                           void * callback_param
                           );

    virtual int binlog_scan (
                              binlog_callback_t task_cb,
                              binlog_callback_t alive_cb,
                              void * alive_cb_param,
                              export_record_callback_t export_cb,
                              void * export_cb_param
                              );

    virtual int binlog (
                         const char * user_key,
                         size_t user_key_len,
                         const char * host,
                         size_t host_len,
                         uint32_t timestamp,
                         uint8_t cmd_type,
                         bool is_rem,
                         conn_ctxt_t conn
                         )
    {
        return 0;
    }

    virtual int hash_info (
                            int user_file_id,
                            int & inner_file_id
                            );

    virtual void set_inner_ttl (
                                 uint32_t ttl,
                                 conn_ctxt_t conn
                                 )
    {
    }

    virtual uint32_t get_inner_ttl (
                                     conn_ctxt_t conn
                                     )
    {
        return 0;
    }

    virtual void set_inner_table (
                                   const char * table,
                                   size_t table_len,
                                   uint8_t type,
                                   conn_ctxt_t conn,
                                   const char * added = NULL
                                   )
    {
    }

    virtual const char * get_inner_tbkey (
                                           const char * key,
                                           size_t & key_len,
                                           conn_ctxt_t conn
                                           )
    {
        return NULL;
    }

    static void * operator new(
                                size_t size
                                )
    {
        void * p = malloc ( size );
        if ( NULL == p )
        {
            throw std::bad_alloc ( );
        }
        return p;
    }

    static void operator delete(
                                 void * p
                                 )
    {
        free ( p );
    }

private:

    bool open (
                config_t & config
                );

    i_kv_t * create_file ( );

private:

    typedef std::vector< i_kv_t * > array_t;
    typedef std::vector< item_ctxt_t > get_buffers_t;

    bool          m_ok;
    int           m_file_count;
    std::string   m_ttl_seek;
    
    array_t       m_files;
    key_hash_t *  m_hash;
    config_t      m_config;
    get_buffers_t m_get_buffers;

private:
    // disable
    kv_array_t ( const kv_array_t & );
    const kv_array_t & operator= ( const kv_array_t & );
};

#endif // #ifndef _kv_array_h_
