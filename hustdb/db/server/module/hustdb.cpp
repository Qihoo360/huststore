#include "hustdb.h"
#include "tasks/task_export.h"
#include "tasks/task_ttl_scan.h"
#include "tasks/task_binlog_scan.h"
#include "kv/kv_array/key_hash.h"
#include "perf_target.h"
#include "kv/md5db/bucket.h"
#include <sstream>

void hustdb_t::kill_me ( )
{
    delete this;
}

hustdb_t::hustdb_t ( )
: m_ini ( NULL )
, m_apptool ( NULL )
, m_appini ( NULL )
, m_current_timestamp ( 0 )
, m_over_threshold ( false )
, m_storage ( NULL )
, m_storage_ok ( false )
, m_mdb ( NULL )
, m_mdb_ok ( false )
, m_rdb ( NULL )
, m_rdb_ok ( false )
, m_timer ( )
, m_slow_tasks ( )
, m_mq_locker ( )
, m_tb_locker ( )
, m_server_conf ( )
, m_store_conf ( )
{
    G_APPTOOL->fmap_init ( & m_queue_index );
    G_APPTOOL->fmap_init ( & m_table_index );
}

hustdb_t::~ hustdb_t ( )
{
    destroy ();
}

void hustdb_t::destroy ( )
{
    if ( m_ini )
    {
        m_appini->ini_destroy ( m_ini );
        LOG_INFO ( "[hustdb][destroy]hustdb.conf closed" );
    }

    LOG_INFO ( "[hustdb][destroy]slow task thread closing" );
    m_slow_tasks.stop ();
    LOG_INFO ( "[hustdb][destroy]slow task thread closed" );

    LOG_INFO ( "[hustdb][destroy]timer closing" );
    m_timer.kill_me ();
    LOG_INFO ( "[hustdb][destroy]timer closed" );

    if ( m_storage )
    {
        m_storage_ok = false;
        LOG_INFO ( "[hustdb][destroy]storage closing" );
        m_storage->kill_me ();
        LOG_INFO ( "[hustdb][destroy]storage closed" );
        m_storage = NULL;
    }

    if ( m_mdb )
    {
        m_mdb_ok = false;
        LOG_INFO ( "[hustdb][destroy]mdb closing" );
        m_mdb->kill_me ();
        LOG_INFO ( "[hustdb][destroy]mdb closed" );
        m_mdb = NULL;
    }

    if ( m_rdb )
    {
        m_rdb_ok = false;
        LOG_INFO ( "[hustdb][destroy]rdb closing" );
        m_rdb->kill_me ();
        LOG_INFO ( "[hustdb][destroy]rdb closed" );
        m_rdb = NULL;
    }

    if ( m_apptool )
    {
        apptool_t::kill_me ();
        LOG_INFO ( "[hustdb][destroy]apptool closed" );
        m_apptool = NULL;
    }

    if ( m_appini )
    {
        appini_t::kill_me ();
        LOG_INFO ( "[hustdb][destroy]apptool closed" );
        m_appini = NULL;
    }

    for ( locker_vec_t::iterator it = m_lockers.begin (); it != m_lockers.end (); ++ it )
    {
        delete ( * it );
    }

    for ( queue_map_t::iterator it = m_queue_map.begin (); it != m_queue_map.end (); it ++ )
    {
        delete it->second.unacked;
        it->second.unacked = NULL;

        delete it->second.redelivery;
        it->second.redelivery = NULL;

        delete it->second.worker;
        it->second.worker = NULL;
    }

    G_APPTOOL->fmap_close ( & m_queue_index );
    G_APPTOOL->fmap_close ( & m_table_index );
}

bool hustdb_t::open ( )
{
    m_apptool = apptool_t::get_apptool ();
    if ( NULL == m_apptool )
    {
        LOG_ERROR ( "[hustdb][open]get_apptool failed" );
        return false;
    }

    m_apptool->set_hustdb ( this );

    if ( m_apptool->log_open ( HUSTDB_LOG_DIR, NULL, false ) != 0 )
    {
        LOG_ERROR ( "[hustdb][open]log_open failed" );
        return false;
    }

    m_appini = appini_t::get_appini ();
    if ( NULL == m_appini )
    {
        LOG_ERROR ( "[hustdb][open]get_appini failed" );
        return false;
    }

    m_ini = m_appini->ini_create ( HUSTDB_CONFIG );
    if ( NULL == m_ini )
    {
        LOG_ERROR ( "[hustdb][open]hustdb.conf load failed" );
        return false;
    }

    if ( ! init_server_config () )
    {
        LOG_ERROR ( "[hustdb][open]init_server_config() failed" );
        return false;
    }

    m_apptool->make_dir ( DB_DATA_ROOT );
    if ( ! m_apptool->is_dir ( DB_DATA_ROOT ) )
    {
        LOG_ERROR ( "[hustdb][open]./DATA create failed" );
        return false;
    }

    m_apptool->make_dir ( DB_DATA_DIR );
    if ( ! m_apptool->is_dir ( DB_DATA_DIR ) )
    {
        LOG_ERROR ( "[hustdb][open]./DATA/DATA create failed" );
        return false;
    }
    
    if ( ! check_invariant_config () )
    {
        LOG_ERROR ( "[hustdb][open]check_invariant_config() failed" );
        return false;
    }

    if ( ! init_hash_config () )
    {
        LOG_ERROR ( "[hustdb][open]init_config failed" );
        return false;
    }

    m_apptool->make_dir ( HUSTDB_EXPORT_DIR );
    if ( ! m_apptool->is_dir ( HUSTDB_EXPORT_DIR ) )
    {
        LOG_ERROR ( "[hustdb][open]./EXPORT create failed" );
        return false;
    }

    if ( ! init_data_engine () )
    {
        LOG_ERROR ( "[hustdb][open]init_data_engine() failed" );
        return false;
    }

    if ( ! init_queue_index () )
    {
        LOG_ERROR ( "[hustdb][open]init_queue_index() failed" );
        return false;
    }

    if ( ! init_table_index () )
    {
        LOG_ERROR ( "[hustdb][open]init_table_index() failed" );
        return false;
    }
    
    if ( ! m_slow_tasks.start () )
    {
        LOG_ERROR ( "[hustdb][open]slow_tasks start failed" );
        return false;
    }

    if ( ! ( m_timer.register_task ( hustdb::timer_task_t ( 1, timestamp_cb, this ) ) &&
             m_timer.register_task ( hustdb::timer_task_t ( 15, over_threshold_cb, this ) ) &&
             m_timer.register_task ( hustdb::timer_task_t ( m_store_conf.db_ttl_scan_interval, ttl_scan_cb, this ) ) &&
             m_timer.register_task ( hustdb::timer_task_t ( m_store_conf.db_binlog_scan_interval, binlog_scan_cb, this ) ) &&
             m_timer.open ( )
             )
         )
    {
        LOG_ERROR ( "[hustdb][open]timer register task failed" );
        return false;
    }

    try
    {
        for ( int i = 0; i < MAX_LOCKER_NUM; i ++ )
        {
            lockable_t * l = new lockable_t ();
            m_lockers.push_back ( l );
        }
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hustdb][open]hustdb new locker failed" );
        return false;
    }

    m_storage_ok = true;

    return true;
}

bool hustdb_t::init_server_config ( )
{
    if ( ! m_ini )
    {
        LOG_ERROR ( "[hustdb][init_server_config]hustdb config not ready" );
        return false;
    }

    m_server_conf.tcp_port = m_appini->ini_get_int ( m_ini, "server", "tcp.port", - 1 );
    if ( m_server_conf.tcp_port <= 0 || m_server_conf.tcp_port >= 65535 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.port invalid, port: %d", m_server_conf.tcp_port );
        return false;
    }

    m_server_conf.tcp_backlog = m_appini->ini_get_int ( m_ini, "server", "tcp.backlog", 512 );
    if ( m_server_conf.tcp_backlog <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.backlog invalid, backlog: %d", m_server_conf.tcp_backlog );
        return false;
    }

    m_server_conf.tcp_enable_reuseport = m_appini->ini_get_bool ( m_ini, "server", "tcp.enable_reuseport", true );

    m_server_conf.tcp_enable_nodelay = m_appini->ini_get_bool ( m_ini, "server", "tcp.enable_nodelay", true );

    m_server_conf.tcp_enable_defer_accept = m_appini->ini_get_bool ( m_ini, "server", "tcp.enable_defer_accept", true );

    m_server_conf.tcp_max_body_size = m_appini->ini_get_int ( m_ini, "server", "tcp.max_body_size", 16 * 1024 * 1024 );
    if ( m_server_conf.tcp_max_body_size <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.max_body_size invalid, max_body_size: %d", m_server_conf.tcp_max_body_size );
        return false;
    }

    m_server_conf.tcp_max_keepalive_requests = m_appini->ini_get_int ( m_ini, "server", "tcp.max_keepalive_requests", 1024 );
    if ( m_server_conf.tcp_max_keepalive_requests <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.max_keepalive_requests invalid, max_keepalive_requests: %d", m_server_conf.tcp_max_keepalive_requests );
        return false;
    }

    m_server_conf.tcp_recv_timeout = m_appini->ini_get_int ( m_ini, "server", "tcp.recv_timeout", 300 );
    if ( m_server_conf.tcp_recv_timeout <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.recv_timeout invalid, recv_timeout: %d", m_server_conf.tcp_recv_timeout );
        return false;
    }

    m_server_conf.tcp_send_timeout = m_appini->ini_get_int ( m_ini, "server", "tcp.send_timeout", 300 );
    if ( m_server_conf.tcp_send_timeout <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.send_timeout invalid, send_timeout: %d", m_server_conf.tcp_send_timeout );
        return false;
    }

    m_server_conf.tcp_worker_count  = m_appini->ini_get_int ( m_ini, "server", "tcp.worker_count", - 1 );
    if ( m_server_conf.tcp_worker_count <= 0 || m_server_conf.tcp_worker_count >= 100 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server tcp.worker_count invalid, worker_count: %d", m_server_conf.tcp_worker_count );
        return false;
    }

    const char * s = NULL;

    s = m_appini->ini_get_string ( m_ini, "server", "http.security.user", "" );
    m_server_conf.http_security_user = s ? s : "";

    s = m_appini->ini_get_string ( m_ini, "server", "http.security.passwd", "" );
    m_server_conf.http_security_passwd = s ? s : "";

    s = m_appini->ini_get_string ( m_ini, "server", "http.access.allow", "" );
    m_server_conf.http_access_allow = s ? s : "";

    m_server_conf.memory_process_threshold  = m_appini->ini_get_int ( m_ini, "server", "memory.process.threshold", 70 );
    if ( m_server_conf.memory_process_threshold < 0 || m_server_conf.memory_process_threshold >= 100 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server memory.process.threshold invalid, process.threshold: %d", m_server_conf.memory_process_threshold );
        return false;
    }

    m_server_conf.memory_system_threshold  = m_appini->ini_get_int ( m_ini, "server", "memory.system.threshold", 90 );
    if ( m_server_conf.memory_system_threshold < 0 || m_server_conf.memory_system_threshold >= 100 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]server memory.system.threshold invalid, system.threshold: %d", m_server_conf.memory_system_threshold );
        return false;
    }

    m_store_conf.mq_redelivery_timeout = m_appini->ini_get_int ( m_ini, "store", "mq.redelivery.timeout", 5 );
    if ( m_store_conf.mq_redelivery_timeout <= 0 || m_store_conf.mq_redelivery_timeout > 255 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store mq.redelivery.timeout invalid, redelivery.timeout: %d", m_store_conf.mq_redelivery_timeout );
        return false;
    }

    m_store_conf.mq_ttl_maximum = m_appini->ini_get_int ( m_ini, "store", "mq.ttl.maximum", 7200 );
    if ( m_store_conf.mq_ttl_maximum <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store mq.ttl.maximum invalid, ttl.maximum: %d", m_store_conf.mq_ttl_maximum );
        return false;
    }

    m_store_conf.db_ttl_maximum = m_appini->ini_get_int ( m_ini, "store", "db.ttl.maximum", 2592000 );
    if ( m_store_conf.db_ttl_maximum <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.ttl.maximum invalid, ttl.maximum: %d", m_store_conf.db_ttl_maximum );
        return false;
    }

    m_store_conf.db_ttl_scan_interval = m_appini->ini_get_int ( m_ini, "store", "db.ttl.scan_interval", 30 );
    if ( m_store_conf.db_ttl_scan_interval <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.ttl.scan_interval invalid, ttl.scan_interval: %d", m_store_conf.db_ttl_scan_interval );
        return false;
    }
    
    m_store_conf.db_binlog_scan_interval = m_appini->ini_get_int ( m_ini, "store", "db.binlog.scan_interval", 20 );
    if ( m_store_conf.db_binlog_scan_interval <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.binlog.scan_interval invalid, binlog.scan_interval: %d", m_store_conf.db_binlog_scan_interval );
        return false;
    }
    
    m_store_conf.db_binlog_thread_count = m_appini->ini_get_int ( m_ini, "store", "db.binlog.thread_count", 4 );
    if ( m_store_conf.db_binlog_thread_count <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.binlog.thread_count invalid, binlog.thread_count: %d", m_store_conf.db_binlog_thread_count );
        return false;
    }
    
    m_store_conf.db_binlog_queue_capacity = m_appini->ini_get_int ( m_ini, "store", "db.binlog.queue_capacity", 2000 );
    if ( m_store_conf.db_binlog_queue_capacity <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.binlog.queue_capacity invalid, binlog.queue_capacity: %d", m_store_conf.db_binlog_queue_capacity );
        return false;
    }
    
    m_store_conf.db_binlog_task_timeout = m_appini->ini_get_int ( m_ini, "store", "db.binlog.task_timeout", 950400 );
    if ( m_store_conf.db_binlog_task_timeout <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.binlog.task_timeout invalid, binlog.task_timeout: %d", m_store_conf.db_binlog_task_timeout );
        return false;
    }

    m_store_conf.mq_queue_maximum = m_appini->ini_get_int ( m_ini, "store", "mq.queue.maximum", 8192 );
    if ( m_store_conf.mq_queue_maximum <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store mq.queue.maximum invalid, queue.maximum: %d", m_store_conf.mq_queue_maximum );
        return false;
    }

    m_store_conf.db_table_maximum = m_appini->ini_get_int ( m_ini, "store", "db.table.maximum", 8192 );
    if ( m_store_conf.db_table_maximum <= 0 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.table.maximum invalid, table.maximum: %d", m_store_conf.db_table_maximum );
        return false;
    }

    m_store_conf.db_ttl_scan_count = m_appini->ini_get_int ( m_ini, "store", "db.ttl.scan_count", 1000 );
    if ( m_store_conf.db_ttl_scan_count <= 0 || m_store_conf.db_ttl_scan_count > 10000 )
    {
        LOG_ERROR ( "[hustdb][init_server_config]store db.ttl.scan_count invalid, ttl.scan_count: %d", m_store_conf.db_ttl_scan_count );
        return false;
    }

    return true;
}

