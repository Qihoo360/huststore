#include <sys/timerfd.h>
#include <poll.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <algorithm>

#include "host_info.h"
#include "queue.h"
#include "task.h"
#include "thread_pool.h"
#include "husthttp.h"
#include "thread.h"
#include "singleton.h"
#include "binlog_time.h"

static void * check_alive ( void * arg )
{
    host_info_t & host_info = singleton_t<host_info_t>::instance ();

    int stop_pfd = * ( int * ) arg;
    int timerfd = timerfd_create ( CLOCK_REALTIME, 0 );
    int redeliver_fd = timerfd_create ( CLOCK_REALTIME, 0 );
    int gc_fd = timerfd_create ( CLOCK_REALTIME, 0 );

    if ( timerfd == - 1 || redeliver_fd == - 1 || gc_fd == - 1 )
    {
        return ( void * ) NULL;
    }

    struct itimerspec check_tv, redeliver_tv, gc_tv;

    memset ( &check_tv, 0, sizeof ( check_tv ) );
    memset ( &redeliver_tv, 0, sizeof ( redeliver_tv ) );
    memset ( &gc_tv, 0, sizeof ( gc_tv ) );

    check_tv.it_value.tv_sec           = 1;
    check_tv.it_interval.tv_sec        = 6;

    redeliver_tv.it_value.tv_sec       = 1;
    redeliver_tv.it_interval.tv_sec    = 1;

    gc_tv.it_value.tv_sec              = 1;
    gc_tv.it_interval.tv_sec           = 300;

    if ( timerfd_settime ( timerfd, 0, &check_tv, NULL ) == - 1
         || timerfd_settime ( redeliver_fd, 0, &redeliver_tv, NULL ) == - 1
         || timerfd_settime ( gc_fd, 0, &gc_tv, NULL ) == - 1 )
    {
        return ( void * ) NULL;
    }

    struct pollfd pfd[4];

    pfd[0].fd        = timerfd;
    pfd[0].events    = POLLIN;

    pfd[1].fd        = redeliver_fd;
    pfd[1].events    = POLLIN;

    pfd[2].fd        = gc_fd;
    pfd[2].events    = POLLIN;

    pfd[3].fd        = stop_pfd;
    pfd[3].events    = POLLIN;

    int ret = - 1;

    uint64_t val;

    while ( 1 )
    {
        ret = poll ( pfd, 4, - 1 );

        if ( ret == - 1 )
        {
            continue;
        }

        if ( pfd[0].revents & POLLIN )
        {
            read ( timerfd, &val, sizeof ( val ) );
            host_info.check_db ();
        }

        if ( pfd[1].revents & POLLIN )
        {
            binlog_time_t::set_current_time ( );
            read ( redeliver_fd, &val, sizeof ( val ) );
            host_info.redeliver ();
        }

        if ( pfd[2].revents & POLLIN )
        {
            read ( gc_fd, &val, sizeof ( val ) );
            host_info.check_silence_and_remove_host ( );
        }

        if ( pfd[3].revents & POLLIN )
        {
            char buf;
            read ( stop_pfd, &buf, 1 );
            break;
        }
    }

    return ( void * ) NULL;
}

host_info_t::host_info_t ( )
: _rwlock ( )
, _redeliver_size ( 500 )
, _max_queue_size ( 0 )
, _tp ( NULL )
, _client ( NULL )
, _thread ( NULL )
, _gc_pool ( )
, _silence_limit ( 3600 )
, _cursor ( 0 )
{
}

host_info_t::~ host_info_t ( )
{
    rw_lock_guard_t lock ( _rwlock, WLOCK );

    for ( std::map<std::string, queue_t *>::iterator it = _queue.begin (); it != _queue.end (); ++ it )
    {
        delete it->second;
    }

    _tp = NULL;

}

