#ifndef __HUSTSTORE_BINLOG_THREADPOOL_H_
#define __HUSTSTORE_BINLOG_THREADPOOL_H_

#include <pthread.h>
#include <deque>
#include <vector>

#include "mutex.h"
#include "condition.h"

class husthttp_t;
class task_t;

class thread_pool_t
{
public:
    thread_pool_t ( );
    thread_pool_t ( int num_threads, size_t max_queue_size );
    ~thread_pool_t ( );

    bool init_threadpool ( );
    void exec_thread ( int client_id );
    void destroy_threadpool ( );

    bool add_task ( task_t * task );
    task_t * take_task ( );

    size_t queue_size ( ) const;

private:
    thread_pool_t ( const thread_pool_t & );
    const thread_pool_t & operator= ( const thread_pool_t & );

    bool is_full ( );

    int _num_threads;
    bool _running;
    size_t _max_queue_size;

    std::vector<pthread_t> _threads;
    std::vector<husthttp_t *> _clients;

    mutable mutex_lock_t _mutex;
    condition_t _not_empty;
    condition_t _not_full;

    std::deque<task_t *> _queue;
};

#endif