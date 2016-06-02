#include "husthttp.h"
#include "global.h"
#include <poll.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <stdint.h>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>


static pthread_t check_db_tid;
static husthttp_t *client;
static pthread_mutex_t check_db_lock = PTHREAD_MUTEX_INITIALIZER;
static int is_running;
static int *stop_pipe;

void *check_db_thread ( void * );
static void check_db ( std::vector<std::string>& );
static void release_file ( );
int start_check_db_thread ( void );
int stop_check_db_thread ( void );

int start_check_db_thread ( void )
{
    int ret;
    client = new husthttp_t ();
    pthread_mutex_lock (&check_db_lock);
    is_running = 1;
    pthread_mutex_unlock (&check_db_lock);
    stop_pipe = ( int * ) calloc (2, sizeof (int ));
    pipe (stop_pipe);
    if ( ( ret = pthread_create (&check_db_tid, NULL, check_db_thread, NULL) ) != 0 )
    {
        return - 1;
    }
    return 0;
}

int stop_check_db_thread ( void )
{
    int ret;
    pthread_mutex_lock (&check_db_lock);
    is_running = 0;
    pthread_mutex_unlock (&check_db_lock);
    write (stop_pipe[1], "s", 1);
    if ( ( ret = pthread_join (check_db_tid, NULL) ) != 0 )
    {
        return - 1;
    }
    free (stop_pipe);
    return 0;
}

void *check_db_thread ( void *arg )
{
    int timerfd = timerfd_create (CLOCK_REALTIME, 0);
    int check_db_fd = timerfd_create (CLOCK_REALTIME, 0);

    if ( timerfd == - 1 || check_db_fd == - 1 )
    {
        return ( void * ) NULL;
    }

    struct itimerspec tv, check_tv;
    memset (&tv, 0, sizeof (tv ));
    memset (&check_tv, 0, sizeof (check_tv ));

    tv.it_value.tv_sec 			= 1;
    tv.it_interval.tv_sec 		= release_interval;

    check_tv.it_value.tv_sec 	= 1;
    check_tv.it_interval.tv_sec = checkdb_interval;

    if ( timerfd_settime (timerfd, 0, &tv, NULL) == - 1
         || timerfd_settime (check_db_fd, 0, &check_tv, NULL) == - 1 )
    {
        return ( void * ) NULL;
    }

    struct pollfd *pfd;
    pfd = ( struct pollfd * ) calloc ( 3, sizeof ( struct pollfd ) );

    if ( pfd == NULL )
    {
        return ( void * ) NULL;
    }

    pfd[0].fd 		= timerfd;
    pfd[0].events 	= POLLIN;
    pfd[1].fd 		= check_db_fd;
    pfd[1].events 	= POLLIN;
    pfd[2].fd 		= stop_pipe[0];
    pfd[2].events 	= POLLIN;

    int ret;
    uint64_t val;
    while ( is_running )
    {
        ret = poll ( pfd, 3, - 1 );
        if ( ret == - 1 )
        {
            if ( errno == EINTR )
            {
                continue;
            }
            continue;
        }
        if ( pfd[0].revents & POLLIN )
        {
            read ( timerfd, &val, sizeof ( val ) );
            release_file ();
        }
        if ( pfd[1].revents & POLLIN )
        {
            read ( check_db_fd, &val, sizeof ( val ) );
            check_db ( hosts );
        }
        if ( pfd[2].revents & POLLIN )
        {
            char buf;
            read ( stop_pipe[0], &buf, 1 );
        }
    }
    close ( timerfd );
    close ( check_db_fd );
    delete client;
    return ( void * ) NULL;
}

static void check_db ( std::vector<std::string> &hosts )
{
    std::string method ("GET");
    std::string path ("/status.html");
    std::string query_string;
    std::string data;
    std::string body, head;
    int http_code;

    int size = hosts.size ();
    int passwd_size = strlen (passwd);

    std::string url;
    for ( int i = 0; i < size; i ++ )
    {
        url.assign (passwd, passwd_size - 1);
        url.push_back ('@');
        url.append (hosts[i]);
        client->set_host (url.c_str (), url.size ());
        client->open2 (method, path, query_string, data, 1, 3);
        int tmp;
        client->process (&tmp);
        client->get_response (body, head, http_code);

        if ( http_code == 200 && strcmp (body.c_str (), "ok\n") == 0 )
        {
            write (pipe_fds[i][1], "s", 1);
        }
    }
}

static void release_file ( )
{
    File *file = NULL;
    pthread_mutex_lock ( &release_lock );
    if ( ! release_queue.empty () )
    {
        file = release_queue.front ();
        release_queue.pop_front ();
    }
    pthread_mutex_unlock ( &release_lock );
    if ( file )
    {
        struct stat sb;
        int fd = open ( file->get_path ().c_str (), O_RDONLY );
        if ( fd == - 1 )
            return;
        fstat (fd, &sb);
        close (fd);

        uint32_t read_size 	= * ( uint32_t * ) ( file->get_addr () );
        uint32_t read_count = * ( uint32_t * ) ( ( char * ) file->get_addr () + sizeof (uint32_t ) );

        if ( sb.st_size == read_size && read_count == file->fetch_accomplish_cnt ( ) )
        {
            std::map<std::string, File *>::iterator it;
            it = file_map.find (file->get_path ().c_str ());
            if ( it != file_map.end () )
            {
                file_map.erase (it);
            }
            uint64_t process_total = * ( uint64_t * ) ( total_status_addr + sizeof (uint64_t ) );
            *( uint64_t * ) ( total_status_addr + sizeof (uint64_t ) ) = process_total + read_count;
            delete file;
        }
        else
        {
            pthread_mutex_lock (&release_lock);
            release_queue.push_back (file);
            pthread_mutex_unlock (&release_lock);
        }
    }
}
