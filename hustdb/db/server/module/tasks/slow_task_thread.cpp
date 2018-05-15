#include "slow_task_thread.h"

slow_task_thread_t::slow_task_thread_t ( )
: TASK_THREAD ( )
, m_tasks ( )
, m_doing_tasks ( )
, m_done_tasks ( )
, m_lock ( )
{
    memset ( & m_event, 0, sizeof ( m_event ) );
}

slow_task_thread_t::~ slow_task_thread_t ( )
{
    stop ();
}

void slow_task_thread_t::stop ( )
{
    G_APPTOOL->event_alarm ( & m_event );

    TASK_THREAD::stop ();

    scope_wlock_t lock ( m_lock );

    if ( ! m_tasks.empty () )
    {
        for ( tasks_t::iterator it = m_tasks.begin (); it != m_tasks.end (); ++ it )
        {
            task2_t * p = ( * it );
            if ( p )
            {
                p->release ();
            }
        }
        m_tasks.clear ();
    }
}

bool slow_task_thread_t::start ( )
{
    if ( ! G_APPTOOL->event_create ( & m_event ) )
    {
        LOG_ERROR ( "[slow_task][start]event_create faield" );
        return false;
    }

    if ( ! TASK_THREAD::start ( ) )
    {
        LOG_ERROR ( "[slow_task][start]thread_t::start faield" );
        return false;
    }

    return true;
}

bool slow_task_thread_t::push (
                                task2_t * task
                                )
{
    if ( unlikely ( NULL == task ) )
    {
        LOG_ERROR ( "[slow_task][push]task NULL or m_queue NULL" );
        return false;
    }

    scope_wlock_t lock ( m_lock );
    
    try
    {
        m_tasks.push_back ( task );
        return G_APPTOOL->event_alarm ( & m_event );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[slow_task][push]bad_alloc" );
        task->release ();
    }

    return false;
}

bool slow_task_thread_t::empty ( )
{
    scope_rlock_t lock ( m_lock );
        
    return m_tasks.empty ();
}

bool slow_task_thread_t::le_one ( )
{
    scope_rlock_t lock ( m_lock );
    
    return m_tasks.size () <= 1;
}

void slow_task_thread_t::run ( )
{
    while ( ! is_need_exit () )
    {
        int r;

        r = G_APPTOOL->event_wait ( & m_event, 500 );
        if ( 1 != r )
        {
            continue;
        }

        while ( 1 )
        {
            task2_t * p = NULL;

            {
                scope_wlock_t lock ( m_lock );
                
                if ( m_tasks.empty () )
                {
                    break;
                }

                p = m_tasks.front ();
                m_tasks.pop_front ();

                m_doing_tasks.push_back ( p );
            }

            if ( p )
            {
                p->process ();
                p->release ();

                m_doing_tasks.pop_front ();

                if ( m_done_tasks.size () >= 3 )
                {
                    m_done_tasks.pop_front ();
                }

                m_done_tasks.push_back ( p );
            }
        }
    }
}

void slow_task_thread_t::info (
                                std::string & info
                                )
{
    scope_rlock_t lock ( m_lock );
    
    char s[ 128 ] = { };

    sprintf ( s,
             "{\"undo\":\"%d\",\"doning\":%d,\"done\":%d}",
             m_tasks.size (),
             m_doing_tasks.size (),
             m_done_tasks.size ()
             );

    info = s;
}

slow_task_type_t slow_task_thread_t::status (
                                              task2_t * task
                                              )
{
    scope_rlock_t lock ( m_lock );

    tasks_t::iterator it;

    for ( it = m_tasks.begin (); it != m_tasks.end (); ++ it )
    {
        if ( * it == task )
        {
            return TASK_UNDO;
        }
    }

    for ( it = m_doing_tasks.begin (); it != m_doing_tasks.end (); ++ it )
    {
        if ( * it == task )
        {
            return TASK_DOING;
        }
    }

    for ( it = m_done_tasks.begin (); it != m_done_tasks.end (); ++ it )
    {
        if ( * it == task )
        {
            return TASK_DONE;
        }
    }

    return TASK_UNKNOW;
}
