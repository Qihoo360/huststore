#ifndef _hustdb_h_
#define _hustdb_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "atomic.h"
#include "base.h"
#include "i_server_kv.h"
#include "slow_task_thread.h"
#include "mdb/mdb.h"
#include <set>
#include <vector>

#define SIZEOF_UNIT32                 4
#define MAX_QUEUE_NAME_LEN            64
#define MAX_QKEY_LEN                  80
#define QUEUE_STAT_LEN                sizeof ( queue_stat_t )
#define QUEUE_INDEX_FILE_LEN          ( sizeof ( queue_stat_t ) * m_store_conf.mq_queue_maximum )

#define TABLE_STAT_LEN                sizeof ( table_stat_t )
#define TABLE_INDEX_FILE_LEN          ( sizeof ( table_stat_t ) * m_store_conf.db_table_maximum )

#define MIN_WORKER_LEN                4
#define MAX_WORKER_LEN                32
#define WORKER_TIMEOUT                512

#define DEF_REDELIVERY_TIMEOUT        5

#define DEF_MSG_TTL                   900
#define MAX_MSG_TTL                   7200
#define MAX_KV_TTL                    2592000

#define MAX_QUEUE_NUM                 8192
#define MAX_TABLE_NUM                 8192

#define MAX_QUEUE_ITEM_NUM            5000000
#define CYCLE_QUEUE_ITEM_NUM          11000000

#define MAX_EXPORT_OFFSET             100000000
#define SYNC_EXPORT_OFFSET            1000000
#define MEM_EXPORT_SIZE               1000
#define DISK_EXPORT_SIZE              1000000

#define ZSET_SCORE_LEN                21

#define CHECK_STRING(key)             ( ! key || key##_len <= 0 )
#define CHECK_VERSION                 ( ver < 0 || ver >= 0xFFFFFFFF )

enum table_type_t
{
    QUEUE_TB    = 0,
    PUSHQ_TB    = 1,
    KV_ALL      = 2,
    ZSET_IN     = 3,
    COMMON_TB   = 4,
    HASH_TB     = 33,
    SET_TB      = 35,
    ZSET_TB     = 124
};

typedef struct queue_stat_s
{
    uint32_t    sp;
    uint32_t    ep;
    uint32_t    sp1;
    uint32_t    ep1;
    uint32_t    sp2;
    uint32_t    ep2;

    uint32_t    flag : 1;
    uint32_t    lock : 1;
    uint32_t    type : 8;
    uint32_t    timeout : 8;
    uint32_t    _reserved : 14;
    uint32_t    max;
    uint32_t    ctime;

    char        qname [ MAX_QUEUE_NAME_LEN ];
} queue_stat_t;

typedef std::map < std::string, uint32_t >      worker_t;
typedef std::map < std::string, uint32_t >      unacked_t;
typedef std::multimap < uint32_t, std::string > redelivery_t;

typedef struct queue_info_s
{
    uint32_t          offset;
    unacked_t *       unacked;
    redelivery_t *    redelivery;
    worker_t *        worker;
} queue_info_t;

typedef std::map < std::string, queue_info_t > queue_map_t;

typedef struct table_stat_s
{
    atomic_t size;

    uint32_t flag : 1;
    uint32_t type : 8;
    uint32_t _reserved : 23;

    char table [ MAX_QUEUE_NAME_LEN + 2 ];
} table_stat_t;

typedef std::map < std::string, uint32_t > table_map_t;

typedef std::vector< lockable_t * > locker_vec_t;

typedef struct server_conf_s
{
    int32_t tcp_port;
    int32_t tcp_backlog;
    bool disable_100_cont;
    bool tcp_enable_reuseport;
    bool tcp_enable_nodelay;
    bool tcp_enable_defer_accept;
    int32_t tcp_max_body_size;
    int32_t tcp_max_keepalive_requests;
    int32_t tcp_recv_timeout;
    int32_t tcp_send_timeout;
    int32_t tcp_worker_count;
    std::string http_security_user;
    std::string http_security_passwd;
    std::string http_access_allow;

    server_conf_s ( )
    : tcp_port ( 0 )
    , tcp_backlog ( 0 )
    , disable_100_cont ( true )
    , tcp_enable_reuseport ( true )
    , tcp_enable_nodelay ( true )
    , tcp_enable_defer_accept ( true )
    , tcp_max_body_size ( 0 )
    , tcp_max_keepalive_requests ( 0 )
    , tcp_recv_timeout ( 0 )
    , tcp_send_timeout ( 0 )
    , tcp_worker_count ( 0 )
    , http_security_user ( )
    , http_security_passwd ( )
    , http_access_allow ( )
    {
    }

} server_conf_t;

