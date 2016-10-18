#ifndef _hustdb_std_include_h_
#define _hustdb_std_include_h_

#if defined( _WIN32 )
#ifndef WIN32
#define WIN32
#endif
#elif defined( _WIN64 )
#ifndef WIN64
#define WIN64
#endif
#endif

#if ( defined( WIN32 ) || defined( WIN64 ) ) && defined( _DEBUG )
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#if defined( WIN32 ) || defined( WIN64 )

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#pragma warning(disable:4996)

#pragma warning(disable:4819)

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501 // CreateWaitableTimer, LPFN_CONNECTEX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wsipx.h>
#include <process.h>    // _beginthreadex
#include <io.h>
#include <Tlhelp32.h>

#pragma comment( lib, "Iphlpapi.lib" )
#pragma comment( lib, "ws2_32.lib" )

#define __attribute__(X)

/* Standard file descriptors.  */
#define STDIN_FILENO    0       /* Standard input.  */
#define STDOUT_FILENO   1       /* Standard output.  */
#define STDERR_FILENO   2       /* Standard error output.  */

#define _exit( r )      TerminateProcess( GetCurrentProcess(), (r) )

#else

#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if defined( __linux )
#include <linux/unistd.h>
#if ! defined( __ANDROID__ )
#include <bits/sockaddr.h>
#endif
#endif

#include <sys/un.h>
#include <dlfcn.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <strings.h>
#include <sys/wait.h>
#include <dirent.h>

#endif

#if defined( __APPLE__ )
#include <sys/un.h>
#include <TargetConditionals.h>
#ifndef CHAR_MAX
#define CHAR_MAX    INT8_MAX
#endif
#endif
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <limits.h>

#if ( defined(__APPLE__) && ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR ) )
#define _LIB
#endif

#define OFFSET_RECORD(address, type, field) ((type *)(  \
    (char*)(address) -    \
    (char*)(&((type *)0)->field)))

#define ALIGN_UP( number, align_size )   \
    (((number) + (align_size) - 1) & ~((align_size) - 1))

#ifdef __cplusplus
#include <algorithm>

#if defined( WIN32 ) || defined( WIN64 )

#undef max
#undef min
#endif // #if defined( WIN32 ) || defined( WIN64 )
#endif // #ifdef __cplusplus

#if defined( _DEBUG )
#if ! defined( DEBUG )
#define DEBUG   1
#endif
#if defined( NDEBUG )
#undef NDEBUG
#endif
#elif defined( DEBUG )
#if ! defined( _DEBUG )
#define _DEBUG  1
#endif
#if defined( NDEBUG )
#undef NDEBUG
#endif
#else
#if ! defined( NDEBUG )
#define NDEBUG  1
#endif
#if defined( _DEBUG )
#undef _DEBUG
#endif
#if defined( DEBUG )
#undef DEBUG
#endif
#endif

#if !defined(IS_LITTLE_ENDIAN)
#if defined(__GNUC__) || defined(__clang__)
#ifdef __BIG_ENDIAN__
#define IS_LITTLE_ENDIAN 0
#else
#define IS_LITTLE_ENDIAN 1
#endif // __BIG_ENDIAN__
#elif defined(_MSC_VER)
#if defined(_M_PPC)
#define IS_LITTLE_ENDIAN 0
#else
#define IS_LITTLE_ENDIAN 1
#endif
#else
#error Unable to determine endianness, define IS_LITTLE_ENDIAN.
#endif
#endif

#if defined( WIN64 ) || defined( WIN32 ) || defined(__i386) || defined(_M_IX86) || defined (__x86_64)
#define S_LITTLE_ENDIAN     1
#define S_BIG_ENDIAN        0
#elif defined(__sparc) || defined(__sparc__) || defined(__hppa) || defined(__ppc__) || defined(_ARCH_COM)
#define S_BIG_ENDIAN        1
#define S_LITTLE_ENDIAN     0
#elif defined(_WIN32_WCE)
#define S_LITTLE_ENDIAN     1
#define S_BIG_ENDIAN        0
#elif defined( __APPLE__ )
#if ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
// IOS
#define S_LITTLE_ENDIAN     1
#define S_BIG_ENDIAN        0
#else
// Mac OS X
#define S_LITTLE_ENDIAN     1
#define S_BIG_ENDIAN        0
#endif
#elif defined( __ANDROID__ )
#define S_LITTLE_ENDIAN     1
#define S_BIG_ENDIAN        0
#elif defined( _OPENWRT )
#define S_LITTLE_ENDIAN     1
#define S_BIG_ENDIAN        0
#else
#error Unknown architecture
#endif

#if defined( WIN32 ) || defined( WIN64 )
#define atoi64(x)       _atoi64((x))
#ifndef stricmp
#define stricmp     _stricmp
#endif
#ifndef strnicmp
#define strnicmp    _strnicmp
#endif
#ifndef snprintf
#define snprintf    _snprintf
#endif
#define lstrnicmp       _strnicmp
#define S_CRLF          "\r\n"
#define S_PATH_SEP_C    '\\'
#define S_PATH_SEP      "\\"
#define localtime_r( NOW, RET )     ((0 == localtime_s( (RET), (NOW) )) ? (RET) : NULL)

#define likely(x)   (x)
#define unlikely(x) (x)

#else
#define atoi64(x)       atoll((x))
#ifndef stricmp
#define stricmp     strcasecmp
#endif
#define strnicmp        strncasecmp
#define MAX_PATH        520
#define S_CRLF          "\n"
#define S_PATH_SEP_C    '/'
#define S_PATH_SEP      "/"

