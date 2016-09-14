#include "mutex.h"

mutex_lock_t::mutex_lock_t ( )
{
    pthread_mutex_init ( &_mutex, NULL );
}

mutex_lock_t::~ mutex_lock_t ( )
{
    pthread_mutex_destroy ( &_mutex );
}

void mutex_lock_t::lock ( )
{
    pthread_mutex_lock ( &_mutex );
}

void mutex_lock_t::unlock ( )
{
    pthread_mutex_unlock ( &_mutex );
}

pthread_mutex_t * mutex_lock_t::get_mutex_ptr ( )
{
    return &_mutex;
}

mutex_lock_guard_t::mutex_lock_guard_t ( mutex_lock_t & mutex ) : _mutex ( mutex )
{
    _mutex.lock ();
}

mutex_lock_guard_t::~ mutex_lock_guard_t ( )
{
    _mutex.unlock ();
}

rw_lock_t::rw_lock_t ( )
{
    pthread_rwlock_init ( &_rwlock, NULL );
}

rw_lock_t::~ rw_lock_t ( )
{
    pthread_rwlock_destroy ( &_rwlock );
}

void rw_lock_t::rdlock ( )
{
    pthread_rwlock_rdlock ( &_rwlock );
}

void rw_lock_t::wrlock ( )
{
    pthread_rwlock_wrlock ( &_rwlock );
}

void rw_lock_t::unlock ( )
{
    pthread_rwlock_unlock ( &_rwlock );
}

rw_lock_guard_t::rw_lock_guard_t ( rw_lock_t & rwlock, int flag )
: _rwlock ( rwlock )
{
    if ( flag == 0 )
    {
        _rwlock.rdlock ();
    }
    else
    {
        _rwlock.wrlock ();
    }
}

rw_lock_guard_t::~ rw_lock_guard_t ( )
{
    _rwlock.unlock ();
}