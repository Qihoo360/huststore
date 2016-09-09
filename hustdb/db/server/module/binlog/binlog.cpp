#include <string>
#include <pthread.h>

#include <iostream>

#include "binlog.h"
#include "singleton.h"
#include "thread_pool.h"
#include "host_info.h"
#include "task.h"

binlog_t::binlog_t (
    const std::string & username,
    const std::string & password )
    : _pool ( NULL )
    , _callback_func ( NULL )
{
    _auth.assign ( username );
    _auth.push_back ( ':' );
    _auth.append ( password );
}

binlog_t::~binlog_t()
{
    host_info_t & host_info = singleton_t<host_info_t>::instance();
    host_info.kill_me();
    delete _pool;
    _pool = NULL;
}

bool binlog_t::init ( int num_threads, size_t max_queue_size, size_t max_backup_queue_size )
{
    _pool = new thread_pool_t ( num_threads, max_queue_size );

    if ( _pool == NULL ) {
        return false;
    }

    if ( !_pool->init_threadpool() ) {
        return false;
    }

    host_info_t & host_info = singleton_t<host_info_t>::instance();

    if ( !host_info.init ( _pool, max_backup_queue_size ) ) {
        return false;
    }

    return true;
}

void binlog_t::set_callback ( callback_func_t callback_func )
{
    _callback_func = callback_func;
}

bool binlog_t::add_task (
    const char * host,
    size_t host_len,
    const char * table,
    size_t table_len,
    const char * key,
    size_t key_len,
    const char * value,
    size_t value_len,
    uint32_t ver,
    uint32_t ttl,
    uint64_t score,
    int8_t opt,
    uint8_t cmd_type,
    void * param )
{
    task_t * task = new task_t ( host, host_len, _callback_func, param, _auth );
    task->make_task (
        host,
        host_len,
        table,
        table_len,
        key,
        key_len,
        value,
        value_len,
        ver,
        ttl,
        score,
        opt,
        cmd_type
    );

    host_info_t & host_info = singleton_t<host_info_t>::instance();

    if ( !host_info.has_host ( host ) || !host_info.is_alive ( host ) ) {
        return false;
    }

    if ( _pool->add_task ( task ) ) {
        host_info.increment_with_lock ( host );
        return true;
    } else {
        return host_info.add_task ( host, task, true );
    }

    return false;
}

void binlog_t::get_alive_hosts ( std::vector<std::string> & lists )
{
    host_info_t & host_info = singleton_t<host_info_t>::instance();
    host_info.get_alive_hosts ( lists );
}

bool binlog_t::add_host ( const std::string & host )
{
    host_info_t & host_info = singleton_t<host_info_t>::instance();
    return host_info.add_host ( host );
}

bool binlog_t::remove_host ( const std::string & host )
{
    host_info_t & host_info = singleton_t<host_info_t>::instance();
    return host_info.remove_host ( host );
}