bool hustdb_t::check_invariant_config ( )
{
    bool first     = false;
    char ph[ 260 ] = { };
    fast_memcpy ( ph, HUSTSTORE_INVARIANT, sizeof ( HUSTSTORE_INVARIANT ) );

    if ( ! m_apptool->is_file ( ph ) )
    {
        first = true;
        
        FILE * fp = fopen ( ph, "wb" );
        if ( ! fp )
        {
            LOG_ERROR ( "[hustdb][check_invariant_config]invariant fopen %s failed", ph );
            return false;
        }

        bool ok = true;
        char buf[ sizeof ( invariant_t ) ];
        memset ( buf, 0, sizeof ( buf ) );
        if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
        {
            LOG_ERROR ( "[hustdb][check_invariant_config]invariant fwrite[%d] failed", sizeof ( buf ) );
            ok = false;
        }

        fclose ( fp );
        if ( ! ok )
        {
            remove ( ph );
            return false;
        }
    }
    
    int r = 0;
    G_APPTOOL->fmap_init ( & m_invariant );
    
    do  
    {
        if ( ! G_APPTOOL->fmap_open ( & m_invariant, ph, 0, 0, true ) )
        {
            r = EINVAL;
            LOG_ERROR ( "[hustdb][check_invariant_config]invariant fmap_open failed" );
            break;
        }

        if ( m_invariant.ptr_len != sizeof ( invariant_t ) )
        {
            r = EINVAL;
            LOG_ERROR ( "[hustdb][check_invariant_config]invariant ptr_len: %d, != must_len: %d, error", m_invariant.ptr_len, sizeof ( invariant_t ) );
            break;
        }

        invariant_t * invar = NULL;
        invar = ( invariant_t * ) m_invariant.ptr;

        int fastdb_count             = m_appini->ini_get_int ( m_ini, "fastdb", "count", 0 );
        int conflictdb_count         = m_appini->ini_get_int ( m_ini, "conflictdb", "count", 0 );
        int contentdb_count          = m_appini->ini_get_int ( m_ini, "contentdb", "count", 0 );
        int fast_conflictdb_count    = m_appini->ini_get_int ( m_ini, "fast_conflictdb", "count", 0 );

        if ( ! first &&
             (
               invar->fastdb != fastdb_count ||
               invar->conflictdb != conflictdb_count ||
               invar->contentdb != contentdb_count ||
               invar->fast_conflictdb != fast_conflictdb_count
               )
             )
        {
            r = EINVAL;
            LOG_ERROR ( "[hustdb][check_invariant_config]invariant error, fastdb:[%d,%d], conflictdb:[%d,%d], contentdb:[%d,%d], fast_conflictdb:[%d,%d]",
                       invar->fastdb, fastdb_count,
                       invar->conflictdb, conflictdb_count,
                       invar->contentdb, contentdb_count,
                       invar->fast_conflictdb, fast_conflictdb_count
                       );
            break;
        }
        
        invar->fastdb             = fastdb_count;
        invar->conflictdb         = conflictdb_count;
        invar->contentdb          = contentdb_count;
        invar->fast_conflictdb    = fast_conflictdb_count;
        
    }
    while ( 0 );
    
    G_APPTOOL->fmap_close ( & m_invariant );
    
    return ( r == 0 );
}

bool hustdb_t::init_hash_config ( )
{
    int            fastdb_count;
    std::string    hash_conf_content;
    FILE *         fp_hash_conf;

    if ( m_apptool->is_file ( DB_HASH_CONFIG )
         && m_apptool->is_file ( HUSTMQ_INDEX )
         && m_apptool->is_file ( HUSTTABLE_INDEX )
         && m_apptool->is_dir ( DB_FULLKEY_DIR )
         && m_apptool->is_dir ( DB_BUCKETS_DIR )
         && m_apptool->is_dir ( DB_CONFLICT_DIR )
         && m_apptool->is_dir ( DB_FAST_CONFLICT_DIR ) )
    {
        LOG_INFO ( "[hustdb][init_hash_config]data config has been created" );
        return true;
    }

    fastdb_count = m_appini->ini_get_int ( m_ini, "fastdb", "count", 10 );
    if ( fastdb_count <= 0 || fastdb_count > 20 )
    {
        LOG_ERROR ( "[hustdb][init_hash_config]storage fastdb count invalid" );
        return false;
    }

    do
    {
        {
            bool b = generate_hash_conf ( fastdb_count, 1, hash_conf_content );
            if ( ! b )
            {
                LOG_ERROR ( "[hustdb][init_hash_config]generate_hash_conf() failed" );
                break;
            }

            fp_hash_conf = fopen ( DB_HASH_CONFIG, "wb" );
            if ( NULL == fp_hash_conf )
            {
                LOG_ERROR ( "[hustdb][init_hash_config]fopen( %s ) failed", DB_HASH_CONFIG );
                break;
            }
        }

        if ( fp_hash_conf )
        {
            fputs ( hash_conf_content.c_str (), fp_hash_conf );
            fclose ( fp_hash_conf );
            fp_hash_conf = NULL;
        }

        LOG_INFO ( "[hustdb][init_hash_config]create config ok" );

        return true;
    }
    while ( 0 );

    if ( fp_hash_conf )
    {
        fclose ( fp_hash_conf );
        fp_hash_conf = NULL;
    }

    return false;
}

bool hustdb_t::init_data_engine ( )
{
    if ( NULL == m_storage )
    {
        try
        {
            m_storage = create_server_kv ( );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hustdb][init_data_engine]bad_alloc" );
            return false;
        }
        LOG_INFO ( "[hustdb][init_data_engine]storage opening" );

        if ( ! m_storage->open () )
        {
            LOG_ERROR ( "[hustdb][init_data_engine]m_storage->open failed" );
            m_storage->kill_me ();
            m_storage = NULL;
            return false;
        }
        LOG_INFO ( "[hustdb][init_data_engine]storage opened" );
    }

    if ( NULL == m_mdb )
    {
        int mdb_cache_size = m_appini->ini_get_int ( m_ini, "fastdb", "l2_cache", 512 );

        m_mdb = new mdb_t ();

        if (
             ! m_mdb->open ( m_server_conf.tcp_worker_count + 1,
                            mdb_cache_size
                            )
             )
        {
            LOG_ERROR ( "[hustdb][init_data_engine]mdb->open failed" );
            m_mdb->kill_me ();
            m_mdb = NULL;
            return false;
        }

        m_mdb_ok = mdb_cache_size > 0 ? true : false;
    }

    if ( NULL == m_rdb )
    {
        int rdb_cache_size = m_appini->ini_get_int ( m_ini, "cachedb", "cache", 512 );

        m_rdb = new rdb_t ();

        if (
             ! m_rdb->open ( m_server_conf.tcp_worker_count + 1,
                            rdb_cache_size
                            )
             )
        {
            LOG_ERROR ( "[hustdb][init_data_engine]rdb->open failed" );
            m_rdb->kill_me ();
            m_rdb = NULL;
            return false;
        }

        m_rdb_ok = rdb_cache_size > 0 ? true : false;
    }

    return true;
}

bool hustdb_t::init_queue_index ( )
{
    char ph[ 260 ] = { };
    fast_memcpy ( ph, HUSTMQ_INDEX, sizeof ( HUSTMQ_INDEX ) );

    if ( ! m_apptool->is_file ( ph ) )
    {
        FILE * fp = fopen ( ph, "wb" );
        if ( ! fp )
        {
            LOG_ERROR ( "[hustdb][init_queue_index]queue index fopen %s failed", ph );
            return false;
        }

        bool ok = true;
        char buf[ QUEUE_INDEX_FILE_LEN ];
        memset ( buf, 0, sizeof ( buf ) );
        if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
        {
            LOG_ERROR ( "[hustdb][init_queue_index]queue index fwrite[%d] failed", sizeof ( buf ) );
            ok = false;
        }

        fclose ( fp );
        if ( ! ok )
        {
            remove ( ph );
            return false;
        }
    }
    else
    {
        FILE * fp = fopen ( ph, "rb+" );
        if ( ! fp )
        {
            LOG_ERROR ( "[hustdb][init_queue_index]queue index fopen %s failed", ph );
            return false;
        }

        fseek ( fp, 0L, SEEK_END );
        uint64_t size = ftell ( fp );

        if ( size < QUEUE_INDEX_FILE_LEN )
        {
            char add_buf [ QUEUE_INDEX_FILE_LEN - size ];
            memset ( add_buf, 0, sizeof ( add_buf ) );

            if ( sizeof ( add_buf ) != fwrite ( add_buf, 1, sizeof ( add_buf ), fp ) )
            {
                LOG_ERROR ( "[hustdb][init_queue_index]queue index fwrite[%d] failed", sizeof ( add_buf ) );
                fclose ( fp );
                return false;
            }
        }

        fclose ( fp );
    }

    if ( ! G_APPTOOL->fmap_open ( & m_queue_index, ph, 0, 0, true ) )
    {
        LOG_ERROR ( "[hustdb][init_queue_index]queue index fmap_open failed" );
        return false;
    }

    if ( m_queue_index.ptr_len < QUEUE_INDEX_FILE_LEN ||
         m_queue_index.ptr_len % QUEUE_STAT_LEN != 0
         )
    {
        LOG_ERROR ( "[hustdb][init_queue_index]queue index ptr_len: %d, lt %d or mod %d != 0, error", m_queue_index.ptr_len, QUEUE_INDEX_FILE_LEN, QUEUE_STAT_LEN );
        return false;
    }

    queue_stat_t * qstat = NULL;
    for ( uint32_t i = 0; i < QUEUE_INDEX_FILE_LEN; i += QUEUE_STAT_LEN )
    {
        qstat = ( queue_stat_t * ) ( m_queue_index.ptr + i );

        if ( qstat->flag > 0 )
        {
            queue_info_t t;

            t.offset        = i;
            t.unacked       = new unacked_t ();
            t.redelivery    = new redelivery_t ();
            t.worker        = new worker_t ();

            m_queue_map.insert (std::pair<std::string, queue_info_t>( qstat->qname, t ));
        }
    }

    return true;
}

