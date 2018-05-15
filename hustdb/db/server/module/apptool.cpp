#include "apptool.h"
#include <sys/mman.h>

apptool_t * apptool_t::m_apptool = NULL;

apptool_t::apptool_t ( )
{
    m_hustdb = NULL;
    m_apptool = NULL;

    m_little_endian = check_endian ();
}

apptool_t::~ apptool_t ( )
{
}

apptool_t * apptool_t::get_apptool ( )
{
    return ( m_apptool == NULL ) ? m_apptool = new apptool_t () : m_apptool;
}

void apptool_t::kill_me ( )
{
    if ( NULL != m_apptool )
    {
        delete m_apptool;
        m_apptool = NULL;
    }
}

void apptool_t::set_hustdb (
                             void * db
                             )
{
    m_hustdb = db;
}

void * apptool_t::get_hustdb ( )
{
    return m_hustdb;
}

bool apptool_t::is_file (
                          const char * path
                          )
{
    char ph[ MAX_PATH ];
#if defined( WIN32 ) || defined( WIN64 )
    DWORD att;
#else
    struct stat st;
#endif
    if ( NULL == path || '\0' == * path )
        return false;
    strncpy ( ph, path, MAX_PATH );
    ph[ MAX_PATH - 1 ] = '\0';
    path_to_os ( ph );

#if defined( WIN32 ) || defined( WIN64 )
    att = GetFileAttributesA ( ph );
#ifdef _WIN32_WCE
    if ( 0xFFFFFFFF == att )
#else
    if ( INVALID_FILE_ATTRIBUTES == att )
#endif
        return false;
    return 0 == ( att & FILE_ATTRIBUTE_DIRECTORY );
#else
    if ( NULL == path || '\0' == * path )
        return false;
    strncpy ( ph, path, MAX_PATH );
    ph[ MAX_PATH - 1 ] = '\0';
    path_to_os ( ph );

    memset ( & st, 0, sizeof ( st ) );
    if ( stat ( ph, & st ) == - 1 )
    {
        return false;
    }
    return 0 == ( st.st_mode & S_IFDIR );
#endif
}