typedef struct store_conf_s
{
    int32_t mq_redelivery_timeout;
    int32_t mq_ttl_maximum;
    int32_t db_ttl_maximum;
    int32_t mq_queue_maximum;
    int32_t db_table_maximum;
    
    store_conf_s ( )
    : mq_redelivery_timeout ( 0 )
    , mq_ttl_maximum ( 0 )
    , db_ttl_maximum ( 0 )
    , mq_queue_maximum ( 0 )
    , db_table_maximum ( 0 )
    {
    }

} store_conf_t;

class hustdb_t
{
public:

    void kill_me ( );
    bool open ( );

    bool ok ( )
    {
        return m_storage_ok;
    }

    bool mdb_ok ( )
    {
        return m_mdb_ok;
    }

    i_server_kv_t * get_storage ( )
    {
        return m_storage;
    }

    int32_t get_worker_count ( )
    {
        return m_server_conf.tcp_worker_count;
    }

    server_conf_t get_server_conf ( )
    {
        return m_server_conf;
    }

public:

    hustdb_t ( );
    ~hustdb_t ( );

    void destroy ( );

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

public:

    void slow_task_info (
                          std::string & info
                          );

    slow_task_type_t slow_task_status (
                                        void * task
                                        );

    void hustdb_info (
                       std::string & info
                       );

    int hustdb_file_count ( );

    int hustdb_exist (
                       const char * key,
                       size_t key_len,
                       uint32_t & ver,
                       conn_ctxt_t conn,
                       item_ctxt_t * & ctxt
                       );

    int hustdb_get (
                     const char * key,
                     size_t key_len,
                     std::string * & rsp,
                     int & rsp_len,
                     uint32_t & ver,
                     conn_ctxt_t conn,
                     item_ctxt_t * & ctxt
                     );

    int hustdb_put (
                     const char * key,
                     size_t key_len,
                     const char * val,
                     size_t val_len,
                     uint32_t & ver,
                     uint32_t ttl,
                     bool is_dup,
                     conn_ctxt_t conn,
                     item_ctxt_t * & ctxt
                     );

    int hustdb_del (
                     const char * key,
                     size_t key_len,
                     uint32_t & ver,
                     bool is_dup,
                     conn_ctxt_t conn,
                     item_ctxt_t * & ctxt
                     );

