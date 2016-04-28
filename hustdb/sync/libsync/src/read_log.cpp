#include "../lib/global.h"
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

static pthread_t read_log_tid;
static pthread_mutex_t read_log_lock = PTHREAD_MUTEX_INITIALIZER;
static int *stop_pipe;
static int is_running;

int start_read_log_thread ( void );
int stop_read_log_thread ( void );
static void *read_log ( void * );
static void make_message_and_deliver ( std::string, uint32_t, File * );

int start_read_log_thread ( void )
{
    int ret;
    pthread_mutex_lock (&read_log_lock);
    is_running = 1;
    pthread_mutex_unlock (&read_log_lock);
    stop_pipe = ( int * ) calloc (2, sizeof (int ));
    pipe (stop_pipe);
    if ( ( ret = pthread_create (&read_log_tid, NULL, read_log, NULL) ) != 0 )
    {
        return - 1;
    }
    return 0;
}

int stop_read_log_thread ( void )
{
    int ret;
    pthread_mutex_lock (&read_log_lock);
    is_running = 0;
    pthread_mutex_unlock (&read_log_lock);
    write (stop_pipe[1], "s", 1);
    if ( ( ret = pthread_join (read_log_tid, NULL) ) != 0 )
    {
        return - 1;
    }
    free (stop_pipe);
    return 0;
}

void *read_log ( void *arg )
{
    struct stat sb;
    char *addr;
    char *item_ptr;
    char *mmap_addr;
    size_t file_size, size;
    int fd;
    struct pollfd *pfd;

    int hosts_size = hosts.size ();
    pfd = ( struct pollfd * ) calloc (hosts_size + 1, sizeof (struct pollfd ));

    for ( int i = 0; i < hosts_size; i ++ )
    {
        pfd[i].fd       = pipe_fds[i][0];
        pfd[i].events   = POLLIN;
    }
    pfd[hosts_size].fd      = stop_pipe[0];
    pfd[hosts_size].events  = POLLIN;

    int ret;
    char val;

    while ( is_running )
    {
        ret = poll (pfd, hosts_size + 1, - 1);
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
        File *file;
        for ( int i = 0; i < hosts_size; i ++ )
        {
            if ( pfd[i].revents & POLLIN )
            {
                read (pipe_fds[i][0], &val, 1);

                pthread_mutex_lock (&file_queue_mutex);
                if ( file_queue[i].empty () )
                {
                    pthread_mutex_unlock (&file_queue_mutex);
                    continue;
                }
                else
                {
                    file = file_queue[i].front ();
                    pthread_mutex_unlock (&file_queue_mutex);

                    std::string path = file->get_path ();
                    fd = open (path.c_str (), O_RDWR);
                    if ( fd == - 1 )
                    {
                        continue;
                    }
                    if ( fstat (fd, &sb) == - 1 )
                    {
                        close (fd);
                        continue;
                    }

                    pthread_mutex_lock (&file_queue_mutex);
                    time_t mtime = sb.st_mtime;
                    struct timeval tv;
                    gettimeofday (&tv, NULL);
                    if ( file_queue[i].size () > 1 || tv.tv_sec > mtime + gen_log_interval + 20 )
                    {
                        file_queue[i].pop_front ();
                        pthread_mutex_lock (&release_lock);
                        release_queue.push_back (file);
                        pthread_mutex_unlock (&release_lock);
                    }
                    pthread_mutex_unlock (&file_queue_mutex);



                    file_size = sb.st_size;
                    size = ( int ) file_size;
                    addr = ( char * ) mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                    close (fd);
                    mmap_addr = addr;
                    if ( addr == MAP_FAILED )
                    {
                        continue;
                    }
                    size_t last_read_pos        = file->get_last_read_pos ();
                    uint32_t last_read_count    = file->get_last_read_count ();
                    addr                        += last_read_pos;
                    file_size                   -= last_read_pos;
                    int index                   = file->get_index ();

                    while ( ( item_ptr = ( char * ) memchr (addr, '\n', file_size) ) != NULL )
                    {
                        int len     = item_ptr - addr;
                        if ( ! file->get_bitmap ().test (last_read_count) )
                        {
                            std::string str (addr + 32, len - 32);
                            make_message_and_deliver (str, last_read_count, file);
                            uint64_t read_total                     = * ( uint64_t * ) total_status_addr;
                            *( uint64_t * ) total_status_addr          = read_total + 1;
                            char *single_total_status_addr          = total_status_addrs[index];
                            uint64_t single_read_total              = * ( uint64_t * ) ( single_total_status_addr );
                            *( uint64_t * ) ( single_total_status_addr ) = single_read_total + 1;
                        }
                        last_read_count += 1;
                        last_read_pos += ( len + 1 );
                        addr = item_ptr + 1;
                        file_size -= ( len + 1 );
                        file->set_last_read_pos (last_read_pos);
                        file->set_last_read_count (last_read_count);
                    }
                    msync (mmap_addr, size, MS_ASYNC);
                    munmap (mmap_addr, size);
                }
            }
        }
        if ( pfd[hosts_size].revents & POLLIN )
        {
            read (stop_pipe[0], &val, 1);
        }
    }
    free (pfd);
    return NULL;
}

static void make_message_and_deliver ( std::string data, uint32_t pos, File *file )
{
    Message *msg = new Message (data, pos, ( void * ) file);
    Task *t = new Task (NULL, ( void * ) msg);
    tp->add_task (t);
}

