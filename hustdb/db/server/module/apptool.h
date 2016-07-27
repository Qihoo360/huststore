#ifndef _apptool_h_
#define _apptool_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "utils/md5.h"

#if ! defined( WIN32 ) && ! defined( WIN64 )
#include <semaphore.h>
#define INFINITE    0xFFFFFFFF
#endif

    struct event2_t;
typedef struct event2_t event2_t;

struct event2_t
{
#if defined( WIN32 ) || defined( WIN64 )
    HANDLE cond;
#else
    sem_t cond;
#endif
    bool is_inited;
};

    struct fmap_t;
typedef struct fmap_t fmap_t;

typedef struct fmap_t
{
#if defined( WIN32 ) || defined( WIN64 )
    HANDLE fd;
    HANDLE mfd;
#endif
    unsigned char * ptr;
    size_t ptr_len;

} fmap_t;

typedef enum
{
    LOG_ALL = 0,
    LOG_DEBUG = 1,
    LOG_INFO = 2,
    LOG_WARNING = 3,
    LOG_ERROR = 4,
    LOG_FATAL = 5,
    LOG_NONE = 6,

    LOG_LEVEL_COUNT = 7
} log_level_t;

#define MAX_BUCKET_NUM           1024
#define MAX_LOCKER_NUM           2048
#define hust_hash(key, c)        ((unsigned int) key * 31 + c)

#define G_APPTOOL                apptool_t::get_apptool ()

class apptool_t
{
public:

    ~apptool_t ( );

public:

    static apptool_t * get_apptool ( );
    static void kill_me ( );

    void set_hustdb ( void * db );
    void * get_hustdb ( );

public:

    bool is_dir (
                  const char * path
                  );

    bool is_file (
                   const char * path
                   );

    bool make_dir (
                    const char * dir
                    );

    void path_to_os (
                      char * path
                      );

    size_t get_exe_path (
                          char * path,
                          size_t path_len
                          );

    size_t get_exe_dir (
                         char * dir,
                         size_t dir_len,
                         bool add_sep
                         );

    void sleep_ms ( unsigned int ms );

    bool set_block (
                     int fd,
                     bool is_block
                     );

    bool get_current_time (
                            int * year,
                            int * month,
                            int * day,
                            int * hour,
                            int * minute,
                            int * second,
                            int * ms
                            );

    unsigned int get_tick_count ( );

public:

#ifdef log_fatal
#undef log_fatal
#endif
#ifdef log_error
#undef log_error
#endif
#ifdef log_warning
#undef log_warning
#endif
#ifdef log_info
#undef log_info
#endif
#ifdef log_debug
#undef log_debug
#endif

#define log_fatal(str, ...)      G_APPTOOL->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_FATAL,   true, str, ##__VA_ARGS__)
#define log_error(str, ...)      G_APPTOOL->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_ERROR,   true, str, ##__VA_ARGS__)
#define log_warning(str, ...)    G_APPTOOL->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_WARNING, false, str, ##__VA_ARGS__)
#define log_info(str, ...)       G_APPTOOL->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_INFO,    false, str, ##__VA_ARGS__)
#ifdef _DEBUG
#define log_debug(str, ...)      G_APPTOOL->log_write( __FILE__, __LINE__, __FUNCTION__, LOG_DEBUG,   false, str, ##__VA_ARGS__)
#else
#define log_debug(str, ...)
#endif

    int log_open (
                   const char * log_dir,
                   const char * log_name,
                   bool redirect_stdout
                   );

    void log_enable_stdout ( bool enable );

    void log_enable_stderr ( bool enable );

    void log_close ( );

    void log_write (
                     const char * file,
                     int line,
                     const char * func,
                     log_level_t level,
                     bool is_user_log,
                     const char * fmt,
                     ...
                     );

    void log_write_huge (
                          const char * file,
                          int line,
                          const char * func,
                          log_level_t level,
                          bool is_user_log,
                          const char * data,
                          size_t data_len
                          );

public:

    void fmap_init (
                     fmap_t * o
                     );

    bool fmap_open (
                     fmap_t * o,
                     const char * path,
                     size_t offset,
                     size_t len,
                     bool read_write
                     );

    bool fmap_flush (
                      fmap_t * o
                      );

    void fmap_close (
                      fmap_t * o
                      );

public:
    
    bool event_create (
                        event2_t * o
                        );

    void event_destroy (
                         event2_t * o
                         );

    bool event_alarm (
                       event2_t * o
                       );

    int event_wait (
                     event2_t * o,
                     unsigned int ms
                     );

public:

    uint32_t fast_crc32 (
                          const char * data,
                          size_t len
                          );

    uint32_t hust_hash_key (
                             const char * data,
                             size_t len
                             );

    uint16_t bucket_hash (
                           const char * data,
                           size_t len
                           );

    uint16_t locker_hash (
                           const char * data,
                           size_t len
                           );

    void md5 (
               const void * data,
               size_t data_len,
               char * digest
               );

private:

    void * m_hustdb;
    static apptool_t * m_apptool;

private:
    // disable
    apptool_t ( );
    apptool_t ( const apptool_t & );
    const apptool_t & operator= ( const apptool_t & );
};

#endif // #ifndef _apptool_h_
