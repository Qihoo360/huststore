#include "mutex.h"

Mutex::Mutex ( )
{
    pthread_mutex_init (&_lock, NULL);
    is_locked = false;
}

Mutex::~ Mutex ( )
{
    while ( is_locked );
    unlock ();
    pthread_mutex_destroy (&_lock);
}

void Mutex::lock ( )
{
    pthread_mutex_lock (&_lock);
    is_locked = true;
}

void Mutex::unlock ( )
{
    is_locked = false;
    pthread_mutex_unlock (&_lock);
}

pthread_mutex_t* Mutex::get_mutex_ptr ( )
{
    return &_lock;
}
