#ifndef __HUSTSTORE_BINLOG_MUTEX_H_
#define __HUSTSTORE_BINLOG_MUTEX_H_

#include <pthread.h>

class mutex_lock_t
{
public:
    mutex_lock_t();
    ~mutex_lock_t();
    void lock();
    void unlock();
    pthread_mutex_t * get_mutex_ptr();
private:
    pthread_mutex_t _mutex;
};

class mutex_lock_guard_t
{
public:
    explicit mutex_lock_guard_t ( mutex_lock_t & mutex );
    ~mutex_lock_guard_t();
private:
    mutex_lock_t & _mutex;
};

class rw_lock_t
{
public:
    rw_lock_t();
    ~rw_lock_t();
    void rdlock();
    void wrlock();
    void unlock();
private:
    pthread_rwlock_t _rwlock;
};

class rw_lock_guard_t
{
public:
    explicit rw_lock_guard_t ( rw_lock_t & rwlock, int flag );
    ~rw_lock_guard_t();
private:
    rw_lock_t & _rwlock;
};

#endif