bool hustdb_t::init_table_index ( )
{
    char ph[ 260 ] = { };
    fast_memcpy ( ph, HUSTTABLE_INDEX, sizeof ( HUSTTABLE_INDEX ) );

    if ( ! m_apptool->is_file ( ph ) )
    {
        FILE * fp = fopen ( ph, "wb" );
        if ( ! fp )
        {
            LOG_ERROR ( "[hustdb][init_table_index]table index fopen %s failed", ph );
            return false;
        }

        bool ok = true;
        char buf[ TABLE_INDEX_FILE_LEN ];
        memset ( buf, 0, sizeof ( buf ) );
        if ( sizeof ( buf ) != fwrite ( buf, 1, sizeof ( buf ), fp ) )
        {
            ok = false;
        }

        fclose ( fp );
        if ( ! ok )
        {
            remove ( ph );
            return false;
        }
    }
    else
    {
        FILE * fp = fopen ( ph, "rb+" );
        if ( ! fp )
        {
            LOG_ERROR ( "[hustdb][init_table_index]table index fopen %s failed", ph );
            return false;
        }

        fseek ( fp, 0L, SEEK_END );
        uint64_t size = ftell ( fp );

        if ( size < TABLE_INDEX_FILE_LEN )
        {
            char add_buf [ TABLE_INDEX_FILE_LEN - size ];
            memset ( add_buf, 0, sizeof ( add_buf ) );

            if ( sizeof ( add_buf ) != fwrite ( add_buf, 1, sizeof ( add_buf ), fp ) )
            {
                LOG_ERROR ( "[hustdb][init_queue_index]table index fwrite[%d] failed", sizeof ( add_buf ) );
                fclose ( fp );
                return false;
            }
        }

        fclose ( fp );
    }

    if ( ! G_APPTOOL->fmap_open ( & m_table_index, ph, 0, 0, true ) )
    {
        LOG_ERROR ( "[hustdb][init_table_index]table index fmap_open failed" );
        return false;
    }

    if ( m_table_index.ptr_len < TABLE_INDEX_FILE_LEN ||
         m_table_index.ptr_len % TABLE_STAT_LEN != 0
         )
    {
        LOG_ERROR ( "[hustdb][init_table_index]table index ptr_len: %d, lt %d or mod %d != 0, error", m_table_index.ptr_len, TABLE_INDEX_FILE_LEN, TABLE_STAT_LEN );
        return false;
    }

    table_stat_t * tstat = NULL;
    for ( uint32_t i = TABLE_STAT_LEN; i < TABLE_INDEX_FILE_LEN; i += TABLE_STAT_LEN )
    {
        tstat = ( table_stat_t * ) ( m_table_index.ptr + i );

        if ( tstat->flag > 0 )
        {
            m_table_map.insert (std::pair<std::string, uint32_t>( tstat->table, i ));
        }
    }

    return true;
}

const char * hustdb_t::errno_string_status ( int r )
{
    switch ( r )
    {
        case 0:
            return "200 OK";

        case EKEYREJECTED:
            return "400 Bad request.";

        case EINVAL:
            return "401 Access denied.";

        case ENOMEM:
            return "403 Forbidden.";

        case ENOENT:
            return "404 Object not found.";

        case EPERM:
            return "412 Precondition failed.";

        default:
            return "500 Internal server error.";
    }
}

int hustdb_t::errno_int_status ( int r )
{
    switch ( r )
    {
        case 0:
            return 200;

        case EKEYREJECTED:
            return 400;

        case EINVAL:
            return 401;

        case ENOMEM:
            return 403;
            
        case ENOENT:
            return 404;

        case EPERM:
            return 412;

        default:
            return 500;
    }
}

char hustdb_t::real_table_type (
                                 uint8_t type
                                 )
{
    switch ( type )
    {
        case HASH_TB:
            return 'H';

        case SET_TB:
            return 'S';
            
        case ZSET_IN:
            return 'Y';

        case ZSET_TB:
            return 'Z';

        default:
            return 'N';
    }
}

bool hustdb_t::generate_hash_conf (
                                    int                             file_count,
                                    int                             copy_count,
                                    std::string &                   hash_conf
                                    )
{
    hash_conf.resize ( 0 );

    if ( file_count < 1 || copy_count != 1 )
    {
        LOG_ERROR ( "[hustdb][generate_hash_conf]file_count: %d, copy_count: %d", file_count, copy_count );
        return false;
    }

    bool b;
    hash_plan_t hp;

    b = hp.create ( file_count, 1, copy_count );
    if ( ! b )
    {
        LOG_ERROR ( "[hustdb][generate_hash_conf]hp.create failed" );
        return false;
    }

    b = hp.save_memory ( hash_conf );
    if ( ! b )
    {
        LOG_ERROR ( "[hustdb][generate_hash_conf]hp.save_memory failed, file_count: %d, copy_count: %d", file_count, copy_count );
        return false;
    }

    {
        hash_plan_t hp;
        b = hp.open_memory ( 0, hash_conf.c_str (), hash_conf.size () );
        if ( ! b )
        {
            LOG_ERROR ( "[hustdb][generate_hash_conf]hp.open_memory failed" );
            return false;
        }
    }

    return true;
}

bool hustdb_t::tb_name_check (
                               const char * name,
                               const int name_len
                               )
{
    if ( ! name || name_len <= 0 || name_len > MAX_QUEUE_NAME_LEN )
    {
        return false;
    }

    const char * str = name;

    for ( int i = 0; i < name_len; i ++ )
    {
        if ( ! ( ( str[i] >= '0' ) && ( str[i] <= '9' ) ) && ( ! ( str[i] >= 'a' && str[i] <= 'z' ) && ! ( str[i] >= 'A' && str[i] <= 'Z' ) ) && str[i] != '_' && str[i] != ':' && str[i] != '.' )
        {
            return false;
        }
    }

    return true;
}

int hustdb_t::find_table_type (
                                std::string & table,
                                uint8_t & type
                                )
{
    table_stat_t *           tstat        = NULL;

    scope_rlock_t tb_locker ( m_tb_locker );
    
    table_map_t::iterator it = m_table_map.find ( table );
    if ( it != m_table_map.end () )
    {
        tstat = ( table_stat_t * ) ( m_table_index.ptr + it->second );

        type = tstat->type;

        return it->second;
    }

    return - 1;
}

int hustdb_t::find_table_offset (
                                  std::string & table,
                                  bool create,
                                  uint8_t type
                                  )
{
    uint32_t                 i            = 0;
    table_stat_t *           tstat        = NULL;

    do
    {
        scope_rlock_t tb_locker ( m_tb_locker );

        table_map_t::iterator it = m_table_map.find ( table );
        if ( it != m_table_map.end () )
        {
            tstat = ( table_stat_t * ) ( m_table_index.ptr + it->second );

            if ( unlikely ( type != COMMON_TB && tstat->type != type ) )
            {
                return - 1;
            }

            return it->second;
        }
    }
    while ( 0 );

    if ( ! create )
    {
        return - 1;
    }

    scope_wlock_t tb_locker ( m_tb_locker );

    for ( i = TABLE_STAT_LEN; i < TABLE_INDEX_FILE_LEN; i += TABLE_STAT_LEN )
    {
        tstat = ( table_stat_t * ) ( m_table_index.ptr + i );

        if ( tstat->flag > 0 && strcmp ( table.c_str (), tstat->table ) == 0 )
        {
            return i;
        }
    }

    for ( i = TABLE_STAT_LEN; i < TABLE_INDEX_FILE_LEN; i += TABLE_STAT_LEN )
    {
        tstat = ( table_stat_t * ) ( m_table_index.ptr + i );

        if ( tstat->flag == 0 )
        {
            memset ( tstat, 0, TABLE_STAT_LEN );

            m_table_map.insert (std::pair<std::string, uint32_t>( table, i ));

            tstat->flag = 1;
            tstat->type = type;
            fast_memcpy ( tstat->table, table.c_str (), table.size () );

            return i;
        }
    }

    return - 1;
}

void hustdb_t::set_table_size (
                                int offset,
                                int atomic
                                )
{
    table_stat_t *           tstat        = NULL;

    tstat = ( table_stat_t * ) ( m_table_index.ptr + offset );
    atomic_fetch_add ( atomic, & tstat->size );

    if ( tstat->size <= 0 && offset > 0 )
    {
        scope_wlock_t tb_locker ( m_tb_locker );

        table_map_t::iterator it = m_table_map.find ( tstat->table );
        if ( it != m_table_map.end () )
        {
            m_table_map.erase ( it );
        }

        memset ( tstat, 0, TABLE_STAT_LEN );
    }
}

int hustdb_t::get_table_size (
                               const char * table,
                               size_t table_len
                               )
{
    table_stat_t *           tstat        = NULL;

    if ( ! table )
    {
        tstat = ( table_stat_t * ) m_table_index.ptr;
        return tstat->size;
    }
    else
    {
        std::string inner_table ( table, table_len );

        scope_rlock_t tb_locker ( m_tb_locker );
        
        table_map_t::iterator it = m_table_map.find ( inner_table );
        if ( it != m_table_map.end () )
        {
            tstat = ( table_stat_t * ) ( m_table_index.ptr + it->second );
            return tstat->size;
        }
    }

    return - 1;
}

uint32_t hustdb_t::clac_real_item (
                                    const uint32_t start,
                                    const uint32_t end
                                    )
{
    if ( start <= end )
    {
        return end - start;
    }
    else if ( start > end && start > MAX_QUEUE_ITEM_NUM )
    {
        return CYCLE_QUEUE_ITEM_NUM - start + end;
    }
    else
    {
        return 0;
    }
}

int hustdb_t::find_queue_offset (
                                  std::string & queue,
                                  bool create,
                                  uint8_t type,
                                  queue_info_t * & queue_info,
                                  const char * worker,
                                  size_t worker_len
                                  )
{
    uint32_t                 i            = 0;
    queue_stat_t *           qstat        = NULL;

    do
    {
        scope_rlock_t mq_locker ( m_mq_locker );

        queue_map_t::iterator it = m_queue_map.find ( queue );
        if ( it != m_queue_map.end () )
        {
            queue_info = & it->second;

            do
            {
                if ( ! worker || worker_len <= 0 )
                {
                    break;
                }

                std::string inner_worker ( worker, worker_len );

                worker_t * wmap = it->second.worker;

                worker_t::iterator wit = wmap->find ( inner_worker );
                if ( wit != wmap->end () )
                {
                    wit->second = m_current_timestamp;
                }
                else
                {
                    if ( unlikely ( wmap->size () > 1024 ) )
                    {
                        break;
                    }

                    wmap->insert (std::pair<std::string, uint32_t>( inner_worker, m_current_timestamp ));
                }
            }
            while ( 0 );

            qstat = ( queue_stat_t * ) ( m_queue_index.ptr + it->second.offset );
            if ( unlikely ( type != COMMON_TB && qstat->type != type ) )
            {
                return - 1;
            }

            return it->second.offset;
        }
    }
    while ( 0 );

    if ( ! create )
    {
        return - 1;
    }

    scope_wlock_t mq_locker ( m_mq_locker );

    for ( i = 0; i < QUEUE_INDEX_FILE_LEN; i += QUEUE_STAT_LEN )
    {
        qstat = ( queue_stat_t * ) ( m_queue_index.ptr + i );

        if ( qstat->flag > 0 && strcmp ( queue.c_str (), qstat->qname ) == 0 )
        {
            return i;
        }
    }

    for ( i = 0; i < QUEUE_INDEX_FILE_LEN; i += QUEUE_STAT_LEN )
    {
        qstat = ( queue_stat_t * ) ( m_queue_index.ptr + i );

        if ( qstat->flag == 0 )
        {
            memset ( qstat, 0, QUEUE_STAT_LEN );

            queue_info_t t;

            t.offset             = i;
            t.unacked            = new unacked_t ();
            t.redelivery         = new redelivery_t ();
            t.worker             = new worker_t ();

            m_queue_map.insert (std::pair<std::string, queue_info_t>( queue, t ));

            qstat->flag          = 1;
            qstat->type          = type;
            qstat->timeout       = m_store_conf.mq_redelivery_timeout;
            qstat->ctime         = ( uint32_t ) m_current_timestamp;
            fast_memcpy ( qstat->qname, queue.c_str (), queue.size () );

            return i;
        }
    }

    return - 1;
}

void hustdb_t::hustdb_memory_threshold ( )
{
    long            rss         = 0L;
    FILE *          fp          = NULL;
    FILE *          meminfo_fp  = NULL;
    long            mem[4]      = { 0L };

    if ( ( meminfo_fp = fopen ("/proc/meminfo", "r") ) == NULL )
    {
        return ;
    }

    for ( int i = 0; i < 4; i ++ )
    {
        if ( fscanf (meminfo_fp, "%*s%ld%*s", mem + i) != 1 )
        {
            fclose (meminfo_fp);
            return ;
        }
    }
    fclose (meminfo_fp);

    unsigned long freeram = ( mem[1] + mem[2] + mem[3] ) * 1024;
    unsigned long totalram = mem[0] * 1024;

    if ( m_server_conf.memory_system_threshold > 0 &&
         ( 100 * freeram / totalram ) < ( 100 - m_server_conf.memory_system_threshold )
         )
    {
        m_over_threshold = true;
        return ;
    }

    if ( ( fp = fopen ("/proc/self/statm", "r") ) == NULL )
    {
        return ;
    }

    if ( fscanf (fp, "%*s%ld", &rss) != 1 )
    {
        fclose (fp);
        return ;
    }
    fclose (fp);

    size_t process_total = ( size_t ) rss * ( size_t ) sysconf (_SC_PAGESIZE);
    if ( m_server_conf.memory_process_threshold > 0 &&
         ( 100 * process_total / totalram ) > m_server_conf.memory_process_threshold
         )
    {
        m_over_threshold = true;
        return ;
    }

    m_over_threshold = false;
}

