#include "../lib/threadpool.h"
#include <iostream>
#include <cstdlib>

typedef struct
{
    ThreadPool *pool;
    int client_id;
} Thread;

ThreadPool::ThreadPool ( )	\
 : _size ( DEFAULT_POOL_SIZE )
{
}

ThreadPool::ThreadPool ( int size )	\
 : _size ( size )
{
}

ThreadPool::~ ThreadPool ( )
{
    if ( _state != STOPPED )
    {
        destroy_threadpool ();
    }
}

extern "C"
void* start_thread ( void *arg )
{
    Thread *thread = ( Thread* ) arg;
    ThreadPool *tp = ( ThreadPool* ) thread->pool;
    tp->exec_thread (thread->client_id);
    delete thread;
    return NULL;
}

int ThreadPool::init_threadpool ( )
{
    _state = STARTED;
    int ret = - 1;
    for ( int i = 0; i < _size; i ++ )
    {
        pthread_t tid;
        Thread *thread = new Thread ();
        thread->pool = this;
        thread->client_id = i;

        husthttp_t *client = new husthttp_t ();
        clients.push_back (client);

        ret = pthread_create (&tid, NULL, start_thread, ( void* ) thread);
        if ( ret != 0 )
        {
            std::cerr << "pthread_create failed" << std::endl;
            return - 1;
        }
        _threads.push_back (tid);

    }

    return 0;
}

int ThreadPool::destroy_threadpool ( )
{
    _task_mutex.lock ();
    _state = STOPPED;
    _task_mutex.unlock ();

    //std::cout << "Broadcasting STOP signal to all threads." << std::endl;
    _task_cond.broadcast ();

    int ret = - 1;
    for ( int i = 0; i < _size; i ++ )
    {
        void *result;
        ret = pthread_join (_threads[i], &result);
        delete clients[i];
        //std::cout << "pthread_join returned" << ret << ": " << strerror(errno) << std::endl;
        _task_cond.broadcast ();
    }
    return 0;
}

void *ThreadPool::exec_thread ( int client_id )
{
    Task *task = NULL;
    while ( true )
    {
        _task_mutex.lock ();
        while ( ( _state != STOPPED ) && _tasks.empty () )
        {
            _task_cond.wait (_task_mutex.get_mutex_ptr ());
        }
        if ( _state == STOPPED )
        {
            _task_mutex.unlock ();
            pthread_exit (NULL);
        }
        task = _tasks.front ();
        _tasks.pop_front ();
        _task_mutex.unlock ();
        if ( ( *task )( clients[client_id] ) )
        {
            delete task;
        }
        else
        {
            this->add_task (task);
        }
    }
    return NULL;
}

int ThreadPool::add_task ( Task *task )
{
    _task_mutex.lock ();
    _tasks.push_back (task);
    _task_cond.signal ();
    _task_mutex.unlock ();
    return 0;
}