bool host_info_t::init ( thread_pool_t * tp, size_t max_queue_size )
{
    _tp = tp;
    _max_queue_size = max_queue_size;
    _client = new husthttp_t ();

    if ( _client == NULL )
    {
        return false;
    }

    _thread = new thread_t ( check_alive );

    if ( _thread == NULL || ! _thread->init () || ! _thread->start () )
    {
        return false;
    }

    return true;
}

bool host_info_t::is_alive ( const std::string & host )
{
    return get_status ( host ) == 1;
}

bool host_info_t::add_task ( const std::string & host, task_t * task, bool with_check )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );
    std::map<std::string, queue_t *>::iterator it;

    if ( ( it = _queue.find ( host ) ) == _queue.end () )
    {
        return false;
    }
    else
    {
        if ( ! with_check )
        {
            return it->second->put ( task );
        }
        else
        {
            if ( is_alive ( host ) && it->second->put_with_check ( task ) )
            {
                increment ( host );
                return true;
            }
        }
    }

    return false;
}

void host_info_t::finish_task ( const std::string & host )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );
    std::map<std::string, binlog_status_t>::iterator it;

    if ( ( it = _status.find ( host ) ) == _status.end () )
    {
        return;
    }

    decrement ( host );
}

bool host_info_t::add_host ( const std::string & host )
{
    do
    {
        rw_lock_guard_t lock ( _rwlock, RLOCK );
        if ( _queue.size () >= 1024 )
        {
            return false;
        }
        
        if ( _queue.find ( host ) != _queue.end () )
        {
            return true;
        }
    }
    while ( 0 );

    do
    {
        rw_lock_guard_t lock ( _rwlock, WLOCK );
        queue_t * queue = new queue_t ( _max_queue_size );

        if ( queue == NULL )
        {
            return false;
        }

        _queue[host] = queue;

        _status[host] = binlog_status_t ();
    }
    while ( 0 );

    return true;
}

bool host_info_t::has_host ( const std::string & host )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );
    return ! ( _queue.find ( host ) == _queue.end () );
}

bool host_info_t::remove_host ( const std::string & host )
{
    rw_lock_guard_t lock ( _rwlock, WLOCK );
    return inner_remove_host ( host );
}

bool host_info_t::inner_remove_host ( const std::string & host )
{
    std::map<std::string, queue_t *>::iterator it;

    if ( ( it = _queue.find ( host ) ) == _queue.end () )
    {
        return true;
    }

    delete _queue[host];
    _queue.erase ( it );

    std::map<std::string, binlog_status_t>::iterator status_it;

    if ( ( status_it = _status.find ( host ) ) == _status.end () )
    {
        return true;
    }

    _status.erase ( status_it );

    return true;
}

void host_info_t::check_silence_and_remove_host ( )
{
    _gc_pool.clear ( );
    rw_lock_guard_t lock ( _rwlock, WLOCK );
    
    for ( std::map<std::string, queue_t *>::iterator it = _queue.begin (); it != _queue.end ( ); ++ it )
    {
        if ( _status[it->first].silence.get ( ) >= _silence_limit )
        {
            _gc_pool.push_back ( it->first );
        }
    }

    size_t gc_size = _gc_pool.size ( );
    for ( size_t i = 0; i < gc_size; i ++ )
    {
        inner_remove_host ( _gc_pool[i] );
    }
}

void host_info_t::get_alives ( std::map<std::string, char> & alives )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );

    for ( std::map<std::string, binlog_status_t>::iterator it = _status.begin (); it != _status.end (); ++ it )
    {
        if ( is_alive ( it->first ) && it->second.remain.get () == 0 )
        {
            alives.insert ( std::pair<std::string, char>( it->first, '+' ) );
        }
        else
        {
            alives.insert ( std::pair<std::string, char>( it->first, '-' ) );
        }
    }
}

