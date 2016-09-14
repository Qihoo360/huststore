#ifndef __HUSTSTORE_BINLOG_THREAD_H_
#define __HUSTSTORE_BINLOG_THREAD_H_

#include <pthread.h>

typedef void * ( * thread_func_t ) ( void * );

class thread_t
{
public:
    thread_t ( thread_func_t thread_func );
    ~thread_t ( );

    bool init ( );

    bool start ( );
    bool stop ( );

private:
    bool _running;
    pthread_t _tid;
    thread_func_t _thread_func;
    void * _data;
    int * _stop_pipe;
};

#endif