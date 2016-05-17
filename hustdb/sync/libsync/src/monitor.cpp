#include "../lib/global.h"
#include <poll.h>
#include <sys/inotify.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>
#include <vector>

#define BUF_LEN (10*sizeof(struct inotify_event) + NAME_MAX + 1)

static pthread_t monitor_tid;
static pthread_mutex_t monitor_lock = PTHREAD_MUTEX_INITIALIZER;
static int is_running;
static int *stop_pipe;

int start_monitor_thread ( void );
int stop_monitor_thread ( void );
static void *monitor_dir ( void * );
static void handle_monitor_event ( struct inotify_event*, int *, int , int & );

int start_monitor_thread ( void )
{
    int ret;
    pthread_mutex_lock (&monitor_lock);
    is_running = 1;
    pthread_mutex_unlock (&monitor_lock);
    stop_pipe = ( int * ) calloc (2, sizeof (int ));
    pipe (stop_pipe);
    if ( ( ret = pthread_create (&monitor_tid, NULL, monitor_dir, NULL) ) != 0 )
    {
        return - 1;
    }
    return 0;
}

int stop_monitor_thread ( void )
{
    int ret;
    pthread_mutex_lock (&monitor_lock);
    is_running = 0;
    pthread_mutex_unlock (&monitor_lock);
    write (stop_pipe[1], "s", 1);
    if ( ( ret = pthread_join (monitor_tid, NULL) ) != 0 )
    {
        return - 1;
    }
    free (stop_pipe);
    return 0;
}

static void *monitor_dir ( void *arg )
{
    int size = hosts.size ();
    int inotify_fd;
    int wds[size];
    struct pollfd pfd[2];
    ssize_t numRead;
    char buf[BUF_LEN];
    struct inotify_event *event;

    inotify_fd = inotify_init ();
    if ( inotify_fd == - 1 )
    {
        return ( void * ) NULL;
    }

    for ( int i = 0; i < size; i ++ )
    {
        wds[i] = inotify_add_watch (inotify_fd, log_dirs[i].c_str (), IN_CREATE | IN_MODIFY);
        if ( wds[i] == - 1 )
        {
            return NULL;
        }
    }

    pfd[0].fd       = inotify_fd;
    pfd[0].events   = POLLIN;

    pfd[1].fd       = stop_pipe[0];
    pfd[1].events   = POLLIN;

    int ret;
    while ( is_running )
    {
        ret = poll (pfd, 2, - 1);
        if ( ret == - 1 )
        {
            if ( errno == EINTR )
            {
                continue;
            }
            continue;
        }
        else if ( ret == 0 )
        {
            continue;
        }
        if ( pfd[0].revents & POLLIN )
        {
            numRead = read (inotify_fd, buf, BUF_LEN);
            if ( numRead == - 1 )
            {
                continue;
            }
            for ( char *p = buf; p < buf + numRead; )
            {
                event = ( struct inotify_event * ) p;
                handle_monitor_event (event, wds, size, inotify_fd);
                p += sizeof (struct inotify_event ) + event->len;
            }
        }
        else if ( pfd[1].revents & POLLIN )
        {
            char val;
            read (stop_pipe[0], &val, 1);
        }
    }
    return NULL;
}

static void handle_monitor_event ( struct inotify_event *event, int *wds, int size, int &inotify_fd )
{
    int index = - 1;
    for ( int i = 0; i < size; i ++ )
    {
        if ( wds[i] == event->wd )
        {
            index = i;
            break;
        }
    }
    if ( index == - 1 ) return ;

    std::string path (log_dirs[index]);
    path.push_back ('/');
    path.append (event->name);
    std::string status_path (status_dirs[index]);
    status_path.push_back ('/');
    status_path.append (event->name);

    if ( event->mask & IN_CREATE )
    {
        int fd = open (status_path.c_str (), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        write (fd, ( void * ) status_buf, FILE_BIT_MAX);
        char *addr = ( char * ) mmap (0, FILE_BIT_MAX, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close (fd);
        *( uint32_t * ) addr = 0;
        *( uint32_t * ) ( addr + sizeof (uint32_t ) ) = 0;
        *( uint32_t * ) ( addr + 2 * sizeof (uint32_t ) ) = 0;
        std::string bitmap_str (( char * ) ( addr + 3 * sizeof (uint32_t ) ), FILE_BIT_MAX - 3 * sizeof (uint32_t ));
        std::bitset<FILE_BIT_MAX> bitmap (bitmap_str);

        File *file = new File (path, bitmap);
        file->set_index (index);
        file->set_addr (addr);
        file->set_status_path (status_path);
        pthread_mutex_lock (&file_queue_mutex);
        file_map[path] = file;
        file_queue[index].push_back (file);
        pthread_mutex_unlock (&file_queue_mutex);
    }
    else if ( event->mask & IN_MODIFY )
    {
        pthread_mutex_lock (&file_queue_mutex);
        if ( file_map.count (path) == 0 )
        {
            File *file = new File (path);
            file->set_index (index);
            file->set_status_path (status_path);
            file_map[path] = file;
            file_queue[index].push_back (file);
        }
        pthread_mutex_unlock (&file_queue_mutex);
    }
}
