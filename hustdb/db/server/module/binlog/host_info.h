#ifndef __HUSTSTORE_BINLOG_HOSTINFO_H_
#define __HUSTSTORE_BINLOG_HOSTINFO_H_

#include <vector>
#include <string>
#include <map>

#include "atomic2.h"
#include "mutex.h"

class queue_t;
class task_t;
class thread_pool_t;
class husthttp_t;
class thread_t;

struct binlog_status_t
{

    binlog_status_t ( ) : status ( 0 ), remain ( ), silence ( )
    {
    }

    int32_t status;
    atomic_int32_t remain;
    atomic_int32_t silence;
};

class host_info_t
{
public:
    host_info_t ( );
    ~host_info_t ( );

    bool init ( thread_pool_t * tp, size_t redeliver_size );

    bool is_alive ( const std::string & host );
    bool add_task ( const std::string & host, task_t * task, bool with_check );
    void finish_task ( const std::string & host );
    bool add_host ( const std::string & host );
    bool has_host ( const std::string & host );
    bool remove_host ( const std::string & host );

    void get_alives ( std::map<std::string, char> & alives );

    void check_db ( );
    void redeliver ( );

    void increment_with_lock ( const std::string & host );

    void kill_me ( );

    void queue_info ( std::string & res );

    void check_silence_and_remove_host ( );

private:

    host_info_t ( const host_info_t & );
    const host_info_t & operator= ( const host_info_t & );

    void set_status ( const std::string & host, int status );
    int get_status ( const std::string & host );

    void increment ( const std::string & host );
    void decrement ( const std::string & host );

    bool redeliver_with_host ( const std::string & host );
    bool inner_remove_host ( const std::string & host ); 

    void inner_check_db ( std::string & host, const char * method, const char * path, std::string & head, std::string & body, int & http_code );

    std::map<std::string, binlog_status_t> _status;
    std::map<std::string, queue_t *> _queue;

    rw_lock_t _rwlock;

    size_t _redeliver_size;
    size_t _max_queue_size;
    thread_pool_t * _tp;
    husthttp_t * _client;
    thread_t * _thread;

    std::vector<std::string> _gc_pool; 
    int32_t _silence_limit;

    int _cursor;
};

#endif