#include "thread.h"

#include <unistd.h>
#include <cstdlib>

thread_t::thread_t ( thread_func_t thread_func )
: _running ( false )
, _tid ( 0 )
, _thread_func ( thread_func )
, _data ( NULL )
, _stop_pipe ( NULL )
{
}

bool thread_t::init ( )
{
    _stop_pipe = ( int * ) calloc ( 2, sizeof ( int ) );

    if ( _stop_pipe == NULL )
    {
        return false;
    }

    if ( pipe ( _stop_pipe ) != 0 )
    {
        return false;
    }

    _data = ( void * ) &_stop_pipe[0];

    return true;
}

thread_t::~ thread_t ( )
{
    if ( _running )
    {
        stop ();
    }
}

bool thread_t::start ( )
{
    int ret = - 1;

    if ( ( ret = pthread_create ( &_tid, NULL, _thread_func, _data ) ) != 0 )
    {
        return false;
    }

    _running = true;
    return true;
}

bool thread_t::stop ( )
{
    write ( _stop_pipe[1], "s", 1 );
    _running = false;

    if ( pthread_join ( _tid, NULL ) != 0 )
    {
        return false;
    }

    free ( _stop_pipe );
    return true;
}