void hustdb_t::slow_task_info (
                                std::string & info
                                )
{
    return m_slow_tasks.info ( info );
}

slow_task_type_t hustdb_t::slow_task_status (
                                              void * task
                                              )
{
    return m_slow_tasks.status ( ( task2_t * ) task );
}

void hustdb_t::hustdb_info (
                             std::string & info
                             )
{
    std::stringstream ss;

    if ( likely ( NULL != m_storage ) )
    {
        m_storage->info ( ss );
    }

    info = ss.str ();
}

int hustdb_t::hustdb_file_count ( )
{
    return m_storage->get_user_file_count ();
}

int hustdb_t::hustdb_exist (
                             const char * key,
                             size_t key_len,
                             uint32_t & ver,
                             conn_ctxt_t conn,
                             item_ctxt_t * & ctxt
                             )
{
    if ( unlikely ( CHECK_STRING ( key ) ) )
    {
        LOG_DEBUG ( "[hustdb][db_exist]params error" );
        return EKEYREJECTED;
    }

    m_storage->set_inner_table ( NULL, 0, KV_ALL, conn );

    return m_storage->exists ( key, key_len, ver, conn, ctxt );
}

int hustdb_t::hustdb_get (
                           const char *     key,
                           size_t           key_len,
                           std::string * &  rsp,
                           int &            rsp_len,
                           uint32_t &       ver,
                           conn_ctxt_t      conn,
                           item_ctxt_t * &  ctxt
                           )
{
    int             r           = 0;
    uint32_t        ttl         = 0;
    int             alive       = 0;
    bool            get_ok      = false;

    if ( unlikely ( CHECK_STRING ( key ) ) )
    {
        LOG_DEBUG ( "[hustdb][db_get]params error" );
        return EKEYREJECTED;
    }

    m_storage->set_inner_table ( NULL, 0, KV_ALL, conn );

    if ( m_mdb_ok && key_len < MDB_KEY_LEN )
    {
        r = m_mdb->get ( key, key_len, conn, rsp, & rsp_len );
        if ( r == 0 )
        {
            fast_memcpy ( & ver, rsp->c_str () + rsp_len - SIZEOF_UNIT32, SIZEOF_UNIT32 );
            rsp_len -= SIZEOF_UNIT32;
            get_ok = true;
        }
    }

    if ( ! get_ok )
    {
        r = m_storage->get ( key, key_len, ver, conn, rsp, ctxt );
        if ( 0 != r )
        {
            if ( ENOENT != r )
            {
                LOG_ERROR ( "[hustdb][db_get]get return %d", r );
            }

            return r;
        }

        ttl = m_storage->get_inner_ttl ( conn );
        if ( ttl > 0 )
        {
            alive = ( ttl > m_current_timestamp ) ? ( ttl - m_current_timestamp ) : - 1;

            if ( alive < 0 )
            {
                ver = 0;
                r = m_storage->del ( key, key_len, ver, false, conn, ctxt );
                if ( 0 != r )
                {
                    if ( ENOENT == r )
                    {
                        LOG_DEBUG ( "[hustdb][db_get]get ttl-del return ENOENT" );
                    }
                    else
                    {
                        LOG_ERROR ( "[hustdb][db_get]get ttl-del return %d", r );
                    }

                    return ENOENT;
                }

                if ( ! ctxt->is_version_error )
                {
                    set_table_size ( 0, - 1 );
                }

                return ENOENT;
            }
        }

        rsp_len = rsp->size ();

        if ( m_mdb_ok && alive >= 0 && key_len < MDB_KEY_LEN && rsp_len < MDB_VAL_LEN )
        {
            rsp->append ( ( const char * ) & ver, SIZEOF_UNIT32 );
            m_mdb->put ( key, key_len, rsp->c_str (), rsp_len + SIZEOF_UNIT32, alive );
        }
    }

    return 0;
}

int hustdb_t::hustdb_put (
                           const char * key,
                           size_t key_len,
                           const char * val,
                           size_t val_len,
                           uint32_t & ver,
                           uint32_t ttl,
                           bool is_dup,
                           conn_ctxt_t conn,
                           item_ctxt_t * & ctxt
                           )
{
    int             r           = 0;
    uint32_t        user_ver    = ver;

    if ( unlikely ( CHECK_STRING ( key ) ||
                   CHECK_STRING ( val ) ||
                   CHECK_VERSION
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_put]params error" );
        return EKEYREJECTED;
    }

    if ( unlikely ( m_over_threshold ) )
    {
        LOG_DEBUG ( "[hustdb][db_put]memory over threshold" );
        return ENOMEM;
    }

    m_storage->set_inner_table ( NULL, 0, KV_ALL, conn );

    if ( ttl <= 0 || ttl > m_store_conf.db_ttl_maximum )
    {
        m_storage->set_inner_ttl ( 0, conn );
    }
    else
    {
        m_storage->set_inner_ttl ( m_current_timestamp + ttl, conn );
    }

    r = m_storage->put ( key, key_len, val, val_len, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ctxt && ctxt->is_version_error )
        {
            if ( ! is_dup )
            {
                LOG_ERROR ( "[hustdb][db_put][user_ver=%u][ver=%u]put return EINVAL", user_ver, ver );
            }
            else
            {
                LOG_INFO ( "[hustdb][db_put][user_ver=%u][ver=%u]put return EINVAL", user_ver, ver );
            }
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_put][key=%p][key_len=%d]put return %d", key, ( int ) key_len, r );
        }

        return r;
    }

    if ( ctxt->kv_type == NEW_KV && ! ctxt->is_version_error )
    {
        set_table_size ( 0, 1 );
    }

    if ( ( ! is_dup && ver > 1 || is_dup && user_ver == ver && ! ctxt->is_version_error ) && m_mdb_ok && key_len < MDB_KEY_LEN )
    {
        if ( val_len < MDB_VAL_LEN )
        {
            std::string * buf = m_mdb->buffer ( conn );
            fast_memcpy ( ( char * ) & ( * buf ) [ 0 ], val, val_len );
            fast_memcpy ( ( char * ) & ( * buf ) [ 0 ] + val_len, & ver, SIZEOF_UNIT32 );

            m_mdb->put ( key, key_len, buf->c_str (), val_len + SIZEOF_UNIT32, ttl );
        }
        else
        {
            m_mdb->del ( key, key_len );
        }
    }

    return 0;
}

int hustdb_t::hustdb_del (
                           const char * key,
                           size_t key_len,
                           uint32_t & ver,
                           bool is_dup,
                           conn_ctxt_t conn,
                           item_ctxt_t * & ctxt
                           )
{
    int             r           = 0;

    if ( unlikely ( CHECK_STRING ( key ) ||
                   CHECK_VERSION
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_del]params error" );
        return EKEYREJECTED;
    }

    m_storage->set_inner_table ( NULL, 0, KV_ALL, conn );

    r = m_storage->del ( key, key_len, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT == r )
        {
            LOG_DEBUG ( "[hustdb][db_del]del return ENOENT" );
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_del]del return %d", r );
        }

        return r;
    }

    if ( ! ctxt->is_version_error )
    {
        set_table_size ( 0, - 1 );
    }

    if ( ! ctxt->is_version_error && m_mdb_ok && key_len < MDB_KEY_LEN )
    {
        m_mdb->del ( key, key_len );
    }

    return 0;
}

int hustdb_t::hustdb_keys (
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
                            )
{
    int             r            = 0;
    int             file_count   = - 1;

    file_count = m_storage->get_user_file_count ();

    if ( unlikely ( offset < 0 || offset > MAX_EXPORT_OFFSET ||
                   size < 0 || size > MEM_EXPORT_SIZE ||
                   file_id < 0 || file_id >= file_count ||
                   ! async && offset > SYNC_EXPORT_OFFSET ||
                   start < 0 || end < 0 || start >= end || end > MAX_BUCKET_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_keys]params error" );
        return EKEYREJECTED;
    }

    m_storage->set_inner_table ( EXPORT_DB_ALL, sizeof ( EXPORT_DB_ALL ) - 1, KV_ALL, conn );

    struct export_cb_param_t cb_pm;
    cb_pm.start             = start;
    cb_pm.end               = end;
    cb_pm.offset            = offset;
    cb_pm.size              = size;
    cb_pm.file_id           = file_id;
    cb_pm.noval             = noval;
    cb_pm.async             = async;
    cb_pm.total             = 0;
    cb_pm.min               = 0;
    cb_pm.max               = 0;

    r = m_storage->export_db_mem ( conn, rsp, ctxt, NULL, & cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[hustdb][db_keys]keys return %d", r );
    }

    hits                    = cb_pm.size;
    total                   = cb_pm.total;

    return r;
}

int hustdb_t::hustdb_stat (
                            const char * table,
                            size_t table_len,
                            int & count
                            )
{
    if ( unlikely ( table &&
                   table_len > 0 &&
                   ! tb_name_check ( table, table_len )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_stat]params error" );
        return EKEYREJECTED;
    }

    if ( table && table_len > 0 )
    {
        count = get_table_size ( table, table_len );
        if ( unlikely ( count < 0 ) )
        {
            return EPERM;
        }
    }
    else
    {
        count = get_table_size ( NULL, 0 );
    }

    return 0;
}

void hustdb_t::hustdb_stat_all (
                                 std::string & stats
                                 )
{
    char stat[ 128 ] = { };

    stats.resize ( 0 );
    stats.reserve ( 1048576 );
    stats += "[";

    sprintf ( stat,
             "{\"table\":\"\",\"type\":\"\",\"size\":%d},",
             get_table_size ( NULL, 0 )
             );

    stats += stat;

    scope_rlock_t tb_locker ( m_tb_locker );
    
    for ( table_map_t::iterator it = m_table_map.begin (); it != m_table_map.end (); it ++ )
    {
        table_stat_t * tstat = ( table_stat_t * ) ( m_table_index.ptr + it->second );

        memset ( stat, 0, sizeof ( stat ) );
        sprintf ( stat,
                 "{\"table\":\"%s\",\"type\":\"%c\",\"size\":%d},",
                 tstat->table,
                 real_table_type ( tstat->type ),
                 tstat->size
                 );
        stats += stat;
    }

    stats.erase ( stats.end () - 1, stats.end () );
    stats += "]";
}

int hustdb_t::hustdb_export (
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
                              )
{
    int             inner_file_id   = - 1;
    int             file_count      = - 1;
    char            path [ 256 ]    = { };
    const char      flag_k[]        = "k";
    const char      flag_kv[]       = "kv";
    const char *    noval_flag      = NULL;
    uint8_t         type            = 0;

    if ( unlikely ( ! m_apptool->is_dir ( HUSTDB_EXPORT_DIR ) ) )
    {
        LOG_ERROR ( "[hustdb][db_export]./EXPORT not exists" );
        return ENOENT;
    }

    file_count = m_storage->get_user_file_count ();

    if ( table && table_len > 0 )
    {
        std::string inner_table ( table, table_len );

        if ( ( ! tb_name_check ( table, table_len ) || find_table_type ( inner_table, type ) < 0 ) )
        {
            LOG_DEBUG ( "[hustdb][db_export]params error" );
            return EKEYREJECTED;
        }

        sprintf ( path, "%s%c", inner_table.c_str (), type );
        inner_file_id = m_apptool->hust_hash_key ( path, strlen ( path ) ) % file_count;
    }
    else
    {
        sprintf ( path, EXPORT_DB_ALL );
        inner_file_id = file_id;
    }

    if ( unlikely ( offset > MAX_EXPORT_OFFSET || size > DISK_EXPORT_SIZE || ( offset > 0 && size == 0 ) ||
                   inner_file_id < 0 || inner_file_id >= file_count ||
                   start < 0 || end < 0 || start >= end || end > MAX_BUCKET_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_export]params error" );
        return EKEYREJECTED;
    }

    noval_flag = noval ? flag_k : flag_kv;

    char i_ph[ 256 ] = { };
    char d_ph[ 256 ] = { };

    sprintf ( i_ph, "./EXPORT/%s%d[%d-%d].%s", path, inner_file_id, start, end, noval_flag );
    m_apptool->path_to_os ( i_ph );

    sprintf ( d_ph, "./EXPORT/%s%d[%d-%d].%s.data", path, inner_file_id, start, end, noval_flag );
    m_apptool->path_to_os ( d_ph );

    if ( unlikely ( m_apptool->is_dir ( i_ph ) || m_apptool->is_dir ( d_ph ) ||
                   ( ! cover && ( m_apptool->is_file ( i_ph ) || m_apptool->is_file ( d_ph ) ) )
                   )
         )
    {
        LOG_ERROR ( "[hustdb][db_export]an error directory or result already exist: %s, %s", i_ph, d_ph );
        return ENOENT;
    }

    if ( ! m_slow_tasks.empty () )
    {
        LOG_ERROR ( "[hustdb][db_export]slow_tasks not empty" );
        return EPERM;
    }

    task_export_t * task = task_export_t::create ( inner_file_id, path, offset, size, start, end, noval, cover );
    if ( NULL == task )
    {
        LOG_ERROR ( "[hustdb][db_export]task_export_t::create failed" );
        return EPERM;
    }

    if ( ! m_slow_tasks.push ( task ) )
    {
        task->release ();

        LOG_ERROR ( "[hustdb][db_export]push task_export_t failed" );
        return EPERM;
    }

    token = task;

    return 0;
}

