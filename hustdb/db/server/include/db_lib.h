#ifndef _hustdb_lib_h_
#define _hustdb_lib_h_

#if ! defined( WIN32 ) && ! defined( WIN64 )
#include <sys/un.h>
#include <netinet/in.h>
#if defined( __linux )
#include <semaphore.h>
#elif defined( __APPLE__ )
#include <mach/semaphore.h>
#include <mach/mach.h>
#endif // #if defined( __linux )
#endif // #if ! defined( WIN32 ) && ! defined( WIN64 )
#include <stdarg.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#endif // #ifdef __cplusplus

#if defined( WIN32 ) || defined( WIN64 )
#ifdef _WIN32_WCE
#include <Ws2tcpip.h>
#pragma comment( lib, "ws2.lib" )
#else
#include <Iphlpapi.h>
#include <Sensapi.h>
#pragma comment( lib, "Iphlpapi.lib" )
#pragma comment( lib, "Sensapi.lib" )
#pragma comment( lib, "ws2_32.lib" )
#endif
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>

#if defined(__linux) || defined(__APPLE__) || defined(__FreeBSD__)
#include <ifaddrs.h>
#if defined( __linux )
#include <sys/epoll.h>
#endif
#else
#include <sys/ioctl.h>
#include <net/if.h>
#ifdef __sun
#include <sys/sockio.h>
#endif
#endif
#endif
#ifdef __cplusplus
#include <list>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _base_string_h_

    typedef struct const_str
    {
        const char * ptr;
        int len;
    } const_str;

    typedef struct const_pair
    {
        const_str name;
        const_str value;
    } const_pair;

    typedef struct const_three
    {
        const_str one;
        const_str two;
        const_str three;
    } const_three;

#endif // #ifndef _base_string_h_

    typedef const_str const_str_t;
    typedef const_pair const_pair_t;

    typedef struct regex_match_item_t
    {
        int begin;
        int end;
    } regex_match_item_t;

    typedef const_str_t url_value_t;
    typedef const_pair_t url_pair_t;


#if ( defined( WIN32 ) || defined( WIN64 ) )

#if _MSC_VER <= 1200
    typedef long intptr_t;
#endif

    struct dirent
    {
        unsigned short d_namlen;
#define d_reclen    d_namlen
        char * d_name;
        unsigned char d_type; // 4->directory; 8->file
    };

    typedef struct DIR
    {
        WIN32_FIND_DATAA dd_dta;
        struct dirent dd_dir;
        HANDLE dd_handle;
        int dd_stat;
        char dd_name[ MAX_PATH ];
    } DIR;

#endif // #if ( defined( WIN32 ) || defined( WIN64 ) )

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#endif // #ifdef __cplusplus

#ifndef _base_string_h_

#ifdef __cplusplus
extern "C"
{
#endif

    static inline int isspace2 ( char c )
    {
        switch ( c )
        {
            case 0x20:
            case 0x09:
            case 0x0A:
            case 0x0D:
            case 0x00:
            case 0x0B:
                return 1;
            default:
                break;
        };

        if ( c > 0 )
        {
            return isspace ( c );
        }

        return 0;
    }

    static inline char * str_trim (
                                    char * s,
                                    int * len
                                    )
    {
        char * t;
        int n;
        char * orig_s;
        char * orig_e;

        assert ( s );
        if ( len && *len >= 0 )
        {
            n = *len;
        }
        else
        {
            n = ( int ) strlen ( s );
        }
        orig_s = s;
        orig_e = s + n;

        // de pre space
        while ( n > 0 && isspace2 ( * s ) )
        {
            ++s;
            --n;
        };

        // de tail space
        if ( n > 0 )
        {
            t = s + n;
            while ( n > 0 && isspace2 ( * ( t - 1 ) ) )
            {
                --t;
                --n;
            };
            if ( t < orig_e )
            {
                * t = '\0';
            }
        }

        if ( len )
        {
            * len = n;
        }
        return s;
    }

#if (S_LITTLE_ENDIAN)
#define fast_mem2cmp(m, c0, c1)                                                \
        ( *(uint16_t *)(m) == (c1 << 8 | c0) )
#define fast_mem3cmp(m, c0, c1, c2)                                            \
        ( fast_mem2cmp((m), c0, c1 ) && ( (m)[2] == c2 ) )
#define fast_mem4cmp(m, c0, c1, c2, c3)                                        \
        ( *(uint32_t *)(m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) )
#define fast_mem5cmp(m, c0, c1, c2, c3, c4)                                    \
        ( *(uint32_t *)(m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)       \
        && (m)[4] == c4 )
#define fast_mem6cmp(m, c0, c1, c2, c3, c4, c5)                                \
        ( *(uint32_t *)(m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)       \
        && (((uint32_t *)(m))[1] & 0xffff) == ((c5 << 8) | c4) )
#define fast_mem7cmp(m, c0, c1, c2, c3, c4, c5, c6)                            \
        ( *(uint32_t *)(m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)       \
        && (((uint32_t *)(m))[1] & 0xffffff) == ( (c6 << 16) | (c5 << 8) | c4) )
#define fast_mem8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
        ( *(uint32_t *)(m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)       \
        && ((uint32_t *)(m))[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4) )
#define fast_mem9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                    \
                    ( fast_mem8cmp((m), c0, c1, c2, c3, c4, c5, c6, c7) && (m)[8] == c8 )
#else
#define fast_mem2cmp(m, c0, c1)                                                \
        ( (m)[0] == c0 && (m)[1] == c1 )
#define fast_mem3cmp(m, c0, c1, c2)                                            \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 )
#define fast_mem4cmp(m, c0, c1, c2, c3)                                        \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 && (m)[3] == c3 )
#define fast_mem5cmp(m, c0, c1, c2, c3, c4)                                    \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 && (m)[3] == c3 && (m)[4] == c4 )
#define fast_mem6cmp(m, c0, c1, c2, c3, c4, c5)                                \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 && (m)[3] == c3                \
        && (m)[4] == c4 && (m)[5] == c5 )