#define likely(x)   __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)

#endif

#define COUNT_OF(a) (sizeof(a)/sizeof((a)[0]))

#ifdef _MSC_VER
#ifndef I64D
#define I64D "%I64d"
#endif
#ifndef I64U
#define I64U "%I64u"
#endif
#else
#ifndef I64D
#define I64D "%lld"
#endif
#ifndef I64U
#define I64U "%llu"
#endif
#endif

#if ! defined( __cplusplus )
#if defined(_MSC_VER)
#define inline  __inline
#elif defined(DIAB_COMPILER)
#define inline
#else
#define inline
#endif
#endif

// S_EXP
// S_IMP
#if ( defined(_MSC_VER) || defined(__CYGWIN__) || (defined(__HP_aCC) && defined(__HP_WINDLL) ) )
#define S_EXP               __declspec(dllexport)
#define S_IMP               __declspec(dllimport)
#elif defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550)
#define S_EXP               __global
#define S_IMP
#else
#define S_EXP
#define S_IMP
#endif

// MSG_NO_SIGNAL
#ifndef MSG_NOSIGNAL
#define	MSG_NOSIGNAL		0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if ( TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
    struct objc_object;
    typedef struct objc_object * id;
#endif

#ifndef __cplusplus

#ifndef bool
#define	bool    unsigned char
#endif
#define	true        1
#define	false       0
#endif

#if defined( WIN32 ) || defined( WIN64 )

    typedef int pid_t;

    static inline pid_t getpid ( )
    {
        return ( pid_t ) GetProcessId ( GetCurrentProcess ( ) );
    }

    static inline int gettid ( )
    {
        return ( int ) GetCurrentThreadId ( );
    }

    static inline int getppid ( )
    {
        DWORD pid = GetProcessId ( GetCurrentProcess ( ) );
        DWORD ppid = -1;
        PROCESSENTRY32 pe;
        HANDLE h;
        h = CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS, 0 );
        memset ( & pe, 0, sizeof ( PROCESSENTRY32 ) );
        pe.dwSize = sizeof ( PROCESSENTRY32 );
        if ( Process32First ( h, & pe ) )
        {
            do
            {
                if ( pe.th32ProcessID == pid )
                {
                    ppid = pe.th32ParentProcessID;
                    break;
                }
            }
            while ( Process32Next ( h, & pe ) );
        }
        CloseHandle ( h );
        return ( int ) ppid;
    }
#else
    typedef int SOCKET;
    typedef int BOOL;

#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR    (int)-1
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (int)-1
#endif

#if defined( __ANDROID__ )

    // android has gettid function

#elif defined( __linux )

    static inline int gettid ( )
    {
        return syscall ( __NR_gettid );
    }

#elif defined( __APPLE__ )

    typedef __darwin_pid_t pid_t;

#define gettid()  pthread_mach_thread_np( pthread_self() )

#elif defined( __FreeBSD__ )

    static inline int gettid ( )
    {
        void * p = pthread_self ( );
        long r;
        memcpy ( & r, & p, sizeof ( long ) );
        return ( int ) r;
    }

#endif

#endif

#if defined( WIN32 ) || defined(WIN64) || defined(__osf__)
    typedef int socklen_t;
#endif

#if ! defined( WIN32 ) && ! defined( WIN64 )
    typedef char TCHAR;
#endif

    typedef unsigned char byte_t;

    typedef short int16_t;
    typedef unsigned short uint16_t;

#if defined(__APPLE__) || defined(__FreeBSD__)

#define S_64    1

#elif ( defined( WIN64 ) )

    typedef int int32_t;
    typedef unsigned int uint32_t;

    typedef unsigned long long uint64_t;
    typedef long long int64_t;

#define S_64    1

#elif ( (defined(__sun) && defined(__sparcv9)) || (defined(__linux) && defined(__x86_64)) || (defined(__hppa) && defined(__LP64__)) || (defined(_ARCH_COM) && defined(__64BIT__)) )

    typedef int int32_t;
    typedef unsigned int uint32_t;

#define S_64    1

#else

    // 32 bit

#ifndef __linux__
    typedef int int32_t;
    typedef unsigned int uint32_t;
#endif

    typedef unsigned long long uint64_t;
    typedef long long int64_t;

#define S_64    0

#endif

#if ! defined( WIN32 ) && ! defined( WIN32 )

    typedef struct UUID
    {
        unsigned char timeLow[4];
        unsigned char timeMid[2];
        unsigned char timeHighAndVersion[2];
        unsigned char clockSeqHiAndReserved;
        unsigned char clockSeqLow;
        unsigned char node[6];

    } UUID;

#endif

#if ! defined( WIN32 ) && ! defined( WIN64 )
#if ( 0 == S_64 )
#define __stdcall   __attribute__((__stdcall__))
#define __cdecl     __attribute__((__cdecl__))
#else
#define __stdcall
#define __cdecl
#endif
#endif

#if defined( _MSC_VER )
#define INT64( n ) n##i64
#elif defined(__HP_aCC)
#define INT64(n)    n
#elif S_64
#define INT64(n)    n##L
#else
#define INT64(n)    n##LL
#endif

#if ! defined( WIN32 ) && ! defined( WIN64 )
#define UINT32  uint32_t
#define UINT16  uint16_t
#define LONG    long
#endif

#ifdef __cplusplus
}
#endif

#endif // #ifndef _hustdb_std_include_h_
