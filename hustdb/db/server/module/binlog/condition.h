#ifndef __HUSTSTORE_BINLOG_CONDITION_H_
#define __HUSTSTORE_BINLOG_CONDITION_H_

#include "mutex.h"

#include <pthread.h>

class condition_t {
public:
    explicit condition_t(mutex_lock_t & mutex);
    ~condition_t();

    void wait();
    void notify();
    void notify_all();

private:
    mutex_lock_t & _mutex;
    pthread_cond_t _cond;
};

#endif