int hustdb_t::hustdb_ttl_scan ( )
{
    if ( ! m_slow_tasks.empty () )
    {
        LOG_ERROR ( "[hustdb][db_ttl_scan]slow_tasks not empty" );
        return EPERM;
    }

    task_ttl_scan_t * task = task_ttl_scan_t::create ( m_store_conf.db_ttl_scan_count );
    if ( NULL == task )
    {
        LOG_ERROR ( "[hustdb][db_ttl_scan]task_ttl_scan_t::create failed" );
        return EPERM;
    }

    if ( ! m_slow_tasks.push ( task ) )
    {
        task->release ();

        LOG_ERROR ( "[hustdb][db_ttl_scan]push task_ttl_scan_t failed" );
        return EPERM;
    }

    return 0;
}

int hustdb_t::hustdb_binlog_scan ( )
{
    if ( ! m_slow_tasks.le_one () )
    {
        LOG_ERROR ( "[hustdb][db_binlog_scan]slow_tasks not empty" );
        return EPERM;
    }

    task_binlog_scan_t * task = task_binlog_scan_t::create ( );
    if ( NULL == task )
    {
        LOG_ERROR ( "[hustdb][db_binlog_scan]task_binlog_scan_t::create failed" );
        return EPERM;
    }

    if ( ! m_slow_tasks.push ( task ) )
    {
        task->release ();

        LOG_ERROR ( "[hustdb][db_binlog_scan]push task_binlog_scan_t failed" );
        return EPERM;
    }

    return 0;
}

int hustdb_t::hustdb_hexist (
                              const char * table,
                              size_t table_len,
                              const char * key,
                              size_t key_len,
                              uint32_t & ver,
                              conn_ctxt_t conn,
                              item_ctxt_t * & ctxt
                              )
{
    int             r            = 0;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_hexist]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, HASH_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_hexist]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, HASH_TB, conn );

    r = m_storage->exists ( key, key_len, ver, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[hustdb][db_hexist]exists return %d", r );
        }
    }

    return r;
}

int hustdb_t::hustdb_hget (
                            const char * table,
                            size_t table_len,
                            const char * key,
                            size_t key_len,
                            std::string * & rsp,
                            int & rsp_len,
                            uint32_t & ver,
                            conn_ctxt_t conn,
                            item_ctxt_t * & ctxt
                            )
{
    int             r            = 0;
    size_t          mkey_len     = 0;
    const char *    mkey         = NULL;
    uint32_t        ttl          = 0;
    int             alive        = 0;
    bool            get_ok       = false;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_hget]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, HASH_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_hget]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, HASH_TB, conn );

    if ( m_mdb_ok && table_len + key_len + 2 < MDB_KEY_LEN )
    {
        mkey_len = key_len;
        mkey = m_storage->get_inner_tbkey ( key, mkey_len, conn );

        r = m_mdb->get ( mkey, mkey_len, conn, rsp, & rsp_len );
        if ( r == 0 )
        {
            fast_memcpy ( & ver, rsp->c_str () + rsp_len - SIZEOF_UNIT32, SIZEOF_UNIT32 );
            rsp_len -= SIZEOF_UNIT32;
            get_ok = true;
        }
    }

    if ( ! get_ok )
    {
        r = m_storage->get ( key, key_len, ver, conn, rsp, ctxt );
        if ( 0 != r )
        {
            if ( ENOENT != r )
            {
                LOG_ERROR ( "[hustdb][db_hget]hget return %d", r );
            }

            return r;
        }

        ttl = m_storage->get_inner_ttl ( conn );
        if ( ttl > 0 )
        {
            alive = ( ttl > m_current_timestamp ) ? ( ttl - m_current_timestamp ) : - 1;

            if ( alive < 0 )
            {
                ver = 0;
                r = m_storage->del ( key, key_len, ver, false, conn, ctxt );
                if ( 0 != r )
                {
                    if ( ENOENT == r )
                    {
                        LOG_DEBUG ( "[hustdb][db_hget]hget ttl-del return ENOENT" );
                    }
                    else
                    {
                        LOG_ERROR ( "[hustdb][db_hget]hget ttl-del return %d", r );
                    }

                    return ENOENT;
                }

                if ( ! ctxt->is_version_error )
                {
                    set_table_size ( offset, - 1 );
                }

                return ENOENT;
            }
        }

        rsp_len = rsp->size ();

        if ( m_mdb_ok && alive >= 0 && table_len + key_len + 2 < MDB_KEY_LEN && rsp_len < MDB_VAL_LEN )
        {
            rsp->append ( ( const char * ) & ver, SIZEOF_UNIT32 );
            m_mdb->put ( mkey, mkey_len, rsp->c_str (), rsp_len + SIZEOF_UNIT32, alive );
        }
    }

    return 0;
}

int hustdb_t::hustdb_hset (
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
                            )
{
    int             r            = 0;
    uint32_t        user_ver     = ver;
    size_t          mkey_len     = 0;
    const char *    mkey         = NULL;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_VERSION ||
                   CHECK_STRING ( key ) ||
                   CHECK_STRING ( val )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_hset]params error" );
        return EKEYREJECTED;
    }

    if ( unlikely ( m_over_threshold ) )
    {
        LOG_DEBUG ( "[hustdb][db_hset]memory over threshold" );
        return ENOMEM;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, true, HASH_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_hset]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, HASH_TB, conn );

    if ( ttl <= 0 || ttl > m_store_conf.db_ttl_maximum )
    {
        m_storage->set_inner_ttl ( 0, conn );
    }
    else
    {
        m_storage->set_inner_ttl ( m_current_timestamp + ttl, conn );
    }

    r = m_storage->put ( key, key_len, val, val_len, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ctxt && ctxt->is_version_error )
        {
            if ( ! is_dup )
            {
                LOG_ERROR ( "[hustdb][db_hset][user_ver=%u][ver=%u]hset return EINVAL", user_ver, ver );
            }
            else
            {
                LOG_INFO ( "[hustdb][db_hset][user_ver=%u][ver=%u]hset return EINVAL", user_ver, ver );
            }
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_hset][key=%p][key_len=%d]hset return %d", key, ( int ) key_len, r );
        }

        return r;
    }

    if ( ctxt->kv_type == NEW_KV && ! ctxt->is_version_error )
    {
        set_table_size ( offset, 1 );
    }

    if ( ( ! is_dup && ver > 1 || is_dup && user_ver == ver && ! ctxt->is_version_error ) && m_mdb_ok && table_len + key_len + 2 < MDB_KEY_LEN )
    {
        mkey_len = key_len;
        mkey = m_storage->get_inner_tbkey ( key, mkey_len, conn );

        if ( val_len < MDB_VAL_LEN )
        {
            std::string * buf = m_mdb->buffer ( conn );
            fast_memcpy ( ( char * ) & ( * buf ) [ 0 ], val, val_len );
            fast_memcpy ( ( char * ) & ( * buf ) [ 0 ] + val_len, & ver, SIZEOF_UNIT32 );
            m_mdb->put ( mkey, mkey_len, buf->c_str (), val_len + SIZEOF_UNIT32, ttl );
        }
        else
        {
            m_mdb->del ( mkey, mkey_len );
        }
    }

    return 0;
}

int hustdb_t::hustdb_hdel (
                            const char * table,
                            size_t table_len,
                            const char * key,
                            size_t key_len,
                            uint32_t & ver,
                            bool is_dup,
                            conn_ctxt_t conn,
                            item_ctxt_t * & ctxt
                            )
{
    int             r            = 0;
    size_t          mkey_len     = 0;
    const char *    mkey         = NULL;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_VERSION ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_del]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, HASH_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_hdel]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, HASH_TB, conn );

    r = m_storage->del ( key, key_len, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT == r )
        {
            LOG_DEBUG ( "[hustdb][db_hdel]hdel return ENOENT" );
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_hdel]hdel return %d", r );
        }

        return r;
    }

    if ( ! ctxt->is_version_error )
    {
        set_table_size ( offset, - 1 );
    }

    if ( ! ctxt->is_version_error && m_mdb_ok && table_len + key_len + 2 < MDB_KEY_LEN )
    {
        mkey_len = key_len;
        mkey = m_storage->get_inner_tbkey ( key, mkey_len, conn );

        m_mdb->del ( mkey, mkey_len );
    }

    return 0;
}

int hustdb_t::hustdb_hkeys (
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
                             )
{
    int             r            = 0;
    int             _offset      = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   offset < 0 || offset > MAX_EXPORT_OFFSET ||
                   size < 0 || size > MEM_EXPORT_SIZE ||
                   ! async && offset > SYNC_EXPORT_OFFSET ||
                   start < 0 || end < 0 || start >= end || end > MAX_BUCKET_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_hkeys]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    _offset = find_table_offset ( inner_table, false, HASH_TB );
    if ( unlikely ( _offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_hkeys]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, HASH_TB, conn );

    struct export_cb_param_t cb_pm;
    cb_pm.start             = start;
    cb_pm.end               = end;
    cb_pm.offset            = offset;
    cb_pm.size              = size;
    cb_pm.file_id           = - 1;
    cb_pm.noval             = noval;
    cb_pm.async             = async;
    cb_pm.total             = 0;
    cb_pm.min               = 0;
    cb_pm.max               = 0;

    r = m_storage->export_db_mem ( conn, rsp, ctxt, NULL, & cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[hustdb][db_hkeys]hkeys return %d", r );
    }

    hits                    = cb_pm.size;
    total                   = cb_pm.total;

    return r;
}

int hustdb_t::hustdb_sismember (
                                 const char * table,
                                 size_t table_len,
                                 const char * key,
                                 size_t key_len,
                                 uint32_t & ver,
                                 conn_ctxt_t conn,
                                 item_ctxt_t * & ctxt
                                 )
{
    int             r            = 0;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_sismember]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, SET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_sismember]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, SET_TB, conn );

    r = m_storage->exists ( key, key_len, ver, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[hustdb][db_sismember]exists return %d", r );
        }
    }

    return r;
}

int hustdb_t::hustdb_sadd (
                            const char * table,
                            size_t table_len,
                            const char * key,
                            size_t key_len,
                            uint32_t & ver,
                            uint32_t ttl,
                            bool is_dup,
                            conn_ctxt_t conn,
                            item_ctxt_t * & ctxt
                            )
{
    int             r            = 0;
    uint32_t        user_ver     = ver;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_VERSION ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_sadd]params error" );
        return EKEYREJECTED;
    }

    if ( unlikely ( m_over_threshold ) )
    {
        LOG_DEBUG ( "[hustdb][db_sadd]memory over threshold" );
        return ENOMEM;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, true, SET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_sadd]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, SET_TB, conn );

    if ( ttl <= 0 || ttl > m_store_conf.db_ttl_maximum )
    {
        m_storage->set_inner_ttl ( 0, conn );
    }
    else
    {
        m_storage->set_inner_ttl ( m_current_timestamp + ttl, conn );
    }

    r = m_storage->put ( key, key_len, NULL, 0, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ctxt && ctxt->is_version_error )
        {
            if ( ! is_dup )
            {
                LOG_ERROR ( "[hustdb][db_sadd][user_ver=%u][ver=%u]sadd return EINVAL", user_ver, ver );
            }
            else
            {
                LOG_INFO ( "[hustdb][db_sadd][user_ver=%u][ver=%u]sadd return EINVAL", user_ver, ver );
            }
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_sadd][key=%p][key_len=%d]sadd return %d", key, ( int ) key_len, r );
        }

        return r;
    }

    if ( ctxt->kv_type == NEW_KV && ! ctxt->is_version_error )
    {
        set_table_size ( offset, 1 );
    }

    return 0;
}