#define fast_mem7cmp(m, c0, c1, c2, c3, c4, c5, c6)                            \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 && (m)[3] == c3                \
        && (m)[4] == c4 && (m)[5] == c5 && (m)[6] == c6 )
#define fast_mem8cmp(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 && (m)[3] == c3                \
        && (m)[4] == c4 && (m)[5] == c5 && (m)[6] == c6 && (m)[7] == c7 )
#define fast_mem9cmp(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                    \
        ( (m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2 && (m)[3] == c3                \
        && (m)[4] == c4 && (m)[5] == c5 && (m)[6] == c6 && (m)[7] == c7 && (m)[8] == c8 )
#endif

#if (S_LITTLE_ENDIAN)
#define fast_mem2cpy(m, c0, c1)                                                \
        ( *(uint16_t *)(m) = (c1 << 8 | c0) )
#define fast_mem3cpy(m, c0, c1, c2)                                            \
        ( fast_mem2cpy((m), c0, c1 ), ( (m)[2] = c2 ) )
#define fast_mem4cpy(m, c0, c1, c2, c3)                                        \
        ( *(uint32_t *)(m) = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) )
#define fast_mem5cpy(m, c0, c1, c2, c3, c4)                                    \
        ( *(uint32_t *)(m) = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)        \
        , (m)[4] = c4 )
#define fast_mem6cpy(m, c0, c1, c2, c3, c4, c5)                                \
        ( *(uint32_t *)(m) = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)        \
        , (m)[4] = c4, (m)[5] = c5 )
#define fast_mem7cpy(m, c0, c1, c2, c3, c4, c5, c6)                            \
        ( *(uint32_t *)(m) = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)        \
        , (m)[4] = c4, (m)[5] = c5, (m)[6] = c6 )
#define fast_mem8cpy(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
        ( *(uint32_t *)(m) = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)        \
        , ((uint32_t *)(m))[1] = ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4) )
#define fast_mem9cpy(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                    \
        ( *(uint32_t *)(m) = ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)        \
        , ((uint32_t *)(m))[1] = ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4)    \
        , (m)[8] = c8 )
#else
#define fast_mem2cpy(m, c0, c1)                                                \
        ( (m)[0] = c0, (m)[1] = c1 )
#define fast_mem3cpy(m, c0, c1, c2)                                            \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2 )
#define fast_mem4cpy(m, c0, c1, c2, c3)                                        \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2, (m)[3] == c3 )
#define fast_mem5cpy(m, c0, c1, c2, c3, c4)                                    \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2, (m)[3] = c3, (m)[4] == c4 )
#define fast_mem6cpy(m, c0, c1, c2, c3, c4, c5)                                \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2, (m)[3] == c3                         \
        , (m)[4] = c4, (m)[5] = c5 )