bool apptool_t::is_dir ( const char * path )
{
    char ph[ MAX_PATH ];
#if defined( WIN32 ) || defined( WIN64 )
    DWORD att;
#else
    struct stat st;
#endif

    if ( NULL == path || '\0' == * path )
    {
        return false;
    }
    strncpy ( ph, path, MAX_PATH );
    ph[ MAX_PATH - 1 ] = '\0';
    path_to_os ( ph );

#if defined( WIN32 ) || defined( WIN64 )
    att = GetFileAttributesA ( ph );
#ifdef _WIN32_WCE
    if ( 0xFFFFFFFF == att )
    {
#else
    if ( INVALID_FILE_ATTRIBUTES == att )
    {
#endif
        return false;
    }
    return 0 != ( att & FILE_ATTRIBUTE_DIRECTORY );
#else

    if ( NULL == path || '\0' == * path )
    {
        return false;
    }
    strncpy ( ph, path, MAX_PATH );
    ph[ MAX_PATH - 1 ] = '\0';
    path_to_os ( ph );

    memset ( & st, 0, sizeof ( st ) );
    if ( stat ( ph, & st ) == - 1 )
    {
        return false;
    }

    return 0 != ( st.st_mode & S_IFDIR );
#endif
}

bool apptool_t::make_dir ( const char * dir )
{
    char ph[ MAX_PATH ];
    char * p;

    if ( NULL == dir || '\0' == * dir )
    {
        return false;
    }

    strncpy ( ph, dir, MAX_PATH );
    ph[ MAX_PATH - 1 ] = '\0';
    path_to_os ( ph );

    p = strchr ( ph, S_PATH_SEP_C );
    if ( NULL == p )
    {
#if defined( WIN32 ) || defined( WIN64 )
        CreateDirectoryA ( ph, 0 );
#else
        if ( 0 != mkdir ( ph, S_IRWXU | S_IRWXG | S_IRWXO ) )
        {
            if ( EEXIST != errno )
            {
                return false;
            }
        }
#endif
    }
    else
    {

        if ( p && p == ( char * ) ph )
        {
            p = strchr ( p + 1, S_PATH_SEP_C );
        }

        do
        {

            if ( p )
            {
                * p = '\0';
            }

#if defined( WIN32 ) || defined( WIN64 )
            CreateDirectoryA ( ph, 0 );
#else
            if ( 0 != mkdir ( ph, S_IRWXU | S_IRWXG | S_IRWXO ) )
            {
                if ( EEXIST != errno )
                {
                    return false;
                }
            }
#endif

            if ( NULL == p )
            {
                break;
            }

            * p = S_PATH_SEP_C;

            p = strchr ( p + 1, S_PATH_SEP_C );

        }
        while ( 1 );
    }

    if ( ! is_dir ( ph ) )
        return false;

    return true;
}

void apptool_t::path_to_os ( char * path )
{
    if ( path )
    {
        while ( * path )
        {
            if ( '/' == * path || '\\' == * path )
                * path = S_PATH_SEP_C;
            ++ path;
        }
    }
}

void apptool_t::sleep_ms ( unsigned int ms )
{
#if defined( WIN32 ) || defined( WIN64 )
    Sleep ( ms );
#else
    if ( ms > 0 )
    {
        struct timespec interval;
        struct timespec remainder;
        int             r;

        interval.tv_sec     = ms / 1000;
        interval.tv_nsec    = ( int ) ( ( ms % 1000 ) * 1e+6 );

        while ( interval.tv_sec + interval.tv_nsec > 0 )
        {
            remainder.tv_sec    = 0;
            remainder.tv_nsec   = 0;
            r = nanosleep ( & interval, & remainder );
            if ( 0 == r )
            {
                break;
            }
            if ( EINTR != errno )
            {
                break;
            }
            interval.tv_sec     = remainder.tv_sec;
            interval.tv_nsec    = remainder.tv_nsec;
        }
    }
    else
    {
        sleep ( 0 );
    }
#endif
}

size_t apptool_t::get_exe_path (
                                 char * path,
                                 size_t  path_len
                                 )
{
#if defined(WIN32) || defined(WIN64)
    DWORD r;
    assert ( path && path_len );
    r = GetModuleFileNameA ( NULL, path, ( DWORD ) path_len );
    if ( ERROR_INSUFFICIENT_BUFFER == GetLastError () || ( size_t ) r >= path_len )
    {
        path[ 0 ] = 0;
        return 0;
    }
    path[ r ] = '\0';
    return ( size_t ) r;
#else
    ssize_t r;
    assert ( path && path_len );
    r = readlink ( "/proc/self/exe", path, path_len );
    if ( r <= 0 )
    {
        path[ 0 ] = 0;
        return 0;
    }
    if ( ( unsigned long ) r >= path_len )
    {
        path[ 0 ] = 0;
        return 0;
    }
    path[ r ] = '\0';
    return r;
#endif
}

size_t apptool_t::get_exe_dir (
                                char *  dir,
                                size_t  dir_len,
                                bool    add_sep
                                )
{
    char * p;
    if ( dir_len <= 1 )
    {
        return 0;
    }

    get_exe_path ( dir, dir_len - 1 );
    if ( '\0' == dir[ 0 ] )
    {
        * dir = '\0';
        return 0;
    }

    p = ( char* ) strrchr ( dir, S_PATH_SEP_C );
    if ( NULL == p )
    {
        * dir = '\0';
        return 0;
    }

    if ( add_sep )
    {
        ++ p;
    }

    * p = '\0';
    return p - dir;
}

bool apptool_t::set_block (
                            int fd,
                            bool is_block
                            )
{
    if ( is_block )
    {
#if defined( WIN32 ) || defined( WIN64 )
        unsigned long arg = 0;
        ioctlsocket (fd, FIONBIO, &arg);
        return true;
#else
        int flags = fcntl (fd, F_GETFL);
        if ( O_NONBLOCK & flags )
        {
            flags &= ~ O_NONBLOCK;
            if ( - 1 == fcntl (fd, F_SETFL, flags) )
            {
                return false;
            }
        }
        return true;
#endif
    }
    else
    {
#if defined( WIN32 ) || defined( WIN64 )
        unsigned long arg = 1;
        ioctlsocket (fd, FIONBIO, &arg);
        return true;
#else
        int flags = fcntl (fd, F_GETFL);
        if ( 0 == ( O_NONBLOCK & flags ) )
        {
            flags |= O_NONBLOCK;
            if ( - 1 == fcntl (fd, F_SETFL, flags) )
            {
                return false;
            }
        }
        return true;
#endif
    }
}

bool apptool_t::get_current_time (
                                   int *   year,
                                   int *   month,
                                   int *   day,
                                   int *   hour,
                                   int *   minute,
                                   int *   second,
                                   int *   ms
                                   )
{
#if defined( WIN32 ) || defined( WIN64 )
    SYSTEMTIME st;
    GetLocalTime ( & st );
    if ( year )         * year = st.wYear;
    if ( month )        * month = st.wMonth;
    if ( day )          * day = st.wDay;
    if ( hour )         * hour = st.wHour;
    if ( minute )       * minute = st.wMinute;
    if ( second )       * second = st.wSecond;
    if ( ms )           * ms = st.wMilliseconds;
    return true;
#else
    struct timeval  tv;
    struct timezone tz;
    struct tm       ltm;
    int             r;

    r = gettimeofday ( & tv, & tz );
    if ( 0 != r )
    {
        if ( year )     * year = 0;
        if ( month )    * month = 0;
        if ( day )      * day = 0;
        if ( hour )     * hour = 0;
        if ( minute )   * minute = 0;
        if ( second )   * second = 0;
        if ( ms )       * ms = 0;
        return false;
    }

    localtime_r ( & tv.tv_sec, & ltm );

    if ( year )         * year  = 1900 + ltm.tm_year;
    if ( month )        * month = 1 + ltm.tm_mon;
    if ( day )          * day   = ltm.tm_mday;
    if ( hour )         * hour  = ltm.tm_hour;
    if ( minute )       * minute = ltm.tm_min;
    if ( second )       * second = ltm.tm_sec;
    if ( ms )           * ms    = ( int ) ( tv.tv_usec / 1000 );
    return true;
#endif
}

unsigned int apptool_t::get_tick_count ( )
{
#if defined( WIN32 ) || defined( WIN64 )
    return GetTickCount ();
#elif defined( __linux )
    struct timespec ts;
    if ( unlikely ( 0 != clock_gettime ( CLOCK_MONOTONIC, & ts ) ) )
    {
        return ( unsigned int ) 0;
    }
    return ( unsigned int ) ( ts.tv_sec * 1000L + ts.tv_nsec / 1000000L );
#else
    struct timeval tv;
    if ( 0 != gettimeofday ( & tv, NULL ) )
    {
        return ( unsigned int ) 0;
    }
    return ( unsigned int ) ( tv.tv_sec * 1000L + tv.tv_usec / 1000L );
#endif
}

#define ENABLE_LOG  1

#define FILETAIL       ".txt"
#define FILETAIL_LEN    (sizeof(FILETAIL) - 1)

#define FILETIME_LEN    (sizeof("YYYYMMDD") - 1)
#define FILETIME_FMT    "%04d%02d%02d"

#define LOGTIME_LEN     (sizeof("YYYY-MM-DD HH:mm:DD.mmm")-1)
#define LOGTIME_FMT     "%04d-%02d-%02d %02d:%02d:%02d.%03d"

typedef struct
{
    char            path[ 260 ];
    size_t          path_len;
    size_t          path_filetime_offset;
    size_t          path_tail_offset;

    bool            enable_stdout;
    bool            enable_stderr;
    bool            is_open;

    FILE *          fp;
    pthread_mutex_t lock;

} log_t;

static log_t g_tools_log;

typedef struct
{
    const char * levels[ 2 ];

} level_item_t;

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int ms;

} time_info_t;