int hustdb_t::hustdb_srem (
                            const char * table,
                            size_t table_len,
                            const char * key,
                            size_t key_len,
                            uint32_t & ver,
                            bool is_dup,
                            conn_ctxt_t conn,
                            item_ctxt_t * & ctxt
                            )
{
    int             r            = 0;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_VERSION ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_srem]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, SET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_srem]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, SET_TB, conn );

    r = m_storage->del ( key, key_len, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT == r )
        {
            LOG_DEBUG ( "[hustdb][db_srem]del return ENOENT" );
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_srem]del return %d", r );
        }

        return r;
    }

    if ( ! ctxt->is_version_error )
    {
        set_table_size ( offset, - 1 );
    }

    return 0;
}

int hustdb_t::hustdb_smembers (
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
                                )
{
    int             r            = 0;
    int             _offset      = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   offset < 0 || offset > MAX_EXPORT_OFFSET ||
                   size < 0 || size > MEM_EXPORT_SIZE ||
                   ! async && offset > SYNC_EXPORT_OFFSET ||
                   start < 0 || end < 0 || start >= end || end > MAX_BUCKET_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_smembers]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    _offset = find_table_offset ( inner_table, false, SET_TB );
    if ( unlikely ( _offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_smembers]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, SET_TB, conn );

    struct export_cb_param_t cb_pm;
    cb_pm.start             = start;
    cb_pm.end               = end;
    cb_pm.offset            = offset;
    cb_pm.size              = size;
    cb_pm.file_id           = - 1;
    cb_pm.noval             = noval;
    cb_pm.async             = async;
    cb_pm.total             = 0;
    cb_pm.min               = 0;
    cb_pm.max               = 0;

    r = m_storage->export_db_mem ( conn, rsp, ctxt, NULL, & cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[hustdb][db_smembers]smembers return %d", r );
    }

    hits                    = cb_pm.size;
    total                   = cb_pm.total;

    return r;
}

int hustdb_t::hustdb_zismember (
                                 const char * table,
                                 size_t table_len,
                                 const char * key,
                                 size_t key_len,
                                 uint32_t & ver,
                                 conn_ctxt_t conn,
                                 item_ctxt_t * & ctxt
                                 )
{
    int             r            = 0;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_zismember]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, ZSET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_zismember]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, ZSET_IN, conn );

    r = m_storage->exists ( key, key_len, ver, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[hustdb][db_zismember]exists return %d", r );
        }
    }

    return r;
}

int hustdb_t::hustdb_zadd (
                            const char * table,
                            size_t table_len,
                            const char * key,
                            size_t key_len,
                            uint64_t score,
                            int opt,
                            uint32_t & ver,
                            uint32_t ttl,
                            bool is_dup,
                            conn_ctxt_t conn,
                            bool & is_version_error
                            )
{
    int             r            = 0;
    uint32_t        user_ver     = ver;
    int             offset       = - 1;
    size_t          val_len      = 0;
    char            val[ 32 ]    = { };
    char            val21[ 32 ]  = { };
    uint32_t        cur_ver      = 0;
    uint64_t        cur_score    = 0;
    bool            has_been     = false;
    item_ctxt_t *   ctxt         = NULL;
    std::string *   rsp          = NULL;
    uint16_t        zhash        = 0;
    lockable_t *    zlkt         = NULL;
    is_version_error             = false;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_VERSION ||
                   CHECK_STRING ( key ) ||
                   score <= 0
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_zadd]params error" );
        return EKEYREJECTED;
    }

    if ( unlikely ( m_over_threshold ) )
    {
        LOG_DEBUG ( "[hustdb][db_zadd]memory over threshold" );
        return ENOMEM;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, true, ZSET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_zadd]find table failed" );
        return EPERM;
    }

    zhash = m_apptool->locker_hash ( key, key_len );
    zlkt = m_lockers.at ( zhash );
    scope_lock_t zlocker ( * zlkt );

    m_storage->set_inner_ttl ( 0, conn );
    m_storage->set_inner_table ( table, table_len, ZSET_IN, conn );

    r = m_storage->get ( key, key_len, cur_ver, conn, rsp, ctxt );
    if ( 0 == r )
    {
        has_been  = true;
        cur_score = strtoul ( rsp->c_str (), NULL, 10 );

        if ( opt == 0 &&
             cur_score == score &&
             cur_ver >= ver
             )
        {
            ver = cur_ver;
            is_version_error = false;

            return 0;
        }
    }

    if ( opt > 0 )
    {
        score = cur_score + score;
    }
    else if ( opt < 0 )
    {
        score = ( cur_score > score ) ? ( cur_score - score ) : 0;
    }

    sprintf ( val, "%lu", score );
    val_len = strlen ( val );

    if ( ttl > 0 && ttl <= m_store_conf.db_ttl_maximum )
    {
        m_storage->set_inner_ttl ( m_current_timestamp + ttl, conn );
    }

    r = m_storage->put ( key, key_len, val, val_len, ver, is_dup, conn, ctxt );
    is_version_error = ctxt ? ctxt->is_version_error : true;
    if ( 0 != r )
    {
        if ( ctxt && ctxt->is_version_error )
        {
            if ( ! is_dup )
            {
                LOG_ERROR ( "[hustdb][db_zadd][user_ver=%u][ver=%u]zadd return EINVAL", user_ver, ver );
            }
            else
            {
                LOG_INFO ( "[hustdb][db_zadd][user_ver=%u][ver=%u]zadd return EINVAL", user_ver, ver );
            }
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_zadd][key=%p][key_len=%d]zadd return %d", key, ( int ) key_len, r );
        }

        return r;
    }

    if ( ctxt->kv_type == NEW_KV && ! ctxt->is_version_error )
    {
        set_table_size ( offset, 1 );
    }

    m_storage->set_inner_ttl ( 0, conn );

    if ( has_been )
    {
        sprintf ( val21, "%021lu", cur_score );
        m_storage->set_inner_table ( table, table_len, ZSET_TB, conn, val21 );

        user_ver = 0;
        r = m_storage->del ( key, key_len, user_ver, false, conn, ctxt );
        if ( 0 != r )
        {
            if ( ENOENT == r )
            {
                LOG_DEBUG ( "[hustdb][db_zadd]del return ENOENT" );
            }
            else
            {
                LOG_ERROR ( "[hustdb][db_zadd]del return %d", r );
            }
        }
    }

    sprintf ( val21, "%021lu", score );
    m_storage->set_inner_table ( table, table_len, ZSET_TB, conn, val21 );

    user_ver = ver;
    r = m_storage->put ( key, key_len, val, val_len, user_ver, true, conn, ctxt );
    if ( 0 != r )
    {
        LOG_ERROR ( "[hustdb][db_zadd][key=%p][key_len=%d]zadd return %d", key, ( int ) key_len, r );
        return r;
    }

    return 0;
}

int hustdb_t::hustdb_zscore (
                              const char * table,
                              size_t table_len,
                              const char * key,
                              size_t key_len,
                              std::string * & rsp,
                              int & rsp_len,
                              uint32_t & ver,
                              conn_ctxt_t conn,
                              item_ctxt_t * & ctxt
                              )
{
    int             r            = 0;
    int             offset       = - 1;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_zscore]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, ZSET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_zscore]find table failed" );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, ZSET_IN, conn );

    r = m_storage->get ( key, key_len, ver, conn, rsp, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT != r )
        {
            LOG_ERROR ( "[hustdb][db_zscore]zscore return %d", r );
        }

        return r;
    }

    rsp_len = rsp->size ();

    return 0;
}

int hustdb_t::hustdb_zrem (
                            const char * table,
                            size_t table_len,
                            const char * key,
                            size_t key_len,
                            uint32_t & ver,
                            bool is_dup,
                            conn_ctxt_t conn,
                            item_ctxt_t * & ctxt
                            )
{
    int             r            = 0;
    int             offset       = - 1;
    char            val21[ 32 ]  = { };
    uint32_t        cur_ver      = 0;
    uint64_t        cur_score    = 0;
    std::string *   rsp          = NULL;
    uint16_t        zhash        = 0;
    lockable_t *    zlkt         = NULL;

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   CHECK_VERSION ||
                   CHECK_STRING ( key )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_zrem]params error" );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    offset = find_table_offset ( inner_table, false, ZSET_TB );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_zrem]find table failed" );
        return EPERM;
    }

    zhash = m_apptool->locker_hash ( key, key_len );
    zlkt = m_lockers.at ( zhash );
    scope_lock_t zlocker ( * zlkt );

    m_storage->set_inner_table ( table, table_len, ZSET_IN, conn );

    r = m_storage->get ( key, key_len, cur_ver, conn, rsp, ctxt );
    if ( 0 == r )
    {
        cur_score = strtoul ( rsp->c_str (), NULL, 10 );
    }

    r = m_storage->del ( key, key_len, ver, is_dup, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT == r )
        {
            LOG_DEBUG ( "[hustdb][db_zrem]del return ENOENT" );
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_zrem]del return %d", r );
        }

        return r;
    }

    if ( ! ctxt->is_version_error )
    {
        set_table_size ( offset, - 1 );
    }

    sprintf ( val21, "%021lu", cur_score );
    m_storage->set_inner_table ( table, table_len, ZSET_TB, conn, val21 );

    cur_ver = 0;
    r = m_storage->del ( key, key_len, cur_ver, false, conn, ctxt );
    if ( 0 != r )
    {
        if ( ENOENT == r )
        {
            LOG_DEBUG ( "[hustdb][db_zrem]del return ENOENT" );
        }
        else
        {
            LOG_ERROR ( "[hustdb][db_zrem]del return %d", r );
        }
    }

    return 0;
}

int hustdb_t::hustdb_zrange (
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
                              )
{
    int             r            = 0;
    int             _offset      = - 1;
    uint16_t        hash         = 0;

    if ( byscore )
    {
        async = false;
    }
    else
    {
        min = max = 0;
    }

    if ( unlikely ( ! tb_name_check ( table, table_len ) ||
                   offset < 0 || offset > MAX_EXPORT_OFFSET ||
                   size < 0 || size > MEM_EXPORT_SIZE ||
                   ! async && offset > SYNC_EXPORT_OFFSET ||
                   start < 0 || end < 0 || start >= end || end > MAX_BUCKET_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_zrange][byscore=%d]params error", byscore );
        return EKEYREJECTED;
    }

    std::string inner_table ( table, table_len );

    _offset = find_table_offset ( inner_table, false, ZSET_TB );
    if ( unlikely ( _offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][db_zrange]find table failed" );
        return EPERM;
    }

    hash = m_apptool->bucket_hash ( table, table_len );
    if ( hash < start || hash >= end )
    {
        LOG_ERROR ( "[hustdb][db_zrange][byscore=%d]bucket_hash [table=%s] out of range[%d-%d]", byscore, inner_table.c_str (), start, end );
        return EPERM;
    }

    m_storage->set_inner_table ( table, table_len, ZSET_TB, conn );

    struct export_cb_param_t cb_pm;
    cb_pm.start             = 0;
    cb_pm.end               = MAX_BUCKET_NUM;
    cb_pm.offset            = offset;
    cb_pm.size              = size;
    cb_pm.file_id           = - 1;
    cb_pm.noval             = noval;
    cb_pm.async             = async;
    cb_pm.total             = 0;
    cb_pm.min               = min;
    cb_pm.max               = max;

    r = m_storage->export_db_mem ( conn, rsp, ctxt, NULL, & cb_pm );
    if ( 0 != r )
    {
        LOG_ERROR ( "[hustdb][db_zrange][byscore=%d]zrange return %d", byscore, r );
    }

    hits                    = cb_pm.size;
    total                   = cb_pm.total;

    return r;
}

int hustdb_t::hustdb_binlog (
                              const char * table,
                              size_t table_len,
                              const char * key,
                              size_t key_len,
                              const char * host,
                              size_t host_len,
                              uint8_t cmd_type,
                              conn_ctxt_t conn
                              )
{
    int                  r            = 0;
    bool                 is_rem       = false;

    if ( unlikely ( ( table && table_len > 0 && ! tb_name_check ( table, table_len ) ) ||
                   CHECK_STRING ( key ) ||
                   CHECK_STRING ( host ) ||
                   host_len > 32
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][db_binlog]params error" );
        return EKEYREJECTED;
    }
    
    switch ( cmd_type )
    {
        case HUSTDB_METHOD_DEL:
            is_rem = true;
        case HUSTDB_METHOD_PUT:
            m_storage->set_inner_table ( NULL, 0, KV_ALL, conn );
            break;
            
        case HUSTDB_METHOD_HDEL:
            is_rem = true;
        case HUSTDB_METHOD_HSET:
            m_storage->set_inner_table ( table, table_len, HASH_TB, conn );
            break;
            
        case HUSTDB_METHOD_SREM:
            is_rem = true;
        case HUSTDB_METHOD_SADD:
            m_storage->set_inner_table ( table, table_len, SET_TB, conn );
            break;
            
        case HUSTDB_METHOD_ZREM:
            is_rem = true;
        case HUSTDB_METHOD_ZADD:
            m_storage->set_inner_table ( table, table_len, ZSET_IN, conn );
            break;
            
        default:
            LOG_DEBUG ( "[hustdb][db_binlog]cmd_type error" );
            return EKEYREJECTED;
    }

    r = m_storage->binlog ( key, key_len, host, host_len, m_current_timestamp, cmd_type, is_rem, conn );
    if ( 0 != r )
    {
        LOG_ERROR ( "[hustdb][db_binlog]binlog return %d", r );
    }

    return r;
}

