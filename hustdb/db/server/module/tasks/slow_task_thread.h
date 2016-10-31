#ifndef _slow_task_thread_h_
#define _slow_task_thread_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../base.h"
#include <list>

enum slow_task_type_t
{
    TASK_UNDO    = 0,
    TASK_DOING   = 1,
    TASK_DONE    = 2,
    TASK_UNKNOW  = 3
};

#define THREAD_STRUCT( name )                                       \
    typedef struct db_tasks_t                                       \
    {                                                               \
        pthread_t       thread;                                     \
        volatile bool   is_started;                                 \
        volatile bool   is_exited;                                  \
        volatile bool   is_need_exit;                               \
        void *          ( * user_routine )( void * p );             \
        void *          user_routine_param;                         \
        event2_t        start_event;                                \
        int             thread_id;                                  \
        bool            start_event_inited;                         \
    } db_tasks_t;

THREAD_STRUCT ( db_tasks )

class TASK_THREAD
{
public:

    TASK_THREAD ( )
    {
    }

    virtual ~TASK_THREAD ( )
    {
        stop ( );
    }

    bool is_started ( ) const
    {
        return m_thread.is_started;
    }

    bool is_need_exit ( ) const
    {
        return m_thread.is_need_exit;
    }

    bool is_exited ( ) const
    {
        return m_thread.is_exited;
    }

    int tid ( ) const
    {
        return m_thread.thread_id;
    }

    static void * start_routine_inner ( void * p )
    {
        db_tasks_t * self = ( db_tasks_t * ) p;

        self->thread_id = gettid ( );
        self->is_exited = false;

        G_APPTOOL->event_alarm ( & self->start_event );

        void * r = self->user_routine ( self->user_routine_param );

        self->thread_id = -1;
        self->is_exited = true;

        return r;
    }

    static void * thread_routine ( void * p )
    {
        TASK_THREAD * t = ( TASK_THREAD * ) p;
        t->run ( );
        return NULL;
    }

    virtual bool start ( )
    {
        m_thread.is_started = false;
        m_thread.is_exited = true;
        m_thread.is_need_exit = false;
        m_thread.user_routine = NULL;
        m_thread.user_routine_param = NULL;
        m_thread.thread_id = -1;
        memset ( & m_thread.thread, 0, sizeof ( pthread_t ) );
        m_thread.start_event_inited = false;

        int n;
        if ( unlikely ( is_started ( ) ) )
        {
            return false;
        }
        if ( !m_thread.start_event_inited )
        {
            if ( G_APPTOOL->event_create ( & m_thread.start_event ) )
            {
                m_thread.start_event_inited = true;
            }
            else
            {
                return false;
            }
        }
        m_thread.user_routine = thread_routine;
        m_thread.user_routine_param = this;
        int r = pthread_create ( & m_thread.thread, NULL, start_routine_inner, & m_thread );
        if ( unlikely ( 0 != r ) )
        {
            return false;
        }

        while ( 1 )
        {
            n = G_APPTOOL->event_wait ( & m_thread.start_event, INFINITE );
            if ( -2 == n )
            {
                continue;
            }

            break;
        }

        if ( 1 != n )
        {
            G_APPTOOL->event_destroy ( & m_thread.start_event );
            m_thread.start_event_inited = false;
            return false;
        }

        G_APPTOOL->event_destroy ( & m_thread.start_event );
        m_thread.start_event_inited = false;

        m_thread.is_started = true;

        return true;
    }

    virtual void stop ( )
    {
        if ( unlikely ( !is_started ( ) ) )
        {
            return;
        }

        m_thread.is_need_exit = true;
        while ( !is_exited ( ) )
        {
            G_APPTOOL->sleep_ms ( 1 );
        }

        pthread_join ( m_thread.thread, NULL );

        memset ( & m_thread.thread, 0, sizeof ( m_thread.thread ) );
        m_thread.is_started = false;
        m_thread.is_need_exit = false;

        if ( m_thread.start_event_inited )
        {
            G_APPTOOL->event_destroy ( & m_thread.start_event );
            m_thread.start_event_inited = false;
        }
    }

    virtual void run ( )
    {
        while ( !is_need_exit ( ) )
        {
            G_APPTOOL->sleep_ms ( 1000 );
        }
    }

protected:

    db_tasks_t m_thread;

private:
    // disable
    TASK_THREAD ( const TASK_THREAD & );
    const TASK_THREAD & operator= ( const TASK_THREAD & );
};

class task2_t
{
public:

    virtual void release ( ) = 0;
    virtual void process ( ) = 0;
};

class slow_task_thread_t : public TASK_THREAD
{
public:
    slow_task_thread_t ( );
    ~slow_task_thread_t ( );

    bool start ( );

    void stop ( );

    bool push (
                task2_t * task
                );

    bool empty ( );
    
    bool le_one ( );

    void run ( );

    void info (
                std::string & info
                );

    slow_task_type_t status (
                              task2_t * task
                              );

private:

    typedef std::list< task2_t * > tasks_t;

    tasks_t m_tasks;
    tasks_t m_doing_tasks;
    tasks_t m_done_tasks;
    lockable_t m_lock;
    event2_t m_event;

private:
    // disable
    slow_task_thread_t ( const slow_task_thread_t & );
    const slow_task_thread_t & operator= ( const slow_task_thread_t & );
};

#endif
