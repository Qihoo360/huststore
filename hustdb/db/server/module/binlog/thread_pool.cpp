#include "thread_pool.h"
#include "husthttp.h"
#include "task.h"

#include <iostream>

typedef struct
{
    thread_pool_t * pool;
    int client_id;
} thread_t;

static void * start_thread ( void * arg )
{
    thread_t * thread = static_cast < thread_t * > ( arg );
    thread_pool_t * tp = static_cast < thread_pool_t * > ( thread->pool );
    tp->exec_thread ( thread->client_id );
    delete thread;
    return NULL;
}

thread_pool_t::thread_pool_t ( )
: _num_threads ( 4 )
, _running ( false )
, _max_queue_size ( 0 )
, _mutex ( )
, _not_empty ( _mutex )
, _not_full ( _mutex )
{
}

thread_pool_t::thread_pool_t ( int num_threads, size_t max_queue_size )
: _num_threads ( num_threads )
, _running ( false )
, _max_queue_size ( max_queue_size )
, _mutex ( )
, _not_empty ( _mutex )
, _not_full ( _mutex )
{
}

thread_pool_t::~ thread_pool_t ( )
{
    if ( _running )
    {
        destroy_threadpool ();
    }
}

bool thread_pool_t::init_threadpool ( )
{
    _running = true;

    for ( int i = 0; i < _num_threads; i ++ )
    {
        thread_t * thread = new thread_t ();
        thread->pool = this;
        thread->client_id = i;

        husthttp_t * client = new husthttp_t ();
        _clients.push_back ( client );

        pthread_t tid;
        int ret = - 1;

        if ( ( ret = pthread_create ( &tid, NULL, start_thread, ( void * ) thread ) ) != 0 )
        {
            std::cerr << "pthread_create failed" << std::endl;
            return false;
        }

        _threads.push_back ( tid );
    }

    return true;
}

void thread_pool_t::exec_thread ( int client_id )
{
    while ( _running )
    {
        task_t * task = take_task ();

        if ( task && task->run ( _clients[client_id] ) )
        {
            delete task;
            task = NULL;
        }
    }
}

void thread_pool_t::destroy_threadpool ( )
{
    do
    {
        mutex_lock_guard_t lock ( _mutex );
        _running = false;
        _not_empty.notify_all ();
    }
    while ( 0 );

    for ( int i = 0; i < _num_threads; i ++ )
    {
        int ret = pthread_join ( _threads[i], NULL );

        if ( ret == 0 )
        {
            delete _clients[i];
        }
        else
        {
            std::cerr << "pthread_join error" << std::endl;
        }
    }

    task_t * task = NULL;

    while ( ! _queue.empty () )
    {
        task = _queue.front ();
        _queue.pop_front ();

        if ( task )
        {
            delete task;
        }
    }
}

bool thread_pool_t::add_task ( task_t * task )
{
    mutex_lock_guard_t lock ( _mutex );

    if ( is_full () )
    {
        return false;
    }

    _queue.push_back ( task );
    _not_empty.notify ();
    return true;
}

task_t * thread_pool_t::take_task ( )
{
    mutex_lock_guard_t lock ( _mutex );

    while ( _queue.empty () && _running )
    {
        _not_empty.wait ();
    }

    task_t * task = NULL;

    if ( ! _queue.empty () )
    {
        task = _queue.front ();
        _queue.pop_front ();

        if ( _max_queue_size > 0 )
        {
            _not_full.notify ();
        }
    }

    return task;
}

size_t thread_pool_t::queue_size ( ) const
{
    mutex_lock_guard_t lock ( _mutex );
    return _queue.size ();
}

bool thread_pool_t::is_full ( )
{
    return _max_queue_size > 0 && _queue.size () >= _max_queue_size;
}