#define fast_mem7cpy(m, c0, c1, c2, c3, c4, c5, c6)                            \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2, (m)[3] = c3                          \
        , (m)[4] = c4, (m)[5] = c5, (m)[6] = c6 )
#define fast_mem8cpy(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2, (m)[3] = c3                          \
        , (m)[4] = c4, (m)[5] = c5, (m)[6] = c6, (m)[7] = c7 )
#define fast_mem9cpy(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                    \
        ( (m)[0] = c0, (m)[1] = c1, (m)[2] = c2, (m)[3] = c3                          \
        , (m)[4] = c4, (m)[5] = c5, (m)[6] = c6, (m)[7] = c7, (m)[8] = c8 )
#endif

    static inline
    void fast_memcpy (
                       void * dest,
                       const void * src,
                       size_t len
                       )
    {
        switch ( len )
        {
            case 0:
                break;
            case 1:
                ( ( char * ) dest )[0] = ( ( const char * ) src )[0];
                break;
            case 2:
                fast_mem2cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1]
                               );
                break;
            case 3:
                fast_mem3cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2]
                               );
                break;
            case 4:
                fast_mem4cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2],
                               ( ( const unsigned char * ) src )[3]
                               );
                break;
            case 5:
                fast_mem5cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2],
                               ( ( const unsigned char * ) src )[3],
                               ( ( const unsigned char * ) src )[4]
                               );
                break;
            case 6:
                fast_mem6cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2],
                               ( ( const unsigned char * ) src )[3],
                               ( ( const unsigned char * ) src )[4],
                               ( ( const unsigned char * ) src )[5]
                               );
                break;
            case 7:
                fast_mem7cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2],
                               ( ( const unsigned char * ) src )[3],
                               ( ( const unsigned char * ) src )[4],
                               ( ( const unsigned char * ) src )[5],
                               ( ( const unsigned char * ) src )[6]
                               );
                break;
            case 8:
                fast_mem8cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2],
                               ( ( const unsigned char * ) src )[3],
                               ( ( const unsigned char * ) src )[4],
                               ( ( const unsigned char * ) src )[5],
                               ( ( const unsigned char * ) src )[6],
                               ( ( const unsigned char * ) src )[7]
                               );
                break;
            case 9:
                fast_mem9cpy ( ( unsigned char * ) dest,
                               ( ( const unsigned char * ) src )[0],
                               ( ( const unsigned char * ) src )[1],
                               ( ( const unsigned char * ) src )[2],
                               ( ( const unsigned char * ) src )[3],
                               ( ( const unsigned char * ) src )[4],
                               ( ( const unsigned char * ) src )[5],
                               ( ( const unsigned char * ) src )[6],
                               ( ( const unsigned char * ) src )[7],
                               ( ( const unsigned char * ) src )[8]
                               );
                break;
            default:
                memcpy ( dest, src, len );
                break;
        }
    }

    static inline
    bool mem_equal (
                     const void * lhd,
                     const void * rhd,
                     size_t len
                     )
    {
        switch ( len )
        {
            case 0:
                return true;
            case 1:
                return (( const char * ) lhd )[0] == ( ( const char * ) rhd )[0];
            case 2:
                return fast_mem2cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1]
                                      );
            case 3:
                return fast_mem3cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2]
                                      );
            case 4:
                return fast_mem4cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2],
                                      ( ( const unsigned char * ) rhd )[3]
                                      );
            case 5:
                return fast_mem5cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2],
                                      ( ( const unsigned char * ) rhd )[3],
                                      ( ( const unsigned char * ) rhd )[4]
                                      );
            case 6:
                return fast_mem6cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2],
                                      ( ( const unsigned char * ) rhd )[3],
                                      ( ( const unsigned char * ) rhd )[4],
                                      ( ( const unsigned char * ) rhd )[5]
                                      );
            case 7:
                return fast_mem7cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2],
                                      ( ( const unsigned char * ) rhd )[3],
                                      ( ( const unsigned char * ) rhd )[4],
                                      ( ( const unsigned char * ) rhd )[5],
                                      ( ( const unsigned char * ) rhd )[6]
                                      );
            case 8:
                return fast_mem8cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2],
                                      ( ( const unsigned char * ) rhd )[3],
                                      ( ( const unsigned char * ) rhd )[4],
                                      ( ( const unsigned char * ) rhd )[5],
                                      ( ( const unsigned char * ) rhd )[6],
                                      ( ( const unsigned char * ) rhd )[7]
                                      );
            case 9:
                return fast_mem9cmp ( ( const unsigned char * ) lhd,
                                      ( ( const unsigned char * ) rhd )[0],
                                      ( ( const unsigned char * ) rhd )[1],
                                      ( ( const unsigned char * ) rhd )[2],
                                      ( ( const unsigned char * ) rhd )[3],
                                      ( ( const unsigned char * ) rhd )[4],
                                      ( ( const unsigned char * ) rhd )[5],
                                      ( ( const unsigned char * ) rhd )[6],
                                      ( ( const unsigned char * ) rhd )[7],
                                      ( ( const unsigned char * ) rhd )[8]
                                      );
            default:
                return 0 == memcmp ( lhd, rhd, len );
        }
    }

