#include "condition.h"

condition_t::condition_t ( mutex_lock_t & mutex )
    : _mutex ( mutex )
{
    pthread_cond_init ( &_cond, NULL );
}

condition_t::~condition_t()
{
    pthread_cond_destroy ( &_cond );
}

void condition_t::wait()
{
    //mutex_lock_guard_t lock(_mutex);
    pthread_cond_wait ( &_cond, _mutex.get_mutex_ptr() );
}

void condition_t::notify()
{
    pthread_cond_signal ( &_cond );
}

void condition_t::notify_all()
{
    pthread_cond_broadcast ( &_cond );
}