#ifndef __timer_20160727104432_h__
#define __timer_20160727104432_h__

#include <stdint.h>
#include <vector>
#include <pthread.h>

namespace hustdb { \

typedef void (*task_func_t)(void * ctx);

class timer_task_t
{
public:
    timer_task_t();
    timer_task_t(int interval, task_func_t task_func, void * ctx);
    void operator()();
    int interval() const { return m_interval; }
private:
    int m_interval; // second
    task_func_t m_task_func;
    void * m_ctx;
};

class async_tasks_t;

class timer_t
{
public:
    timer_t();
    ~timer_t();

    bool open();
    void kill_me();

    bool register_task(const timer_task_t& task);
private:
    static void * timer_thread(void * arg);
    void run();
private:
    std::vector<timer_task_t> m_tasks;
    pthread_t m_tid;
    int m_timeout;
    int m_stop_pipe[2];
    async_tasks_t * m_async_tasks;
};

}

#endif // __timer_20160727104432_h__