    int hustdb_keys (
                      int offset,
                      int size,
                      int file_id,
                      int start,
                      int end,
                      bool async,
                      bool noval,
                      uint32_t & hits,
                      uint32_t & total,
                      std::string * & rsp,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_stat (
                      const char * table,
                      size_t table_len,
                      int & count
                      );

    void hustdb_stat_all (
                           std::string & stats
                           );

    int hustdb_export (
                        const char * table,
                        size_t table_len,
                        int offset,
                        int size,
                        int file_id,
                        int start,
                        int end,
                        bool cover,
                        bool noval,
                        void * & token
                        );

    int hustdb_sweep (
                       const char * table,
                       size_t table_len
                       );

public:

    int hustdb_hexist (
                        const char * table,
                        size_t table_len,
                        const char * key,
                        size_t key_len,
                        uint32_t & ver,
                        conn_ctxt_t conn,
                        item_ctxt_t * & ctxt
                        );

    int hustdb_hget (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      std::string * & rsp,
                      int & rsp_len,
                      uint32_t & ver,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_hset (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      const char * val,
                      size_t val_len,
                      uint32_t & ver,
                      uint32_t ttl,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_hdel (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      uint32_t & ver,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_hkeys (
                       const char * table,
                       size_t table_len,
                       int offset,
                       int size,
                       int start,
                       int end,
                       bool async,
                       bool noval,
                       uint32_t & hits,
                       uint32_t & total,
                       std::string * & rsp,
                       conn_ctxt_t conn,
                       item_ctxt_t * & ctxt
                       );

public:

    int hustdb_sismember (
                           const char * table,
                           size_t table_len,
                           const char * key,
                           size_t key_len,
                           uint32_t & ver,
                           conn_ctxt_t conn,
                           item_ctxt_t * & ctxt
                           );

    int hustdb_sadd (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      uint32_t & ver,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_srem (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      uint32_t & ver,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_smembers (
                          const char * table,
                          size_t table_len,
                          int offset,
                          int size,
                          int start,
                          int end,
                          bool async,
                          bool noval,
                          uint32_t & hits,
                          uint32_t & total,
                          std::string * & rsp,
                          conn_ctxt_t conn,
                          item_ctxt_t * & ctxt
                          );

public:

    int hustdb_zismember (
                           const char * table,
                           size_t table_len,
                           const char * key,
                           size_t key_len,
                           uint32_t & ver,
                           conn_ctxt_t conn,
                           item_ctxt_t * & ctxt
                           );

    int hustdb_zadd (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      uint64_t score,
                      int opt,
                      uint32_t & ver,
                      bool is_dup,
                      conn_ctxt_t conn,
                      bool & is_version_error
                      );

    int hustdb_zscore (
                        const char * table,
                        size_t table_len,
                        const char * key,
                        size_t key_len,
                        std::string * & rsp,
                        int & rsp_len,
                        uint32_t & ver,
                        conn_ctxt_t conn,
                        item_ctxt_t * & ctxt
                        );

    int hustdb_zrem (
                      const char * table,
                      size_t table_len,
                      const char * key,
                      size_t key_len,
                      uint32_t & ver,
                      bool is_dup,
                      conn_ctxt_t conn,
                      item_ctxt_t * & ctxt
                      );

    int hustdb_zrange (
                        const char * table,
                        size_t table_len,
                        uint64_t min,
                        uint64_t max,
                        int offset,
                        int size,
                        int start,
                        int end,
                        bool async,
                        bool noval,
                        bool byscore,
                        uint32_t & hits,
                        uint32_t & total,
                        std::string * & rsp,
                        conn_ctxt_t conn,
                        item_ctxt_t * & ctxt
                        );
    
public:

    int hustmq_put (
                     const char * queue,
                     size_t queue_len,
                     const char * item,
                     size_t item_len,
                     uint32_t priori,
                     conn_ctxt_t conn
                     );

    int hustmq_get (
                     const char * queue,
                     size_t queue_len,
                     const char * worker,
                     size_t worker_len,
                     bool is_ack,
                     std::string & ack,
                     std::string & unacked,
                     std::string * & rsp,
                     conn_ctxt_t conn
                     );

    int hustmq_ack_inner (
                           std::string & ack,
                           conn_ctxt_t conn
                           );

    int hustmq_ack (
                     const char * queue,
                     size_t queue_len,
                     const char * ack,
                     size_t ack_len,
                     conn_ctxt_t conn
                     );
    
    int hustmq_worker (
                        const char * queue,
                        size_t queue_len,
                        std::string & workers
                        );

    int hustmq_stat (
                      const char * queue,
                      size_t queue_len,
                      std::string & stat
                      );

    void hustmq_stat_all (
                           std::string & stats
                           );

    int hustmq_max (
                     const char * queue,
                     size_t queue_len,
                     uint32_t max
                     );

    int hustmq_lock (
                      const char * queue,
                      size_t queue_len,
                      uint8_t lock
                      );

    int hustmq_timeout (
                         const char * queue,
                         size_t queue_len,
                         uint8_t timeout
                         );

    int hustmq_purge (
                       const char * queue,
                       size_t queue_len,
                       uint32_t priori,
                       conn_ctxt_t conn
                       );

    int hustmq_pub (
                     const char * queue,
                     size_t queue_len,
                     const char * item,
                     size_t item_len,
                     uint32_t idx,
                     uint32_t wttl,
                     conn_ctxt_t conn
                     );

    int hustmq_sub (
                     const char * queue,
                     size_t queue_len,
                     uint32_t idx,
                     uint32_t & sp,
                     uint32_t & ep,
                     std::string * & rsp,
                     conn_ctxt_t conn
                     );

public:

    static
    const char * errno_string_status (
                                       int r
                                       );

    static
    int errno_int_status (
                           int r
                           );
    
    static
    char real_table_type (
                           uint8_t type
                           );

private:

    bool init_server_config ( );

    bool init_hash_config ( );

    bool init_data_engine ( );
    
    bool init_queue_index ( );
    
    bool init_table_index ( );

    bool generate_hash_conf (
                              int file_count,
                              int copy_count,
                              std::string & hash_conf
                              );

    bool tb_name_check (
                       const char * qname,
                       const int qname_len
                       );

    int find_table_type (
                          std::string & table,
                          uint8_t & type
                          );
    
    int find_table_offset (
                            std::string & table,
                            bool create,
                            uint8_t type
                            );

    void set_table_size (
                          int offset,
                          int atomic
                          );

    int get_table_size (
                         const char * table,
                         size_t table_len
                         );

    uint32_t clac_real_item (
                              const uint32_t start,
                              const uint32_t end
                              );

    int find_queue_offset (
                            std::string & queue,
                            bool create,
                            uint8_t type,
                            queue_info_t * & queue_info,
                            const char * worker = NULL,
                            size_t worker_len = 0
                            );
    
private:

    ini_t * m_ini;

    apptool_t * m_apptool;
    appini_t * m_appini;

    i_server_kv_t * m_storage;
    bool m_storage_ok;

    slow_task_thread_t m_slow_tasks;

    mdb_t * m_mdb;
    bool m_mdb_ok;

    fmap_t m_queue_index;
    queue_map_t m_queue_map;
    lockable_t m_mq_locker;
    
    locker_vec_t m_lockers;

    fmap_t m_table_index;
    table_map_t m_table_map;

    lockable_t m_tb_locker;

    server_conf_t m_server_conf;
    store_conf_t m_store_conf;

private:
    // disable
    hustdb_t ( const hustdb_t & );
    const hustdb_t & operator= ( const hustdb_t & );
};

#endif