#if ENABLE_LOG

static inline
int get_errno ( )
{
#if defined( WIN32 ) || defined( WIN64 )
    return ( int ) GetLastError ();
#else
    return errno;
#endif
}

static inline
bool log_calc_filetime (
                         time_info_t *   nowt,
                         char *          s,
                         size_t          slen
                         )
{
    int r;

    if ( unlikely ( slen <= FILETIME_LEN ) )
    {
        return false;
    }

    r = snprintf ( s, slen, FILETIME_FMT, nowt->year, nowt->month, nowt->day );

    return r > 0 && r < ( int ) slen;
}

#if ! defined( WIN32 ) && ! defined( WIN64 )

static void log_redirect_fd ( log_t * log )
{
    int fd = fileno ( log->fp );
    if ( 1 != fd )
    {
        dup2 ( fd, 1 );
    }
    if ( 2 != fd )
    {
        dup2 ( fd, 2 );
    }
}

#endif // #if ! defined( WIN32 ) && ! defined( WIN64 )

#endif // #if ENABLE_LOG

int apptool_t::log_open (
                          const char * log_dir,
                          const char * log_name,
                          bool redirect_stdout
                          )
{
#if ENABLE_LOG
    int                 r;
    size_t              len;
    time_info_t         nowt;
    log_t *             p                                      = NULL;
    char                def_dir[ 1024 ]                        = "";
    char                def_name[ 1024 ]                       = "";
    char                path[ sizeof ( g_tools_log.path ) ]    = "";
    size_t              path_len                               = 0;
    size_t              path_filetime_offset                   = 0;
    size_t              path_tail_offset                       = 0;

    if ( unlikely ( g_tools_log.is_open ) )
    {
        fprintf ( stderr, "[log][path=%s] log already open\n", g_tools_log.path );
        return 0;
    }

    if ( unlikely ( ! get_current_time ( & nowt.year,
                                        & nowt.month,
                                        & nowt.day,
                                        & nowt.hour,
                                        & nowt.minute,
                                        & nowt.second,
                                        & nowt.ms ) )
         )
    {
        return - EFAULT;
    }

    if ( NULL == log_dir || '\0' == * log_dir )
    {

        r = 0;
        do
        {
            char * t;

            def_dir[ 0 ] = '\0';
            get_exe_path ( def_dir, sizeof ( def_dir ) );
            if ( '\0' == def_dir[ 0 ] )
            {
                log_dir = "./log";
                break;
            }

            t = ( char * ) strrchr ( def_dir, S_PATH_SEP_C );
            if ( NULL == t )
            {
                fprintf ( stderr, "[log][def_dir=%s] %s not found\n", def_dir, S_PATH_SEP );
                r = - EFAULT;
                break;
            }

            ++ t;
            * t = '\0';

            len = strlen ( def_dir );
            if ( len + sizeof ( "log" ) - 1 >= sizeof ( path ) - 1 )
            {
                fprintf ( stderr, "[log]len too long\n" );
                r = - EINVAL;
                break;
            }

            strcpy ( t, "log" );

            log_dir = def_dir;
        }
        while ( 0 );
        if ( 0 != r )
        {
            return r;
        }
    }
    len = strlen ( log_dir );
    if ( len >= sizeof ( path ) - 1 )
    {
        fprintf ( stderr, "[log]len too long" );
        return - EINVAL;
    }
    strcpy ( path, log_dir );
    path_to_os ( path );
    path_len = len;
    if ( S_PATH_SEP_C == path[ path_len - 1 ] )
    {
        path[ path_len - 1 ] = '\0';
        -- path_len;
    }

    make_dir ( path );
    if ( ! is_dir ( path ) )
    {
        fprintf ( stderr, "[log][path=%s]path is not a directory\n", path );
        return - EINVAL;
    }

    strcat ( path, S_PATH_SEP );
    ++ path_len;

    if ( NULL == log_name || '\0' == * log_name )
    {

        r = 0;
        do
        {
            char * t;
            char * t2;

            def_name[ 0 ] = '\0';
            get_exe_path ( def_name, sizeof ( def_name ) );
            if ( '\0' == def_name[ 0 ] )
            {
                log_name = "";
                break;
            }

            t = ( char * ) strrchr ( def_name, S_PATH_SEP_C );
            if ( NULL == t )
            {
                fprintf ( stderr, "[log][def_name=%s] %s not found\n", def_name, S_PATH_SEP );
                r = - EFAULT;
                break;
            }

            ++ t;

            len = strlen ( t );
            if ( 0 == len || len + path_len + 1 >= sizeof ( path ) - 1 )
            {
                fprintf ( stderr, "[log]len too long\n" );
                r = - EINVAL;
                break;
            }

            t2 = ( char * ) strrchr ( t, '.' );
            if ( t2 )
            {
                if ( 0 == stricmp ( ".exe", t2 ) )
                {
                    * t2 = '\0';
                }
            }

            log_name = t;
        }
        while ( 0 );
        if ( 0 != r )
        {
            return r;
        }
    }
    if ( strchr ( log_name, '/' ) || strchr ( log_name, '\\' ) )
    {
        fprintf ( stderr, "[log][log_name=%s] / and \\ is not allowed\n", log_name );
        return - EINVAL;
    }
    len = strlen ( log_name );
    if ( len > 0 )
    {
        if ( unlikely ( path_len + len + 1 >= sizeof ( path ) ) )
        {
            fprintf ( stderr, "[log]len too long\n" );
            return - EINVAL;
        }

        strcat ( path, log_name );
        strcat ( path, "_" );
        path_len += len + 1;
    }
    path_filetime_offset = path_len;
    path_tail_offset = path_filetime_offset + FILETIME_LEN;

    if ( unlikely ( path_len + FILETIME_LEN + FILETAIL_LEN >= sizeof ( path ) ) )
    {
        fprintf ( stderr, "[log]len too long\n" );
        return - EINVAL;
    }

    if ( unlikely ( ! log_calc_filetime (
                                         & nowt,
                                         & path[ path_filetime_offset ],
                                         sizeof ( path ) - 1 - path_filetime_offset
                                         ) ) )
    {
        fprintf ( stderr, "[log]log_calc_filetime failed\n" );
        return - EFAULT;
    }

    strcpy ( & path[ path_tail_offset ], FILETAIL );
    path_len += FILETIME_LEN + FILETAIL_LEN;

#ifdef _DEBUG
    remove ( path );
#endif

    p = & g_tools_log;
    memcpy ( p->path, path, path_len );
    p->path[ path_len ]     = '\0';
    p->path_len             = path_len;
    p->path_filetime_offset = path_filetime_offset;
    p->path_tail_offset     = path_tail_offset;

    p->fp = fopen ( path, "ab" );
    if ( NULL == p->fp )
    {
        fprintf ( stderr, "[log][path=%s]fopen failed\n", path );
        return - EFAULT;
    }

    r = pthread_mutex_init ( & p->lock, NULL );
    if ( 0 != r )
    {
        fprintf ( stderr, "[log]pthread_mutex_init failed\n" );
        fclose ( p->fp );
        p->fp = NULL;
        return - EFAULT;
    }

#if ! defined( WIN32 ) && ! defined( WIN64 )
    if ( redirect_stdout )
    {
        log_redirect_fd ( p );
    }
#endif

    p->is_open = true;
#endif // #if ENABLE_LOG
    return 0;
}

