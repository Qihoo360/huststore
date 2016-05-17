#include "../lib/global.h"
#include "../lib/status_serialization.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstdio>
#include <fcntl.h>

#include <errno.h>

int start_monitor_thread ( void );
int stop_monitor_thread ( void );
int start_read_log_thread ( void );
int stop_read_log_thread ( void );
int start_timer_thread ( void );
int stop_timer_thread ( void );
int start_check_db_thread ( void );
int stop_check_db_thread ( void );
char *get_status ( int );
void dispose_status ( char * );

std::vector<std::string> hosts;
std::vector<std::string> status_dirs;
std::vector<std::string> log_dirs;
std::vector<char *> total_status_addrs;
std::vector<int *> pipe_fds;
std::vector<std::deque<File *> >file_queue;
std::deque<File *>release_queue;
pthread_mutex_t file_queue_mutex;
pthread_mutex_t release_lock;
std::map<std::string, File *>file_map;
ThreadPool *tp;
char status_buf[FILE_BIT_MAX];
char *total_status_addr;
char passwd[256];
int release_interval;
int checkdb_interval;
int gen_log_interval;


static void construct_file_queues ( const char *, const char * );
static int construct_file_queue ( int , int );

c_bool_t init ( const char *dir, const char *nginx_dir, const char *auth, int pool_size, int check_release, int check_db, int gen_log )
{
    release_interval        = check_release / 1000;
    checkdb_interval        = check_db / 1000;
    gen_log_interval        = gen_log / 1000;

    pthread_mutex_init (&file_queue_mutex, NULL);
    pthread_mutex_init (&release_lock, NULL);

    memset (status_buf, '0', FILE_BIT_MAX);
    tp = new ThreadPool (pool_size);
    tp->init_threadpool ();

    std::string status_dir_str (nginx_dir);
    status_dir_str.append ("status/");
    const char *status_dir = status_dir_str.c_str ();
    int md = mkdir (status_dir, S_IRWXU | S_IRWXG | S_IRWXO);
    if ( md == - 1 && errno != EEXIST )
    {
        return false;
    }
    char total_status_file[256];
    sprintf (total_status_file, "%stotal_status.log", status_dir);
    int fd = open (total_status_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if ( fd == - 1 )
    {
        std::cerr << "open total_status_file failed" << std::endl;
        return false;
    }
    struct stat sb;
    fstat (fd, &sb);

    if ( sb.st_size == 0 )
    {
        write (fd, ( void * ) status_buf, 2 * sizeof (uint64_t ));
        total_status_addr = ( char * ) mmap (NULL, 2 * sizeof (uint64_t ), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if ( total_status_addr == MAP_FAILED )
        {
            std::cerr << "map total_status_addr failed" << std::endl;
            return false;
        }
        *( uint64_t * ) total_status_addr = 0;
        *( uint64_t * ) ( total_status_addr + sizeof (uint64_t ) ) = 0;
    }
    else
    {
        total_status_addr = ( char * ) mmap (NULL, 2 * sizeof (uint64_t ), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    close (fd);

    fd = open (auth, O_RDONLY);
    if ( fd == - 1 )
    {
        std::cerr << "auth failed" << std::endl;
        return false;
    }
    read (fd, passwd, 255);
    close (fd);

    construct_file_queues (status_dir, dir);


    if ( start_monitor_thread () != 0 )
    {
        return false;
    }
    if ( start_check_db_thread () != 0 )
    {
        return false;
    }
    if ( start_timer_thread () != 0 )
    {
        return false;
    }
    if ( start_read_log_thread () != 0 )
    {
        return false;
    }

    return true;
}

c_bool_t stop_sync ( )
{
    if ( stop_monitor_thread () != 0 )
    {
        return false;
    }
    if ( stop_check_db_thread () != 0 )
    {
        return false;
    }
    if ( stop_timer_thread () != 0 )
    {
        return false;
    }
    if ( stop_read_log_thread () != 0 )
    {
        return false;
    }
    delete tp;
    return true;
}

static void construct_file_queues ( const char *status_dir, const char *dir )
{
    DIR *dirp;
    struct dirent *dp;

    dirp = opendir (dir);
    if ( dirp == NULL )
        return;
    struct stat sb;

    std::string path;
    std::string log_path;
    std::string status_path;
    std::string status_total_file;

    for ( ; ; )
    {
        dp = readdir (dirp);
        if ( dp == NULL )
        {
            break;
        }
        if ( strcmp (dp->d_name, ".") == 0
             || strcmp (dp->d_name, "..") == 0 )
        {
            continue;
        }
        path = dir;
        path.append (dp->d_name);
        lstat (path.c_str (), &sb);
        if ( ! S_ISDIR (sb.st_mode) )
        {
            continue;
        }
        log_path = dir;
        log_path.append (dp->d_name);
        status_path = status_dir;
        status_path.append (dp->d_name);

        hosts.push_back (dp->d_name);
        log_dirs.push_back (log_path);

        mkdir (status_path.c_str (), S_IRWXU | S_IRWXG | S_IRWXO);
        status_dirs.push_back (status_path);

        status_total_file = status_path;
        status_total_file.append ("/total_status.log");
        int fd = open (status_total_file.c_str (), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        fstat (fd, &sb);
        int status_file_size = sb.st_size;
        if ( status_file_size == 0 )
        {
            const char *buf = "0000000000000000";
            write (fd, ( void * ) buf, 2 * sizeof (uint64_t ));
        }
        char *addr = ( char * ) mmap (NULL, 2 * sizeof (uint64_t ), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if ( addr == MAP_FAILED )
        {
            return;
        }

        if ( status_file_size == 0 )
        {
            *( uint64_t * ) addr                       = 0;
            *( uint64_t * ) ( addr + sizeof (uint64_t ) )  = 0;
        }
        else
        {
            uint64_t has_sync = * ( uint64_t * ) ( addr + sizeof (uint64_t ) );
            *( uint64_t * ) addr = has_sync;
        }

        close (fd);
        total_status_addrs.push_back (addr);

        std::deque<File *> files;
        file_queue.push_back (files);

        int *pipe_fd = ( int * ) calloc (2, sizeof (int ));
        pipe (pipe_fd);
        pipe_fds.push_back (pipe_fd);
    }
    closedir (dirp);

    int size = status_dirs.size ();
    for ( int i = 0; i < size; i ++ )
    {
        construct_file_queue (i, i);
    }
}

static int construct_file_queue ( int status_dir_index, int log_dir_index )
{
    DIR *status_dirp;
    struct dirent *dp;
    struct stat sb;
    char name[256];
    char path[256];

    status_dirp = opendir (status_dirs[status_dir_index].c_str ());
    if ( status_dirp == NULL )
    {
        return - 1;
    }
    for ( ; ; )
    {
        dp = readdir (status_dirp);
        if ( dp == NULL )
        {
            break;
        }
        if ( strcmp (dp->d_name, ".") == 0
             || strcmp (dp->d_name, "..") == 0
             || strcmp (dp->d_name, "total_status.log") == 0 )
        {
            continue;
        }
        sprintf (name, "%s/%s", status_dirs[status_dir_index].c_str (), dp->d_name);
        sprintf (path, "%s/%s", log_dirs[log_dir_index].c_str (), dp->d_name);

        int fd = open (name, O_RDWR);

        if ( fd == - 1 )
        {
            continue;
        }
        fstat (fd, &sb);
        char *status = ( char * ) mmap (NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close (fd);

        std::string bitmap_str (( status + 3 * sizeof (uint32_t ) ), sb.st_size - 3 * sizeof (uint32_t ));
        std::bitset<FILE_BIT_MAX> bitmap (bitmap_str);

        File *file = new File (path, bitmap);
        file->set_index (status_dir_index);
        file->set_addr (status);
        std::string status_path (name);
        file->set_status_path (status_path);
        file_map[path] = file;
        file_queue[status_dir_index].push_back (file);
    }
    closedir (status_dirp);
    return 0;
}

char *get_status ( int hosts_size )
{
    json_lib::Status json_val;
    std::map<std::string, int> total_status;
    std::map<std::string, std::map<std::string, int> > status;

    std::string processed ("processed_items");
    std::string synced ("synced_items");
    total_status[processed] = * ( uint64_t * ) total_status_addr;
    total_status[synced]    = * ( uint64_t * ) ( total_status_addr + sizeof (uint64_t ) );

    for ( int i = 0; i < hosts_size; i ++ )
    {
        std::map<std::string, int> single_status;
        single_status[processed]    = * ( uint64_t * ) total_status_addrs[i];
        single_status[synced]       = * ( uint64_t * ) ( total_status_addrs[i] + sizeof (uint64_t ) );
        status[hosts[i]]            = single_status;
    }

    json_val.total_status   = total_status;
    json_val.status         = status;

    std::string res;


    if ( ! json_lib::Serialize (json_val, res) )
    {
        return NULL;
    }

    size_t status_size = res.size ();
    char *status_addr   = ( char * ) malloc (status_size + 1);
    if ( ! status_addr )
    {
        return NULL;
    }
    memcpy (status_addr, res.c_str (), status_size);
    status_addr[status_size] = '\0';

    return status_addr;
}

void dispose_status ( char *status_addr )
{
    free (status_addr);
}
