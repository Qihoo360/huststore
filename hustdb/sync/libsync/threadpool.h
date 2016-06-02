#pragma once

#include <pthread.h>
#include <unistd.h>
#include <deque>
#include <vector>
#include <cerrno>
#include <cstring>

#include "mutex.h"
#include "task.h"
#include "cond.h"

#include "husthttp.h"

const int DEFAULT_POOL_SIZE = 10;
const int STARTED = 0;
const int STOPPED = 1;

class ThreadPool
{
public:
    ThreadPool ( );
    ThreadPool ( int size );
    ~ThreadPool ( );
    int init_threadpool ( );
    int destroy_threadpool ( );
    void* exec_thread ( int );
    int add_task ( Task *task );
private:
    int _size;
    Mutex _task_mutex;
    Condition _task_cond;
    std::vector<pthread_t> _threads;
    std::deque<Task*> _tasks;
    std::vector<husthttp_t*> clients;
    volatile int _state;
};