#ifdef __cplusplus
}
#endif

#endif // #ifndef _base_string_h_

#ifndef _base_lock_h_
#define _base_lock_h_

#ifdef __cplusplus
extern "C"
{
#endif

#if defined( WIN32 ) || defined( WIN64 )

    typedef struct pthread_mutex_t
    {
        CRITICAL_SECTION lock;

        unsigned char is_init;

    } pthread_mutex_t;

    /**
     * @function pthread_mutex_init
     * @brief 
     * @param[in] pthread_mutex_t * p:
     * @param[in] void * zero: not used, must be zero
     */
    static inline
    int pthread_mutex_init ( pthread_mutex_t * p, void * zero )
    {
        InitializeCriticalSection ( & p->lock );
        p->is_init = 1;
        return 0;
    }

    /**
     * @function pthread_mutex_destroy
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    static inline
    void pthread_mutex_destroy ( pthread_mutex_t * p )
    {
        DeleteCriticalSection ( & p->lock );
        p->is_init = 0;
    }

    /**
     * @function pthread_mutex_lock
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    static inline
    void pthread_mutex_lock ( pthread_mutex_t * p )
    {
        if ( !p->is_init )
        {
            pthread_mutex_init ( p, NULL );
        }

        EnterCriticalSection ( & p->lock );
    }

    /**
     * @function pthread_mutex_unlock
     * @brief 
     * @param[in] pthread_mutex_t * p: 
     */
    static inline
    void pthread_mutex_unlock ( pthread_mutex_t * p )
    {
        LeaveCriticalSection ( & p->lock );
    }

#else

    typedef pthread_mutex_t CRITICAL_SECTION;

    /**
     * @function InitializeCriticalSection
     * @brief 
     * @param[out] CRITICAL_SECTION * p: 
     */
    static inline
    void InitializeCriticalSection ( CRITICAL_SECTION * p )
    {
        pthread_mutex_init ( p, NULL );
    }

    /**
     * @function DeleteCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    static inline
    void DeleteCriticalSection ( CRITICAL_SECTION * p )
    {
        pthread_mutex_destroy ( p );
    }

    /**
     * @function EnterCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    static inline
    void EnterCriticalSection ( CRITICAL_SECTION * p )
    {
        pthread_mutex_lock ( p );
    }

    /**
     * @function LeaveCriticalSection
     * @brief 
     * @param[in] CRITICAL_SECTION * p: 
     */
    static inline
    void LeaveCriticalSection ( CRITICAL_SECTION * p )
    {
        pthread_mutex_unlock ( p );
    }

#endif

#ifdef __cplusplus
}

#define DEBUG_LOCK  0

/**
 * @class lockable_t
 * @brief 
 */
struct lockable_t
{
public:

    lockable_t ( )
#if DEBUG_LOCK
    : m_file ( NULL ), m_line ( 0 )
#endif
    {
#if defined( WIN32 ) || defined( WIN64 )
        pthread_mutex_init ( & m_lock, NULL );
#elif defined(__FreeBSD__) || defined(__APPLE__)
        pthread_mutex_init ( & m_lock, NULL );
#else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init ( & attr );
        pthread_mutexattr_settype ( & attr, PTHREAD_MUTEX_RECURSIVE_NP );
        pthread_mutex_init ( & m_lock, & attr );
        pthread_mutexattr_destroy ( & attr );
#endif
    }

    ~lockable_t ( )
    {
        pthread_mutex_destroy ( & m_lock );
    }

    pthread_mutex_t * get ( )
    {
        return & m_lock;
    }

    void lock ( )
    {
        pthread_mutex_lock ( & m_lock );
    }

