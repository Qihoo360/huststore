#pragma once

#include <pthread.h>
#include <unistd.h>

class Mutex
{
public:
    Mutex ( );
    ~Mutex ( );
    void lock ( );
    void unlock ( );
    pthread_mutex_t *get_mutex_ptr ( );
private:
    pthread_mutex_t _lock;
    volatile bool is_locked;
};