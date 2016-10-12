#include <sys/timerfd.h>
#include <poll.h>
#include <time.h>
#include <stdint.h>
#include <cstring>

#include "host_info.h"
#include "queue.h"
#include "task.h"
#include "thread_pool.h"
#include "husthttp.h"
#include "thread.h"
#include "singleton.h"

static void * check_alive ( void * arg )
{
    host_info_t & host_info = singleton_t<host_info_t>::instance ();

    int stop_pfd = * ( int * ) arg;
    int timerfd = timerfd_create ( CLOCK_REALTIME, 0 );
    int redeliver_fd = timerfd_create ( CLOCK_REALTIME, 0 );

    if ( timerfd == - 1 || redeliver_fd == - 1 )
    {
        return ( void * ) NULL;
    }

    struct itimerspec tv, redeliver_tv;

    memset ( &tv, 0, sizeof ( tv ) );

    memset ( &redeliver_tv, 0, sizeof ( redeliver_tv ) );

    tv.it_value.tv_sec = 1;

    tv.it_interval.tv_sec = 5;

    redeliver_tv.it_value.tv_sec = 1;

    redeliver_tv.it_interval.tv_sec = 1;

    if ( timerfd_settime ( timerfd, 0, &tv, NULL ) == - 1
         || timerfd_settime ( redeliver_fd, 0, &redeliver_tv, NULL ) == - 1 )
    {
        return ( void * ) NULL;
    }

    struct pollfd pfd[3];

    pfd[0].fd = timerfd;

    pfd[0].events = POLLIN;

    pfd[1].fd = redeliver_fd;

    pfd[1].events = POLLIN;

    pfd[2].fd = stop_pfd;

    pfd[2].events = POLLIN;

    int ret = - 1;

    uint64_t val;

    while ( 1 )
    {
        ret = poll ( pfd, 3, - 1 );

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
            read ( redeliver_fd, &val, sizeof ( val ) );
            host_info.redeliver ();
        }

        if ( pfd[2].revents & POLLIN )
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
, _mutex ( )
, _redeliver_size ( 500 )
, _max_queue_size ( 0 )
, _tp ( NULL )
, _client ( NULL )
, _thread ( NULL )
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
    rw_lock_guard_t lock ( _rwlock, WLOCK );

    if ( _queue.find ( host ) != _queue.end () )
    {
        return true;
    }

    queue_t * queue = new queue_t ( _max_queue_size );

    if ( queue == NULL )
    {
        return false;
    }

    _queue[host] = queue;

    _status[host] = binlog_status_t ();
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
    mutex_lock_guard_t lock ( _mutex );
    _status[host].status = status;
}

int host_info_t::get_status ( const std::string & host )
{
    mutex_lock_guard_t lock ( _mutex );
    return _status[host].status;
}

void host_info_t::increment ( const std::string & host )
{
    _status[host].remain.increment ();
}

void host_info_t::decrement ( const std::string & host )
{
    _status[host].remain.decrement ();
}

void host_info_t::check_db ( )
{
    const char * method = "GET";
    const char * path = "/status.html";
    std::string head, body;
    int http_code = 0;

    rw_lock_guard_t lock ( _rwlock, RLOCK );

    for ( std::map<std::string, queue_t *>::iterator it = _queue.begin (); it != _queue.end (); ++ it )
    {
        _client->set_host ( it->first.c_str (), it->first.size () );

        if ( ! _client->open ( method, path, NULL, NULL, 0, 1, 3 ) )
        {
            set_status ( it->first, 0 );
            continue;
        }

        int tmp;

        if ( ! _client->process ( &tmp ) )
        {
            set_status ( it->first, 0 );
            continue;
        }

        _client->get_response ( body, head, http_code );

        if ( http_code == 200 && strncmp ( body.c_str (), "ok\n", 3 ) == 0 )
        {
            set_status ( it->first, 1 );
        }
        else
        {
            set_status ( it->first, 0 );
        }
    }
}

void host_info_t::redeliver ( )
{
    rw_lock_guard_t lock ( _rwlock, RLOCK );

    for ( std::map<std::string, queue_t *>::iterator it = _queue.begin (); it != _queue.end (); ++ it )
    {
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