bool host_info_t::redeliver_with_host ( const std::string & host )
{
    std::map<std::string, queue_t *>::iterator it;

    if ( ( it = _queue.find ( host ) ) == _queue.end () || ! is_alive ( host ) )
    {
        return false;
    }

    for ( size_t i = 0; i < _redeliver_size; i ++ )
    {
        task_t * task = it->second->take ();

        if ( task == NULL )
        {
            break;
        }

        if ( ! _tp->add_task ( task ) )
        {
            return it->second->put ( task );
        }
    }

    return true;
}

void host_info_t::set_status ( const std::string & host, int status )
{
    _status[host].status = status;
}

int host_info_t::get_status ( const std::string & host )
{
    return _status[host].status;
}

void host_info_t::increment ( const std::string & host )
{
    _status[host].remain.increment ( );
    _status[host].silence.get_and_set ( 0 );
}

void host_info_t::decrement ( const std::string & host )
{
    _status[host].remain.decrement ( );
}

void host_info_t::check_db ( )
{
    int          http_code    = 0;
    const char * method       = "GET";
    const char * path         = "/status.html";
    std::string  head;
    std::string  body;
    std::string  host;

    rw_lock_guard_t lock ( _rwlock, RLOCK );
    
    if ( _queue.empty () )
    {
        _cursor = 0;
        return;
    }

    std::map<std::string, queue_t *>::iterator it = _queue.begin ( );
    std::advance ( it, _cursor );
    int dist = std::distance ( it, _queue.end ( ) );
    int size = std::min ( dist, 5 );
    int remain = 5 - size;

    for ( int i = 0; i < size && it != _queue.end ( ); ++ i, ++ it )
    {
        host.assign ( it->first );
        inner_check_db ( host, method, path, head, body, http_code );
    }

    if ( remain != 0 )
    {
        it = _queue.begin ( );
        for ( int i = 0; i < remain && it != _queue.end ( ); ++ i, ++ it )
        {
            host.assign ( it->first );
            inner_check_db ( host, method, path, head, body, http_code );
        }
    }

    _cursor = std::distance ( _queue.begin ( ), it );
}

void host_info_t::inner_check_db ( std::string & host, const char * method, const char * path, std::string & head, std::string & body, int & http_code )
{
    int proc_ret;

    _client->set_host ( host.c_str (), host.size () );

    if ( ! _client->open ( method, path, NULL, NULL, 0, 1, 1 ) )
    {
        set_status ( host, 0 );
        return;
    }

    if ( ! _client->process ( & proc_ret ) )
    {
        set_status ( host, 0 );
        return;
    }

    _client->get_response ( body, head, http_code );

    if ( http_code == 200 && strncmp ( body.c_str (), "ok\n", 3 ) == 0 )
    {
        set_status ( host, 1 );
    }
    else
    {
        set_status ( host, 0 );
    }
}

void host_info_t::redeliver ( )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );

    for ( std::map<std::string, queue_t *>::iterator it = _queue.begin (); it != _queue.end (); ++ it )
    {
        if ( _status[it->first].remain.get ( ) == 0 )
        {
            _status[it->first].silence.increment ( );
            continue;
        }
        
        redeliver_with_host ( it->first );
    }
}

void host_info_t::increment_with_lock ( const std::string & host )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );

    if ( _status.find ( host ) != _status.end () )
    {
        increment ( host );
    }
}

void host_info_t::kill_me ( )
{
    rw_lock_guard_t lock ( _rwlock, WLOCK );

    if ( _thread )
    {
        delete _thread;
        _thread = NULL;
    }

    if ( _client )
    {
        delete _client;
        _client = NULL;
    }
}

void host_info_t::queue_info ( std::string & res )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );

    for ( std::map<std::string, binlog_status_t>::iterator it = _status.begin ( ); it != _status.end ( ); ++ it )
    {
        char info [ 128 ] = { };
        sprintf ( info, "[%s->%d]", it->first.c_str ( ), it->second.remain.get ( ) );
        res.append ( info );
    }
}