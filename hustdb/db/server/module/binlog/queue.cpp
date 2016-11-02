#include "queue.h"
#include "task.h"

queue_t::queue_t ( size_t max_queue_size )
: _max_queue_size ( max_queue_size )
, _mutex ( )
, _cond ( _mutex )
, _queue ( )
{
}

queue_t::~ queue_t ( )
{
    while ( !_queue.empty ( ) ) 
    {
        task_t * task = _queue.front ( );
        _queue.pop_front ( );
        delete task;
    }
}

bool queue_t::put_with_check ( task_t * task )
{
    mutex_lock_guard_t lock ( _mutex );

    if ( is_full () )
    {
        return false;
    }

    _queue.push_back ( task );
    //_cond.notify();
    return true;
}

bool queue_t::put ( task_t * task )
{
    mutex_lock_guard_t lock ( _mutex );
    _queue.push_back ( task );
    //_cond.notify();
    return true;
}

task_t * queue_t::take ( )
{
    mutex_lock_guard_t lock ( _mutex );

    if ( _queue.empty () )
    {
        return NULL;
    }

    task_t * task = _queue.front ();
    _queue.pop_front ();
    return task;
}

size_t queue_t::size ( ) const
{
    return _queue.size ();
}

bool queue_t::is_full ( )
{
    return _max_queue_size > 0 && _max_queue_size <= size ();
}