    void unlock ( )
    {
#if DEBUG_LOCK
        if ( NULL != m_file || 0 != m_line )
        {
            //printf( "%s:%d leave\n",
            //    m_file, m_line );
            m_file = NULL;
            m_line = 0;
        }
        else
        {
            printf ( "double leave\n" );
        }
#endif
        pthread_mutex_unlock ( & m_lock );
    }

#if DEBUG_LOCK

    void lock ( const char * file, int line )
    {
        if ( NULL != m_file || 0 != m_line )
        {
            printf ( "%s:%d coming, but %s:%d already lock\n",
                     file, line, m_file, m_line );
        }
        else
        {
            //printf( "%s:%d coming\n", file, line );
            m_file = file;
            m_line = line;
        }

        pthread_mutex_lock ( & m_lock );
    }
#else

    void lock ( const char *, int )
    {
        pthread_mutex_lock ( & m_lock );
    }
#endif

private:
    pthread_mutex_t m_lock;
#if DEBUG_LOCK
    const char * m_file;
    int m_line;
#endif

private:
    // disable
    lockable_t ( const lockable_t & );
    const lockable_t & operator= ( const lockable_t & );
};

/**
 * @class scope_lock_t
 * @brief 
 */
struct scope_lock_t
{
public:

    explicit scope_lock_t ( lockable_t & lock ) : m_lock ( lock )
    {
        m_lock.lock ( );
    }

    ~scope_lock_t ( )
    {
        m_lock.unlock ( );
    }

#if DEBUG_LOCK

    explicit scope_lock_t ( lockable_t & lock, const char * file, int line )
    : m_lock ( lock )
    {
        m_lock.lock ( file, line );
    }
#else

    explicit scope_lock_t ( lockable_t & lock, const char *, int ) : m_lock ( lock )
    {
        m_lock.lock ( );
    }
#endif

private:
    lockable_t & m_lock;

private:
    // disable
    scope_lock_t ( );
    scope_lock_t ( const scope_lock_t & );
    const scope_lock_t & operator= ( const scope_lock_t & );
};

#endif // #ifdef __cplusplus

///////////////////////////////////////////////////////////////////////
//
// rwlockable_t, scope_rlock_t, scope_wlock_t
//

#ifdef __cplusplus

#if defined( WIN32 ) || defined( WIN64 )

struct rwlockable_t
{
public:

    rwlockable_t ( ) : m_nReaders ( 0 ), m_nWriters ( 0 )
    {
        m_hDataEvent = CreateEvent (
                                     NULL, // no security attributes
                                     FALSE, // Auto reset event
                                     FALSE, // initially set to non signaled state
                                     NULL ); // un named event
        InitializeCriticalSection ( & m_WriteLock );
    }

    ~rwlockable_t ( )
    {
        DeleteCriticalSection ( & m_WriteLock );
        CloseHandle ( m_hDataEvent );
    }

    void rlock ( ) const
    {
        while ( m_nReaders > 0 )
        {
            WaitForSingleObject ( m_hDataEvent, 50 );
        }

        InterlockedIncrement ( & m_nReaders );
    }

    void runlock ( ) const
    {
        long n = InterlockedDecrement ( & m_nReaders );
        if ( 0 == n )
        {
            SetEvent ( m_hDataEvent );
        }
    }

    void wlock ( ) const
    {
        while ( m_nReaders > 0 || m_nWriters > 0 )
        {
            WaitForSingleObject ( m_hDataEvent, 50 );
        }

        InterlockedIncrement ( & m_nWriters );

        EnterCriticalSection ( & m_WriteLock );
    }

    void wunlock ( ) const
    {
        LeaveCriticalSection ( & m_WriteLock );

        long n = InterlockedDecrement ( & m_nWriters );

        if ( 0 == n )
        {
            SetEvent ( m_hDataEvent );
        }
    }

private:
    mutable volatile long m_nReaders;
    mutable volatile long m_nWriters;
    mutable CRITICAL_SECTION m_WriteLock;
    mutable HANDLE m_hDataEvent;

private:
    // disable
    rwlockable_t ( const rwlockable_t & );
    const rwlockable_t & operator= ( const rwlockable_t & );
};

#else

struct rwlockable_t
{
public:

    rwlockable_t ( )
    {
        pthread_rwlock_init ( & m_lock, NULL );
    }

    ~rwlockable_t ( )
    {
        pthread_rwlock_destroy ( & m_lock );
    }

    void rlock ( ) const
    {
        pthread_rwlock_rdlock ( & m_lock );
    }

    void runlock ( ) const
    {
        pthread_rwlock_unlock ( & m_lock );
    }

