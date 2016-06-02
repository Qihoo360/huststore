#include "global.h"
#include <sys/timerfd.h>
#include <sys/time.h>
#include <poll.h>
static pthread_t timer_tid;
static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static int is_running;

void *timer_thread ( void * );
static void set_current_time ( void );
int start_timer_thread ( void );
int stop_timer_thread ( void );

int start_timer_thread ( void )
{
    int ret;
    pthread_mutex_lock (&timer_lock);
    is_running = 1;
    pthread_mutex_unlock (&timer_lock);
    if ( ( ret = pthread_create (&timer_tid, NULL, timer_thread, NULL) ) != 0 )
    {
        return - 1;
    }
    return 0;
}

int stop_timer_thread ( void )
{
    int ret;
    pthread_mutex_lock (&timer_lock);
    is_running = 0;
    pthread_mutex_unlock (&timer_lock);
    if ( ( ret = pthread_join (timer_tid, NULL) ) != 0 )
    {
        return - 1;
    }
    return 0;
}

void *timer_thread ( void * )
{
    int timerfd = timerfd_create (CLOCK_REALTIME, 0);
    if ( timerfd == - 1 )
    {
        return NULL;
    }

    struct itimerspec tv;
    memset (&tv, 0, sizeof (tv ));

    tv.it_value.tv_sec = 1;
    tv.it_interval.tv_sec = 1;

    if ( timerfd_settime (timerfd, 0, &tv, NULL) == - 1 )
        return NULL;

    struct pollfd pfd;
    pfd.fd = timerfd;
    pfd.events = POLLIN;

    int ret;
    uint64_t val;

    while ( is_running )
    {
        ret = poll (&pfd, 1, - 1);
        if ( ret == - 1 )
        {
            if ( errno == EINTR )
            {
                continue;
            }
            continue;
        }
        if ( pfd.revents & POLLIN )
        {
            read (timerfd, &val, sizeof (val ));
            set_current_time ();
        }
    }
    close (timerfd);
    return NULL;
}

static void set_current_time ( void )
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    current_time = ( rel_time_t ) ( tv.tv_sec );
}
