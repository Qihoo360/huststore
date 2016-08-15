#include "timer.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#define DEFAULT_TIMEOUT 5000

namespace hustdb
{

    static int gen_timerfd (
                             int second
                             )
    {
        int timerfd = timerfd_create (CLOCK_REALTIME, 0);
        if ( - 1 == timerfd )
        {
            return - 1;
        }

        struct itimerspec tv;
        memset (&tv, 0, sizeof (tv ));
        tv.it_value.tv_sec = second;
        tv.it_interval.tv_sec = second;

        if ( timerfd_settime (timerfd, 0, &tv, NULL) == - 1 )
        {
            close (timerfd);
            return - 1;
        }

        return timerfd;
    }

    timer_task_t::timer_task_t ( )
    : m_interval ( 0 )
    , m_task_func ( 0 )
    , m_ctx ( NULL )
    {
    }

    timer_task_t::timer_task_t (
                                 int interval,
                                 task_func_t task_func,
                                 void * ctx
                                 )
    : m_interval ( interval )
    , m_task_func ( task_func )
    , m_ctx ( ctx )
    {
    }

    void timer_task_t::operator() ( )
    {
        if ( m_task_func )
        {
            m_task_func (m_ctx);
        }
    }

    struct async_tasks_t
    {

        async_tasks_t ( )
        : m_pfds ( 0 )
        , m_size ( 0 )
        {
        }

        ~ async_tasks_t ( );

        bool init (
                    const std::vector<timer_task_t>& tasks,
                    int quit_fd
                    );

        int wait (
                   int timeout
                   );

        bool quit ( );

        void run_tasks (
                         std::vector<timer_task_t>& tasks
                         );

    private:
        struct pollfd * m_pfds;
        size_t m_size;
    } ;

    async_tasks_t::~ async_tasks_t ( )
    {
        if ( m_pfds )
        {
            for ( size_t i = 0; i < m_size - 1; ++ i )
            {
                close (m_pfds[i].fd);
            }
            delete [] m_pfds;
            m_pfds = NULL;
        }
    }

    bool async_tasks_t::init (
                               const std::vector<timer_task_t>& tasks,
                               int quit_fd
                               )
    {
        size_t size = tasks.size ();

        m_pfds = new pollfd[size + 1];
        if ( ! m_pfds )
        {
            return false;
        }

        m_size = size + 1;

        for ( size_t i = 0; i < size; ++ i )
        {
            int timerfd = gen_timerfd (tasks[i].interval ());
            if ( - 1 == timerfd )
            {
                return false;
            }
            m_pfds[i].fd = timerfd;
            m_pfds[i].events = POLLIN;
        }

        m_pfds[size].fd = quit_fd;
        m_pfds[size].events = POLLIN;

        return true;
    }

    int async_tasks_t::wait (
                              int timeout
                              )
    {
        return poll (m_pfds, m_size, timeout);
    }

    bool async_tasks_t::quit ( )
    {
        if ( m_pfds[m_size - 1].revents & POLLIN )
        {
            char val;
            read (m_pfds[m_size - 1].fd, &val, 1);
            return true;
        }
        return false;
    }

    void async_tasks_t::run_tasks (
                                    std::vector<timer_task_t>& tasks
                                    )
    {
        for ( size_t i = 0; i < m_size - 1; ++ i )
        {
            if ( m_pfds[i].revents & POLLIN )
            {
                uint64_t val;
                read (m_pfds[i].fd, &val, sizeof (val ));
                tasks[i]( );
            }
        }
    }

    timer_t::timer_t ( )
    : m_tid ( 0 )
    , m_timeout ( DEFAULT_TIMEOUT )
    {
        pipe (m_stop_pipe);
        m_async_tasks = new async_tasks_t ();
    }

    timer_t::~ timer_t ( )
    {
        if ( m_async_tasks )
        {
            delete m_async_tasks;
            m_async_tasks = NULL;
        }
    }

    bool timer_t::open ( )
    {
        if ( ! m_async_tasks )
        {
            return false;
        }
        if ( ! m_async_tasks->init (m_tasks, m_stop_pipe[0]) )
        {
            return false;
        }
        return 0 == pthread_create (&m_tid, NULL, timer_t::timer_thread, this);
    }

    void timer_t::kill_me ( )
    {
        if ( ! m_tid )
        {
            return ;
        }
        
        write (m_stop_pipe[1], "q", 1);
        pthread_join (m_tid, NULL);
        close (m_stop_pipe[0]);
        close (m_stop_pipe[1]);
    }

    bool timer_t::register_task (
                                  const timer_task_t& task
                                  )
    {
        if ( task.interval () > m_timeout )
        {
            m_timeout = task.interval ();
        }
        m_tasks.push_back (task);
        return true;
    }

    void * timer_t::timer_thread (
                                   void * arg
                                   )
    {
        timer_t * obj = ( timer_t * ) arg;
        if ( obj )
        {
            obj->run ();
        }
        return 0;
    }

    void timer_t::run ( )
    {
        while ( true )
        {
            int rc = m_async_tasks->wait (m_timeout);
            if ( - 1 == rc )
            {
                if ( EINTR == errno )
                {
                    continue;
                }
                break;
            }
            else if ( 0 == rc )
            {
                continue;
            }
            
            if ( m_async_tasks->quit () )
            {
                break;
            }
            else
            {
                m_async_tasks->run_tasks (m_tasks);
            }
        }
    }

}