void apptool_t::log_close ( )
{
#if ENABLE_LOG
    log_t * p  = & g_tools_log;

    if ( p->is_open )
    {

        pthread_mutex_destroy ( & p->lock );

        if ( p->fp )
        {
            fclose ( p->fp );
            p->fp = NULL;
        }

        p->is_open = false;
    }

    p->enable_stdout = false;
    p->enable_stderr = false;

#endif // #if ENABLE_LOG
}

void apptool_t::log_enable_stdout ( bool enable )
{
    log_t * p  = & g_tools_log;
    if ( enable )
    {
        p->enable_stdout = true;
        p->enable_stderr = false;
    }
    else
    {
        p->enable_stdout = false;
    }
}

void apptool_t::log_enable_stderr ( bool enable )
{
    log_t * p  = & g_tools_log;
    if ( enable )
    {
        p->enable_stderr = true;
        p->enable_stdout = false;
    }
    else
    {
        p->enable_stderr = false;
    }
}

#if ENABLE_LOG

static inline
int log_write_file (
                     log_t *         log,
                     time_info_t *   nowt,
                     const char *    data,
                     int             data_len
                     )
{
    char        filetime[ FILETIME_LEN + 1 ] = "";
    size_t      written;
    char *      p;
    int         r = 0;

    if ( unlikely ( NULL == log ) )
    {
        return - EINVAL;
    }

    if ( likely ( NULL != log->fp ) )
    {

        do
        {
            if ( unlikely ( ! log_calc_filetime ( nowt, filetime, sizeof ( filetime ) ) ) )
            {
                fprintf ( stderr, "[log]log_calc_filetime failed\n" );
                r = - EFAULT;
                break;
            }
            if ( unlikely ( log->path_filetime_offset >= log->path_len ) )
            {
                fprintf ( stderr, "[log]invalid path_filetime_offset %d\n", ( int ) log->path_filetime_offset );
                r = - EFAULT;
                break;
            }
            p = ( char * ) & log->path[ log->path_filetime_offset ];

            if ( unlikely ( ! mem_equal ( filetime, p, FILETIME_LEN ) ) )
            {
                pthread_mutex_lock ( & log->lock );
                if ( ! mem_equal ( filetime, p, FILETIME_LEN ) )
                {
                    fast_memcpy ( p, filetime, FILETIME_LEN );
                    if ( log->fp )
                    {
                        fclose ( log->fp );
                        log->fp = NULL;
                    }
                    log->fp = fopen ( log->path, "ab" );
                }
                pthread_mutex_unlock ( & log->lock );
                if ( unlikely ( NULL == log->fp ) )
                {
                    fprintf ( stderr, "[log]fopen( %s ) failed. errno=%d\n", log->path, get_errno () );
                    r = - EFAULT;
                    break;
                }
            }

            written = fwrite ( data, 1, data_len, log->fp );
            fflush ( log->fp );
            if ( written != data_len )
            {
                r = - get_errno ();
            }
        }
        while ( 0 );
    }

    if ( log->enable_stdout )
    {
        fwrite ( data, 1, data_len, stdout );
        fflush ( stdout );
    }

    if ( log->enable_stderr )
    {
        fwrite ( data, 1, data_len, stderr );
    }

    return r;
}