int hustdb_t::hustmq_put (
                           const char * queue,
                           size_t queue_len,
                           const char * item,
                           size_t item_len,
                           uint32_t priori,
                           conn_ctxt_t conn
                           )
{
    int             r                       = 0;
    size_t          qkey_len                = 0;
    char            qkey[ MAX_QKEY_LEN ]    = { };
    uint32_t        tag                     = 0;
    uint32_t        ver                     = 0;
    uint16_t        qhash                   = 0;
    uint32_t        real                    = 0;
    int             offset                  = - 1;
    lockable_t *    qlkt                    = NULL;
    queue_stat_t *  qstat                   = NULL;
    unsigned char * qstat_val               = NULL;
    queue_info_t *  queue_info              = NULL;
    item_ctxt_t *   ctxt                    = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   CHECK_STRING ( item ) ||
                   priori < 0 || priori > 2
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_put]params error" );
        return EKEYREJECTED;
    }

    if ( unlikely ( m_over_threshold ) )
    {
        LOG_DEBUG ( "[hustdb][mq_put]memory over threshold" );
        return ENOMEM;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, true, QUEUE_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_put][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );
    qstat_val = ( unsigned char * ) ( m_queue_index.ptr + offset );

    fast_memcpy ( & tag, qstat_val + SIZEOF_UNIT32 * ( priori * 2 + 1 ), SIZEOF_UNIT32 );
    tag = ( tag + 1 ) % CYCLE_QUEUE_ITEM_NUM;

    real = clac_real_item ( qstat->sp, qstat->ep ) + clac_real_item ( qstat->sp1, qstat->ep1 ) + clac_real_item ( qstat->sp2, qstat->ep2 );

    if ( qstat->lock == 1 || ( qstat->max > 0 && real >= qstat->max ) || real >= MAX_QUEUE_ITEM_NUM )
    {
        return EINVAL;
    }

    sprintf ( qkey, "%s|%u:%u", inner_queue.c_str (), priori, tag );
    qkey_len = strlen ( qkey );

    m_storage->set_inner_ttl ( 0, conn );
    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    r = m_storage->put ( qkey, qkey_len, item, item_len, ver, false, conn, ctxt );
    if ( unlikely ( 0 != r ) )
    {
        LOG_ERROR ( "[hustdb][mq_put]put failed, return:%d, key:%s, item_len:%d", r, qkey, item_len );
        return r;
    }

    fast_memcpy ( qstat_val + SIZEOF_UNIT32 * ( priori * 2 + 1 ), & tag, SIZEOF_UNIT32 );

    return 0;
}

int hustdb_t::hustmq_get (
                           const char * queue,
                           size_t queue_len,
                           const char * worker,
                           size_t worker_len,
                           bool is_ack,
                           std::string & ack,
                           std::string & unacked,
                           std::string * & rsp,
                           conn_ctxt_t conn
                           )
{
    int             r                       = 0;
    size_t          qkey_len                = 0;
    char            qkey[ MAX_QKEY_LEN ]    = { };
    uint32_t        priori                  = 0;
    uint32_t        tag                     = 0;
    uint32_t        ver                     = 0;
    uint16_t        qhash                   = 0;
    int             offset                  = - 1;
    lockable_t *    qlkt                    = NULL;
    queue_stat_t *  qstat                   = NULL;
    unsigned char * qstat_val               = NULL;
    queue_info_t *  queue_info              = NULL;
    item_ctxt_t *   ctxt                    = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   worker_len <= MIN_WORKER_LEN ||
                   worker_len > MAX_WORKER_LEN
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_get]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info, worker, worker_len );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_get][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );
    qstat_val = ( unsigned char * ) ( m_queue_index.ptr + offset );

    redelivery_t::iterator rit = queue_info->redelivery->begin ();

    if ( unlikely ( rit != queue_info->redelivery->end () &&
                   m_current_timestamp - rit->first >= qstat->timeout * 60 )
         )
    {
        sprintf ( qkey, "%s|%s", inner_queue.c_str (), rit->second.c_str () );
        qkey_len = strlen ( qkey );

        queue_info->redelivery->erase ( rit );
    }
    else
    {
        if ( clac_real_item ( qstat->sp2, qstat->ep2 ) > 0 )
        {
            priori = 2;
            tag = ( qstat->sp2 + 1 ) % CYCLE_QUEUE_ITEM_NUM;
        }
        else if ( clac_real_item ( qstat->sp1, qstat->ep1 ) > 0 )
        {
            priori = 1;
            tag = ( qstat->sp1 + 1 ) % CYCLE_QUEUE_ITEM_NUM;
        }
        else if ( clac_real_item ( qstat->sp, qstat->ep ) > 0 )
        {
            priori = 0;
            tag = ( qstat->sp + 1 ) % CYCLE_QUEUE_ITEM_NUM;
        }
        else
        {
            return ENOENT;
        }

        fast_memcpy ( qstat_val + SIZEOF_UNIT32 * priori * 2, & tag, SIZEOF_UNIT32 );

        sprintf ( qkey, "%s|%u:%u", inner_queue.c_str (), priori, tag );
        qkey_len = strlen ( qkey );
    }

    ack = qkey;

    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    r = m_storage->get ( qkey, qkey_len, ver, conn, rsp, ctxt );
    if ( unlikely ( 0 != r ) )
    {
        LOG_ERROR ( "[hustdb][mq_get]get failed, return:%d, key:%s", r, qkey );
        return r;
    }

    if ( ! is_ack )
    {
        unacked = qkey + queue_len + 1;

        queue_info->unacked->insert ( std::pair<std::string, uint32_t>( unacked, m_current_timestamp ) );
        queue_info->redelivery->insert ( std::pair<uint32_t, std::string>( m_current_timestamp, unacked ) );
    }

    return 0;
}

int hustdb_t::hustmq_ack_inner (
                                 std::string & ack,
                                 conn_ctxt_t conn
                                 )
{
    uint32_t        ver                     = 0;
    item_ctxt_t *   ctxt                    = NULL;

    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    return m_storage->del ( ack.c_str (), ack.size (), ver, false, conn, ctxt );
}

int hustdb_t::hustmq_ack (
                           const char * queue,
                           size_t queue_len,
                           const char * ack,
                           size_t ack_len,
                           conn_ctxt_t conn
                           )
{
    int             offset                  = - 1;
    size_t          qkey_len                = 0;
    char            qkey[ MAX_QKEY_LEN ]    = { };
    uint32_t        ver                     = 0;
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    queue_info_t *  queue_info              = NULL;
    item_ctxt_t *   ctxt                    = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   ! tb_name_check ( ack, ack_len )
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_ack]params error" );
        return EKEYREJECTED;
    }

    do
    {
        qhash = m_apptool->locker_hash ( queue, queue_len );
        qlkt = m_lockers.at ( qhash );
        scope_lock_t qlocker ( * qlkt );

        std::string inner_queue ( queue, queue_len );

        offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info );
        if ( unlikely ( offset < 0 ) )
        {
            LOG_ERROR ( "[hustdb][mq_ack][queue=%s]find queue failed ", inner_queue.c_str () );
            return EPERM;
        }

        std::string inner_ack ( ack, ack_len );

        unacked_t::iterator ait = queue_info->unacked->find ( inner_ack );
        if ( ait == queue_info->unacked->end () )
        {
            LOG_ERROR ( "[hustdb][mq_ack][queue=%s][ack=%s]find queue failed ", inner_queue.c_str (), inner_ack.c_str () );
            return EPERM;
        }

        redelivery_t::iterator rit = queue_info->redelivery->find ( ait->second );
        while ( rit != queue_info->redelivery->end () )
        {
            if ( rit->first != ait->second )
            {
                break;
            }

            if ( rit->second == inner_ack )
            {
                queue_info->redelivery->erase ( rit );
                break;
            }

            rit ++;
        }

        queue_info->unacked->erase ( ait );

        sprintf ( qkey, "%s|%s", inner_queue.c_str (), inner_ack.c_str () );
        qkey_len = strlen ( qkey );
    }
    while ( 0 );

    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    return m_storage->del ( qkey, qkey_len, ver, false, conn, ctxt );
}

int hustdb_t::hustmq_worker (
                              const char * queue,
                              size_t queue_len,
                              std::string & workers
                              )
{
    int             offset                  = - 1;
    worker_t *      wmap                    = NULL;
    char            wt[ 64 ]                = { };
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    queue_info_t *  queue_info              = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ) )
    {
        LOG_DEBUG ( "[hustdb][mq_worker]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_worker][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }
    wmap = queue_info->worker;

    workers.resize ( 0 );
    workers.reserve ( 4096 );
    workers += "[";

    worker_t::iterator wit = wmap->begin ();
    while ( wit != wmap->end () )
    {
        if ( wit->second < m_current_timestamp - WORKER_TIMEOUT )
        {
            worker_t::iterator twit = wit;
            wit ++;
            wmap->erase ( twit );

            continue;
        }

        memset ( wt, 0, sizeof ( wt ) );
        sprintf ( wt,
                 "{\"w\":\"%s\",\"t\":%u},",
                 wit->first.c_str (),
                 wit->second
                 );
        workers += wt;

        wit ++;
    }

    if ( workers.size () > 2 )
    {
        workers.erase ( workers.end () - 1, workers.end () );
    }
    workers += "]";

    return 0;
}

int hustdb_t::hustmq_stat (
                            const char * queue,
                            size_t queue_len,
                            std::string & stat
                            )
{
    int             offset                  = - 1;
    queue_stat_t *  qstat                   = NULL;
    char            st[ 1024 ]              = { };
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    queue_info_t *  queue_info              = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ) )
    {
        LOG_DEBUG ( "[hustdb][mq_stat]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, COMMON_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_stat][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );

    sprintf ( st,
             "{\"queue\":\"%s\",\"ready\":[%u,%u,%u],\"unacked\":%u,\"max\":%u,\"lock\":%u,\"type\":%u,\"timeout\":%u,\"si\":%u,\"ci\":%u,\"tm\":%u}",
             inner_queue.c_str (),
             clac_real_item ( qstat->sp, qstat->ep ),
             clac_real_item ( qstat->sp1, qstat->ep1 ),
             clac_real_item ( qstat->sp2, qstat->ep2 ),
             queue_info->unacked->size (),
             qstat->max,
             qstat->lock,
             qstat->type,
             qstat->timeout,
             qstat->sp,
             qstat->ep,
             qstat->ctime
             );

    stat = st;

    return 0;
}

void hustdb_t::hustmq_stat_all (
                                 std::string & stats
                                 )
{
    queue_stat_t *  qstat                   = NULL;
    char            stat[ 1024 ]            = { };
    queue_info_t *  queue_info              = NULL;

    stats.resize ( 0 );
    stats.reserve ( 1048576 );
    stats += "[";
    
    scope_rlock_t mq_locker ( m_mq_locker );

    for ( queue_map_t::iterator it = m_queue_map.begin (); it != m_queue_map.end (); it ++ )
    {
        queue_info = & it->second;

        qstat = ( queue_stat_t * ) ( m_queue_index.ptr + queue_info->offset );

        memset ( stat, 0, sizeof ( stat ) );
        sprintf ( stat,
                 "{\"queue\":\"%s\",\"ready\":[%u,%u,%u],\"unacked\":%u,\"max\":%u,\"lock\":%u,\"type\":%u,\"timeout\":%u,\"si\":%u,\"ci\":%u,\"tm\":%u},",
                 it->first.c_str (),
                 clac_real_item ( qstat->sp, qstat->ep ),
                 clac_real_item ( qstat->sp1, qstat->ep1 ),
                 clac_real_item ( qstat->sp2, qstat->ep2 ),
                 queue_info->unacked->size (),
                 qstat->max,
                 qstat->lock,
                 qstat->type,
                 qstat->timeout,
                 qstat->sp,
                 qstat->ep,
                 qstat->ctime
                 );
        stats += stat;
    }

    if ( stats.size () > 2 )
    {
        stats.erase ( stats.end () - 1, stats.end () );
    }
    stats += "]";
}

int hustdb_t::hustmq_max (
                           const char * queue,
                           size_t queue_len,
                           uint32_t max
                           )
{
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    int             offset                  = - 1;
    queue_stat_t *  qstat                   = NULL;
    queue_info_t *  queue_info              = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   max < 0 || max > MAX_QUEUE_ITEM_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_max]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_max][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );

    qstat->max = max;

    return 0;
}