    void wlock ( ) const
    {
        pthread_rwlock_wrlock ( & m_lock );
    }

    void wunlock ( ) const
    {
        pthread_rwlock_unlock ( & m_lock );
    }

private:
    mutable pthread_rwlock_t m_lock;

private:
    // disable
    rwlockable_t ( const rwlockable_t & );
    const rwlockable_t & operator= ( const rwlockable_t & );
};

#endif

struct scope_rlock_t
{
public:

    explicit scope_rlock_t ( rwlockable_t & lock ) : m_lock ( lock )
    {
        m_lock.rlock ( );
    }

    ~scope_rlock_t ( )
    {
        m_lock.runlock ( );
    }

private:
    rwlockable_t & m_lock;

private:
    // disable
    scope_rlock_t ( );
    scope_rlock_t ( const scope_rlock_t & );
    const scope_rlock_t & operator= ( const scope_rlock_t & );
};

struct scope_wlock_t
{
public:

    explicit scope_wlock_t ( rwlockable_t & lock ) : m_lock ( lock )
    {
        m_lock.wlock ( );
    }

    ~scope_wlock_t ( )
    {
        m_lock.wunlock ( );
    }

private:
    rwlockable_t & m_lock;

private:
    // disable
    scope_wlock_t ( );
    scope_wlock_t ( const scope_wlock_t & );
    const scope_wlock_t & operator= ( const scope_wlock_t & );
};

#endif // #ifdef __cplusplus

#endif // #ifndef _base_lock_h_

#ifndef _base_string_h_
#ifdef __cplusplus

static inline
bool to_vector ( const std::string & src, const std::string & sep, std::vector< std::string > & result )
{
    result.resize ( 0 );

    if ( sep.empty ( ) )
    {
        return false;
    }

    int count = 1;
    size_t pos = 0;

    while ( pos < src.size ( ) )
    {

        pos = src.find ( sep.c_str ( ), pos );
        if ( pos == std::string::npos )
        {
            break;
        }

        ++count;

        pos += sep.size ( );
    }

    if ( result.capacity ( ) < ( size_t ) count )
    {
        try
        {
            result.reserve ( count );
        }
        catch ( ... )
        {
            return false;
        }
    }

    std::string t;
    const char * p;

    pos = 0;
    size_t pos2 = 0;
    while ( pos < src.size ( ) )
    {

        pos2 = src.find ( sep.c_str ( ), pos );
        if ( std::string::npos == pos2 )
        {
            try
            {
                t = src.substr ( pos );
                p = str_trim ( ( char * ) t.c_str ( ), NULL );
                result.push_back ( p ? p : "" );
            }
            catch ( ... )
            {
                result.resize ( 0 );
                return false;
            }
            break;
        }
        else
        {
            try
            {
                t = src.substr ( pos, pos2 - pos );
                p = str_trim ( ( char * ) t.c_str ( ), NULL );
                result.push_back ( p ? p : "" );
            }
            catch ( ... )
            {
                result.resize ( 0 );
                return false;
            }
        }

        pos = pos2 + sep.size ( );
    }

    if ( result.size ( ) < ( size_t ) count )
    {
        result.resize ( ( size_t ) count );
    }

    return true;
}

static inline
int stdstr_replace ( std::string & s, const char * lpszOld, const char * lpszNew )
{
    if ( s.empty ( ) )
        return 0;
    if ( NULL == lpszOld || '\0' == *lpszOld )
        return 0;

    int count = 0;

    if ( NULL == lpszNew )
        lpszNew = "";

    std::string src = lpszOld;
    std::string dest = lpszNew;

    size_t srclen = src.size ( );
    size_t dstlen = dest.size ( );

    if ( srclen == dstlen )
    {
        // from begin to end
        size_t pos = 0;
        while ( ( pos = s.find ( src, pos ) ) != std::string::npos )
        {
            fast_memcpy ( & s[ pos ], dest.c_str ( ), dstlen );
            ++count;
            pos += srclen;
        }
    }
    else
    {
        // from tail to begin
        size_t pos = std::string::npos;
        while ( ( pos = s.rfind ( src, pos ) ) != std::string::npos )
        {
            s.replace ( pos, srclen, dest );

            ++count;

            if ( 0 == pos )
                break;

            --pos;
        }
    }

    return count;
}
#endif // #ifdef __cplusplus
#endif // #ifndef _base_string_h_

#endif // #ifndef _hustdb_lib_h_