#endif // #if ENABLE_LOG

void apptool_t::log_write (
                            const char *    file,
                            int             line,
                            const char *    func,
                            log_level_t     level,
                            bool            is_user_log,
                            const char *    fmt,
                            ...
                            )
{
#if ENABLE_LOG
#ifdef _DEBUG
    char            _overflow_check_1[ 8 ];
#endif
    char            raw_buf[ 1024 ];
#ifdef _DEBUG
    char            _overflow_check_2[ 8 ];
#endif
#if defined( WIN32 ) || defined( WIN64 )
    int             e = ( int ) GetLastError ();
#else
    int             e = errno;
#endif
    char *          buf     = ( ( char * ) raw_buf ) + ( LOGTIME_LEN + 1 ); // add space
    size_t          buf_len = sizeof ( raw_buf ) - ( LOGTIME_LEN + 1 ); // add space
    const char *    p;
    const char *    level_s;
    va_list         ap;
    int             r;
    int             r2;
    log_t *         log_obj = & g_tools_log;
    const char      log_tail[] = "  " S_CRLF;
    time_info_t     nowt;
    static const level_item_t levels[ LOG_LEVEL_COUNT ] = {
                                                           // LOG_ALL      = 0,
        { "all   |", "all  *|" },

                                                           // LOG_DEBUG    = 1,
        { "debug |", "debug*|" },

                                                           // LOG_INFO     = 2,
        { "info  |", "info *|" },

                                                           // LOG_WARNING  = 3,
        { "warn  |", "warn *|" },

                                                           // LOG_ERROR    = 4,
        { "error |", "error*|" },

                                                           // LOG_FATAL    = 5,
        { "fatal |", "fatal*|" },

                                                           // LOG_NONE     = 6,
        { "?     |", "?     |" }
    };
    is_user_log = is_user_log ? 1 : 0;
    if ( unlikely ( level < 0 || level >= LOG_LEVEL_COUNT ) )
    {
        level = LOG_ERROR;
    }
#define LEADDING_BYTES  7

#ifdef _DEBUG
    _overflow_check_1[ 0 ] = '0';
    _overflow_check_1[ 1 ] = '1';
    _overflow_check_1[ 2 ] = '2';
    _overflow_check_1[ 3 ] = '3';
    _overflow_check_1[ 4 ] = '4';
    _overflow_check_1[ 5 ] = '5';
    _overflow_check_1[ 6 ] = '6';
    _overflow_check_1[ 7 ] = '7';
    _overflow_check_2[ 0 ] = '7';
    _overflow_check_2[ 1 ] = '6';
    _overflow_check_2[ 2 ] = '5';
    _overflow_check_2[ 3 ] = '4';
    _overflow_check_2[ 4 ] = '3';
    _overflow_check_2[ 5 ] = '2';
    _overflow_check_2[ 6 ] = '1';
    _overflow_check_2[ 7 ] = '0';
#endif

    if ( unlikely ( NULL == log_obj->fp && ! log_obj->enable_stdout && ! log_obj->enable_stderr ) )
    {
        // user disabled log
        return;
    }

    if ( unlikely ( ! get_current_time ( & nowt.year,
                                        & nowt.month,
                                        & nowt.day,
                                        & nowt.hour,
                                        & nowt.minute,
                                        & nowt.second,
                                        & nowt.ms ) )
         )
    {
        fprintf ( stderr, "[log]get_current_time failed. errno=%d\n", get_errno () );
        return;
    }

#if defined( WIN32 ) || defined( WIN64 )
    memset ( buf, 0, buf_len );
#endif

    level_s = levels[ level ].levels[ is_user_log ];

    p = strrchr ( file, S_PATH_SEP_C );
    if ( p )
    {
        file = p + 1;
    }

    va_start ( ap, fmt );
    r = vsnprintf ( & buf[ LEADDING_BYTES ], buf_len - LEADDING_BYTES, fmt, ap );
    va_end ( ap );
    buf[ buf_len - 1 ] = '\0';
    assert ( 7 == LEADDING_BYTES );
    fast_memcpy ( buf, level_s, 7 );

    if ( r >= ( int ) buf_len - LEADDING_BYTES )
    {
        r = buf_len - 1;
    }
    else if ( r < 0 )
    {
        r = ( int ) strlen ( buf );
    }
    else
    {
        r += LEADDING_BYTES;
    }
    if ( r > 0 )
    {
        if ( '\n' == buf[ r - 1 ] )
        {
            buf[ r - 1 ] = '\0';
            -- r;
        }
    }
    if ( r > 0 )
    {
        if ( '\r' == buf[ r - 1 ] )
        {
            buf[ r - 1 ] = '\0';
            -- r;
        }
    }

#define MIN_LOG_LEN     40
    if ( r < MIN_LOG_LEN )
    {
        memset ( & buf[ r ], ' ', MIN_LOG_LEN - r );
        r = MIN_LOG_LEN;
    }
    buf[ r ] = '\0';

#if defined( WIN32 ) || defined( WIN64 )
    r2 = snprintf ( & buf[ r ], buf_len - r,
                   " | [pid=%d][tid=%d][err=%d][%s:%d:%s]",
                   getpid (), gettid (), e, file, ( int ) line, func );
#else
    r2 = snprintf ( & buf[ r ], buf_len - r,
                   " | [pid=%d][tid=%d][err=%d,%s][%s:%d:%s]",
                   getpid (), gettid (), e, strerror (e), file, ( int ) line, func );
#endif
    if ( r2 >= ( int ) buf_len - r )
    {
        r = buf_len - 1;
    }
    else if ( r2 < 0 )
    {
        r += ( int ) strlen ( & buf[ r ] );
    }
    else
    {
        r += r2;
    }

    if ( buf_len - r >= sizeof ( log_tail ) )
    {
        fast_memcpy ( & buf[ r ], log_tail, sizeof (log_tail ) - 1 );
        r += sizeof (log_tail ) - 1;
    }
    else
    {
        fast_memcpy ( & buf[ buf_len - ( sizeof ( log_tail ) - 1 ) - 1 ],
                     log_tail, sizeof (log_tail ) - 1 );
        r = buf_len - 1;
    }
    buf[ r ] = '\0';

    sprintf ( raw_buf, LOGTIME_FMT,
             nowt.year, nowt.month, nowt.day,
             nowt.hour, nowt.minute, nowt.second, nowt.ms
             );
    raw_buf[ LOGTIME_LEN ] = ' ';

    log_write_file ( log_obj, & nowt, raw_buf, LOGTIME_LEN + 1 + r );

#ifdef _DEBUG
    if (    _overflow_check_1[ 0 ] != '0' ||
         _overflow_check_1[ 1 ] != '1' ||
         _overflow_check_1[ 2 ] != '2' ||
         _overflow_check_1[ 3 ] != '3' ||
         _overflow_check_1[ 4 ] != '4' ||
         _overflow_check_1[ 5 ] != '5' ||
         _overflow_check_1[ 6 ] != '6' ||
         _overflow_check_1[ 7 ] != '7' ||
         _overflow_check_2[ 0 ] != '7' ||
         _overflow_check_2[ 1 ] != '6' ||
         _overflow_check_2[ 2 ] != '5' ||
         _overflow_check_2[ 3 ] != '4' ||
         _overflow_check_2[ 4 ] != '3' ||
         _overflow_check_2[ 5 ] != '2' ||
         _overflow_check_2[ 6 ] != '1' ||
         _overflow_check_2[ 7 ] != '0'
         )
    {
        fprintf ( stderr, "memory error!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        log_error ( "memory error!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
        abort ();
    }
#endif
#endif // #if ENABLE_LOG
}

void apptool_t::log_write_huge (
                                 const char *    file,
                                 int             line,
                                 const char *    func,
                                 log_level_t     level,
                                 bool            is_user_log,
                                 const char *    data,
                                 size_t          data_len
                                 )
{
#if ENABLE_LOG
    log_t * log_obj = & g_tools_log;
    time_info_t     nowt;

    if ( unlikely ( NULL == log_obj->fp && ! log_obj->enable_stdout && ! log_obj->enable_stderr ) )
    {
        return;
    }

    if ( 0 == data_len )
    {
        return;
    }
    if ( unlikely ( ! get_current_time ( & nowt.year,
                                        & nowt.month,
                                        & nowt.day,
                                        & nowt.hour,
                                        & nowt.minute,
                                        & nowt.second,
                                        & nowt.ms ) )
         )
    {
        fprintf ( stderr, "[log]get_current_time failed. errno=%d\n", get_errno () );
        return;
    }

    log_write ( file, line, func, level, is_user_log, "[HUGE][len=%d]", ( int ) data_len );

    log_write_file ( log_obj, & nowt, data, ( int ) data_len );
    if ( '\n' != data[ data_len - 1 ] )
    {
        log_write_file ( log_obj, & nowt, S_CRLF, sizeof ( S_CRLF ) - 1 );
    }
#endif // #if ENABLE_LOG
}

void apptool_t::fmap_init (
                            fmap_t *    o
                            )
{
#if defined( WIN32 ) || defined( WIN64 )
    o->fd       = INVALID_HANDLE_VALUE;
    o->mfd      = NULL;
#endif
    o->ptr      = NULL;
    o->ptr_len  = 0;
}

bool apptool_t::fmap_open (
                            fmap_t *        o,
                            const char *    path,
                            size_t          offset,
                            size_t          len,
                            bool            read_write
                            )
{
#if defined( WIN32 ) || defined( WIN64 )
    DWORD   sz;
#else
    off_t   sz;
    int     fd;
#endif    
    fmap_init ( o );

#if defined( WIN32 ) || defined( WIN64 )

    o->fd = CreateFileA (
                         path,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );
    if ( INVALID_HANDLE_VALUE == o->fd )
    {
        return false;
    }

    if ( 0 != len )
    {
        sz = ( DWORD ) len;
    }
    else
    {
        sz = GetFileSize ( o->fd, NULL );
        if ( 0xFFFFFFFF == sz )
        {
            CloseHandle ( o->fd );
            o->fd = INVALID_HANDLE_VALUE;
            return false;
        }
    }

    if ( sz > 0 )
    {
        o->mfd = CreateFileMapping (
                                    o->fd,
                                    NULL,
                                    read_write ? PAGE_READWRITE : PAGE_READONLY,
                                    offset,
                                    sz,
                                    NULL );
        if ( NULL == o->mfd )
        {
            CloseHandle ( o->fd );
            o->fd = INVALID_HANDLE_VALUE;
            return false;
        }

        o->ptr = ( byte_t * ) MapViewOfFile (
                                             o->mfd,
                                             read_write ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
                                             0,
                                             0,
                                             sz );
        if ( NULL == o->ptr )
        {
            CloseHandle ( o->mfd );
            o->mfd = NULL;
            CloseHandle ( o->fd );
            o->fd = INVALID_HANDLE_VALUE;
            return false;
        }

        o->ptr_len = ( size_t ) sz;
    }
    else
    {
        o->mfd      = NULL;
        o->ptr      = NULL;
        o->ptr_len  = 0;
    }
    return true;

#else

#if defined( __linux )
    fd = open ( path, O_RDWR
#if ! defined( _OPENWRT )
        | O_LARGEFILE
#endif
        );
#else
    fd = open ( path, O_RDWR );
#endif
    if ( - 1 == fd )
    {
        return false;
    }

    if ( 0 != len )
    {
        sz = ( off_t ) len;
    }
    else
    {
        sz = lseek ( fd, 0, SEEK_END );
        if ( ( off_t ) - 1 == sz )
        {
            close ( fd );
            return false;
        }
    }

    if ( sz > 0 )
    {
        o->ptr = ( byte_t * ) mmap (
                                    NULL,
                                    ( size_t ) sz,
                                    read_write ? ( PROT_READ | PROT_WRITE ) : PROT_READ,
                                    MAP_SHARED,
                                    fd,
                                    offset );
        if ( NULL == o->ptr || MAP_FAILED == o->ptr )
        {
            close ( fd );
            return false;
        }

        close ( fd );
        o->ptr_len = ( size_t ) sz;
    }
    else
    {
        o->ptr      = NULL;
        o->ptr_len  = 0;
    }
    return true;
#endif
}

void apptool_t::fmap_close ( fmap_t * o )
{
#if defined( WIN32 ) || defined( WIN64 )
    if ( o->ptr )
    {
        UnmapViewOfFile ( o->ptr );
        o->ptr = NULL;
    }
    o->ptr_len = 0;

    if ( o->mfd )
    {
        CloseHandle ( o->mfd );
        o->mfd = NULL;
    }

    if ( INVALID_HANDLE_VALUE != o->fd )
    {
        CloseHandle ( o->fd );
        o->fd = INVALID_HANDLE_VALUE;
    }
#else
    if ( o->ptr && o->ptr_len )
    {
        munmap ( o->ptr, o->ptr_len );
    }
    o->ptr = NULL;
    o->ptr_len = 0;
#endif
}

bool apptool_t::fmap_flush (
                             fmap_t *        o
                             )
{
#if defined( WIN32 ) || defined( WIN64 )
    //TODO:
    return true;
#else

    int r = msync ( o->ptr, o->ptr_len, MS_SYNC );
    if ( unlikely ( 0 != r ) )
    {
        return false;
    }

    return true;
#endif
}

bool apptool_t::event_create (
                               event2_t * o
                               )
{
#if defined( WIN32 ) || defined( WIN64 )
    assert ( o );
    o->is_inited = false;
    o->cond = CreateEvent (
                           NULL,
                           FALSE,
                           FALSE,
                           NULL );
    if ( NULL == o->cond )
    {
        return false;
    }
    
    o->is_inited = true;
    
    return true;
#else
    int r;
    
    assert ( o );
    o->is_inited = false;
    r = sem_init (
                  & o->cond,
                  0,
                  0 );
    if ( unlikely ( - 1 == r ) )
    {
        return false;
    }
    
    o->is_inited = true;
    
    return true;
#endif
}

void apptool_t::event_destroy (
                                event2_t * o
                                )
{
    assert ( o );
    if ( o->is_inited )
    {
#if defined( WIN32 ) || defined( WIN64 )
        assert ( o->cond );
        CloseHandle ( o->cond );
        o->cond = NULL;
#else
        sem_destroy ( & o->cond );
#endif
        o->is_inited = false;
    }
}

bool apptool_t::event_alarm (
                              event2_t * o
                              )
{
    assert ( o );
    if ( o->is_inited )
    {
#if defined( WIN32 ) || defined( WIN64 )
        return SetEvent ( o->cond ) ? true : false;
#else
        return 0 == sem_post ( & o->cond ) ? true : false;
#endif
    }
    else
    {
        return false;
    }
}

int apptool_t::event_wait (
                            event2_t * o,
                            unsigned int ms
                            )
{
#if defined( WIN32 ) || defined( WIN64 )
    DWORD f;
    assert ( o );

    if ( ! o->is_inited )
    {
        return - 1;
    }

    f = WaitForSingleObject ( o->cond, ms );
    switch ( f )
    {
        case WAIT_OBJECT_0:
            return 1;
            
        case WAIT_TIMEOUT:
            return 0;
    }

    return - 1;
    
#else

    int             r;
    struct timeval  tv;
    struct timespec to;

    assert ( o );

    if ( ! o->is_inited )
    {
        return - 1;
    }

    if ( INFINITE == ms )
    {
        to.tv_sec   = 0;
        to.tv_nsec  = 0;
        tv.tv_sec   = 0;
        tv.tv_usec  = 0;
        r = sem_wait ( & o->cond );
    }
    else
    {

        if ( 0 != gettimeofday ( & tv, NULL ) )
        {
            return - 1;
        }

        to.tv_sec = tv.tv_sec + ms / 1000;
        to.tv_nsec = tv.tv_usec * 1000 + ( ms % 1000 ) * 1000000;
        if ( to.tv_nsec >= 1000000000 )
        {
            to.tv_sec  += ( to.tv_nsec / 1000000000 );
            to.tv_nsec %= 1000000000;
        }

        r = sem_timedwait ( & o->cond, & to );
    }
    if ( 0 == r )
    {
        return 1;
    }

    switch ( errno )
    {
        case 0:
            return 1;
            
        case ETIMEDOUT:
            return 0;
            
        case EINTR:
            return - 2;
    }

    return - 1;

#endif
}

uint32_t apptool_t::fast_crc32 (
                                 const char * data,
                                 size_t len
                                 )
{
    uint32_t sum;

    for ( sum = 0; len; len -- )
    {
        sum = sum >> 1 | sum << 31;
        sum += * data ++;
    }

    return sum;
}

uint32_t apptool_t::hust_hash_key (
                                    const char * data,
                                    size_t len
                                    )
{
    uint32_t  i, key;

    key = 0;

    for ( i = 0; i < len; i ++ )
    {
        key = hust_hash ( key, data[ i ] );
    }

    return key;
}

uint16_t apptool_t::bucket_hash (
                                  const char * data,
                                  size_t len
                                  )
{
    uint32_t hash = hust_hash_key ( data, len );

    return ( uint16_t ) hash % MAX_BUCKET_NUM;
}

uint16_t apptool_t::locker_hash (
                                  const char * data,
                                  size_t len
                                  )
{
    uint32_t hash = hust_hash_key ( data, len );

    return ( uint16_t ) hash % MAX_LOCKER_NUM;
}

void apptool_t::md5 (
                      const void * data,
                      size_t data_len,
                      char * digest
                      )
{
    MD5_CTX ctx;

    if ( unlikely ( NULL == data || 0 == data_len ) )
    {
        memset ( digest, 0, 16 );
        return;
    }

    MD5Init ( & ctx );
    MD5Update ( & ctx, ( const unsigned char * ) data, ( unsigned int ) data_len );
    MD5Final ( & ctx, ( unsigned char * ) digest );
}

bool apptool_t::check_endian ()
{
  	int i = 1;
	i = * ( char * ) & i;
	return i == 1;
}