int hustdb_t::hustmq_lock (
                            const char * queue,
                            size_t queue_len,
                            uint8_t lock
                            )
{
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    int             offset                  = - 1;
    queue_stat_t *  qstat                   = NULL;
    queue_info_t *  queue_info              = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   lock != 0 && lock != 1 )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_lock]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_lock][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );

    qstat->lock = lock;

    return 0;
}

int hustdb_t::hustmq_timeout (
                               const char * queue,
                               size_t queue_len,
                               uint8_t timeout
                               )
{
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    int             offset                  = - 1;
    queue_stat_t *  qstat                   = NULL;
    queue_info_t *  queue_info              = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   timeout <= 0 || timeout > 255
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_timeout]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_timeout][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );

    qstat->timeout = timeout;

    return 0;
}

int hustdb_t::hustmq_purge (
                             const char * queue,
                             size_t queue_len,
                             uint32_t priori,
                             conn_ctxt_t conn
                             )
{
    int             r                       = 0;
    uint32_t        stag                    = 0;
    uint32_t        etag                    = 0;
    size_t          qkey_len                = 0;
    char            qkey[ MAX_QKEY_LEN ]    = { };
    uint32_t        ver                     = 0;
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    uint32_t        real                    = 0;
    int             offset                  = - 1;
    queue_stat_t *  qstat                   = NULL;
    unsigned char * qstat_val               = NULL;
    queue_info_t *  queue_info              = NULL;
    item_ctxt_t *   ctxt                    = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   priori < 0 || priori > 2
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_purge]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, QUEUE_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_purge][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );
    qstat_val = ( unsigned char * ) ( m_queue_index.ptr + offset );

    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    fast_memcpy ( & stag, qstat_val + SIZEOF_UNIT32 * priori * 2, SIZEOF_UNIT32 );
    fast_memcpy ( & etag, qstat_val + SIZEOF_UNIT32 * ( priori * 2 + 1 ), SIZEOF_UNIT32 );

    if ( stag > etag && stag > MAX_QUEUE_ITEM_NUM )
    {
        etag += CYCLE_QUEUE_ITEM_NUM;
    }

    for ( uint32_t p = stag + 1; p <= etag; p ++ )
    {
        memset ( qkey, 0, sizeof ( qkey ) );

        sprintf ( qkey, "%s|%u:%u", inner_queue.c_str (), priori, p % CYCLE_QUEUE_ITEM_NUM );
        qkey_len = strlen ( qkey );

        ver = 0;
        r = m_storage->del ( qkey, qkey_len, ver, false, conn, ctxt );
        if ( unlikely ( 0 != r ) )
        {
            continue;
        }
    }

    real = clac_real_item ( qstat->sp, qstat->ep ) + clac_real_item ( qstat->sp1, qstat->ep1 ) + clac_real_item ( qstat->sp2, qstat->ep2 ) - clac_real_item ( stag, etag );

    if ( real <= 0 )
    {
        scope_wlock_t mq_locker ( m_mq_locker );

        queue_map_t::iterator it = m_queue_map.find ( inner_queue );
        if ( it != m_queue_map.end () )
        {
            delete it->second.unacked;
            it->second.unacked = NULL;

            delete it->second.redelivery;
            it->second.redelivery = NULL;

            delete it->second.worker;
            it->second.worker = NULL;

            m_queue_map.erase ( it );
        }

        memset ( qstat_val, 0, QUEUE_STAT_LEN );
    }
    else
    {
        stag = etag = 0;
        fast_memcpy ( qstat_val + SIZEOF_UNIT32 * priori * 2, & stag, SIZEOF_UNIT32 );
        fast_memcpy ( qstat_val + SIZEOF_UNIT32 * ( priori * 2 + 1 ), & etag, SIZEOF_UNIT32 );
    }

    return 0;
}

int hustdb_t::hustmq_pub (
                           const char * queue,
                           size_t queue_len,
                           const char * item,
                           size_t item_len,
                           uint32_t idx,
                           uint32_t wttl,
                           conn_ctxt_t conn
                           )
{
    int             r                       = 0;
    size_t          qkey_len                = 0;
    char            qkey[ MAX_QKEY_LEN ]    = { };
    uint32_t        priori                  = 0;
    uint32_t        ver                     = 0;
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    uint32_t        rttl                    = 0;
    uint32_t        stag                    = 0;
    uint32_t        etag                    = 0;
    int             offset                  = - 1;
    uint32_t        real                    = 0;
    queue_stat_t *  qstat                   = NULL;
    unsigned char * qstat_val               = NULL;
    queue_info_t *  queue_info              = NULL;
    item_ctxt_t *   ctxt                    = NULL;
    std::string *   rsp                     = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   CHECK_STRING ( item ) ||
                   wttl <= 0 || wttl > m_store_conf.mq_ttl_maximum
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_pub]params error" );
        return EKEYREJECTED;
    }

    if ( unlikely ( m_over_threshold ) )
    {
        LOG_DEBUG ( "[hustdb][mq_pub]memory over threshold" );
        return ENOMEM;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, true, PUSHQ_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_pub][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );
    qstat_val = ( unsigned char * ) ( m_queue_index.ptr + offset );

    fast_memcpy ( & stag, qstat_val + SIZEOF_UNIT32 * priori * 2, SIZEOF_UNIT32 );
    fast_memcpy ( & etag, qstat_val + SIZEOF_UNIT32 * ( priori * 2 + 1 ), SIZEOF_UNIT32 );

    real = clac_real_item ( qstat->sp, qstat->ep );

    if ( qstat->lock == 1 || ( qstat->max > 0 && real >= qstat->max ) || real >= MAX_QUEUE_ITEM_NUM )
    {
        return EINVAL;
    }

    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    for ( int i = 0; i < real; i ++ )
    {
        stag = ( stag + 1 ) % CYCLE_QUEUE_ITEM_NUM;

        memset ( qkey, 0, MAX_QKEY_LEN );
        sprintf ( qkey, "%s|%u:%u", inner_queue.c_str (), priori, stag );
        qkey_len = strlen ( qkey );

        if ( idx < CYCLE_QUEUE_ITEM_NUM && clac_real_item ( stag, idx ) > 0 )
        {
            ver = 0;
            m_storage->del ( qkey, qkey_len, ver, false, conn, ctxt );

            continue;
        }

        ver = 0;
        r = m_storage->get ( qkey, qkey_len, ver, conn, rsp, ctxt );
        if ( unlikely ( 0 != r ) )
        {
            continue;
        }

        rttl = m_storage->get_inner_ttl ( conn );
        if ( rttl <= m_current_timestamp )
        {
            ver = 0;
            m_storage->del ( qkey, qkey_len, ver, false, conn, ctxt );
        }
        else
        {
            stag --;

            break;
        }
    }

    if ( idx < CYCLE_QUEUE_ITEM_NUM && clac_real_item ( etag, idx ) > 0 )
    {
        etag = idx;
        stag = ( etag + CYCLE_QUEUE_ITEM_NUM - 1 ) % CYCLE_QUEUE_ITEM_NUM;
    }
    else
    {
        etag =  ( etag + 1 ) % CYCLE_QUEUE_ITEM_NUM;
    }

    memset ( qkey, 0, MAX_QKEY_LEN );
    sprintf ( qkey, "%s|%u:%u", inner_queue.c_str (), priori, etag );
    qkey_len = strlen ( qkey );

    m_storage->set_inner_ttl ( m_current_timestamp + wttl, conn );

    ver = 0;
    r = m_storage->put ( qkey, qkey_len, item, item_len, ver, false, conn, ctxt );
    if ( unlikely ( 0 != r ) )
    {
        LOG_ERROR ( "[hustdb][mq_pub]put failed, return:%d, key:%s, item_len:%d", r, qkey, item_len );
        return r;
    }

    qstat->ctime = ( uint32_t ) m_current_timestamp;
    fast_memcpy ( qstat_val + SIZEOF_UNIT32 * priori * 2, & stag, SIZEOF_UNIT32 );
    fast_memcpy ( qstat_val + SIZEOF_UNIT32 * ( priori * 2 + 1 ), & etag, SIZEOF_UNIT32 );

    return 0;
}

int hustdb_t::hustmq_sub (
                           const char * queue,
                           size_t queue_len,
                           uint32_t idx,
                           uint32_t & sp,
                           uint32_t & ep,
                           std::string * & rsp,
                           conn_ctxt_t conn
                           )
{
    int             r                       = 0;
    size_t          qkey_len                = 0;
    char            qkey[ MAX_QKEY_LEN ]    = { };
    uint32_t        priori                  = 0;
    uint32_t        ver                     = 0;
    uint16_t        qhash                   = 0;
    lockable_t *    qlkt                    = NULL;
    uint32_t        rttl                    = 0;
    int             offset                  = - 1;
    queue_stat_t *  qstat                   = NULL;
    queue_info_t *  queue_info              = NULL;
    item_ctxt_t *   ctxt                    = NULL;

    if ( unlikely ( ! tb_name_check ( queue, queue_len ) ||
                   idx >= CYCLE_QUEUE_ITEM_NUM
                   )
         )
    {
        LOG_DEBUG ( "[hustdb][mq_pub]params error" );
        return EKEYREJECTED;
    }

    qhash = m_apptool->locker_hash ( queue, queue_len );
    qlkt = m_lockers.at ( qhash );
    scope_lock_t qlocker ( * qlkt );

    std::string inner_queue ( queue, queue_len );

    offset = find_queue_offset ( inner_queue, false, PUSHQ_TB, queue_info );
    if ( unlikely ( offset < 0 ) )
    {
        LOG_ERROR ( "[hustdb][mq_sub][queue=%s]find queue failed ", inner_queue.c_str () );
        return EPERM;
    }

    qstat = ( queue_stat_t * ) ( m_queue_index.ptr + offset );

    if ( clac_real_item ( qstat->sp, qstat->ep ) <= 0 )
    {
        return ENOENT;
    }

    sp = ep = 0;
    if ( clac_real_item ( qstat->sp, idx ) <= 0 || clac_real_item ( ( idx + CYCLE_QUEUE_ITEM_NUM - 1 ) % CYCLE_QUEUE_ITEM_NUM, qstat->ep ) <= 0 )
    {
        sp = qstat->sp;
        ep = qstat->ep;

        return EINVAL;
    }

    sprintf ( qkey, "%s|%u:%u", inner_queue.c_str (), priori, idx );
    qkey_len = strlen ( qkey );

    m_storage->set_inner_table ( NULL, 0, QUEUE_TB, conn );

    ver = 0;
    r = m_storage->get ( qkey, qkey_len, ver, conn, rsp, ctxt );
    if ( unlikely ( 0 != r ) )
    {
        LOG_ERROR ( "[hustdb][mq_sub]get failed, return:%d, key:%s", r, qkey );
        return r;
    }

    rttl = m_storage->get_inner_ttl ( conn );
    if ( rttl <= m_current_timestamp )
    {
        ver = 0;
        m_storage->del ( qkey, qkey_len, ver, false, conn, ctxt );

        return ENOENT;
    }

    return 0;
}

static void timestamp_cb (
                           void * ctx
                           )
{
    if ( unlikely ( ! ctx ) )
    {
        return ;
    }

    hustdb_t * db = ( hustdb_t * ) ctx;

    struct timeval tv;
    gettimeofday ( & tv, NULL );

    db->set_current_timestamp ( tv.tv_sec );

    db->get_mdb ()->set_mdb_timestamp ( tv.tv_sec );
}

static void over_threshold_cb (
                                void * ctx
                                )
{
    if ( unlikely ( ! ctx ) )
    {
        return ;
    }

    hustdb_t * db = ( hustdb_t * ) ctx;

    if ( db->get_server_conf ().memory_process_threshold > 0 ||
         db->get_server_conf ().memory_system_threshold > 0
         )
    {
        db->hustdb_memory_threshold ( );
    }
}

static void ttl_scan_cb (
                          void * ctx
                          )
{
    if ( unlikely ( ! ctx ) )
    {
        return ;
    }

    hustdb_t * db = ( hustdb_t * ) ctx;
    db->hustdb_ttl_scan ( );
}

static void binlog_scan_cb (
                             void * ctx
                             )
{
    if ( unlikely ( ! ctx ) )
    {
        return ;
    }

    hustdb_t * db = ( hustdb_t * ) ctx;
    db->hustdb_binlog_scan ( );
}
