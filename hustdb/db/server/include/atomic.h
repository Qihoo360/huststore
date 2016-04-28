#ifndef _base_atomic_h_
#define _base_atomic_h_

#if defined( WIN32 ) || defined( WIN64 )
#include <winsock2.h>
#include <windows.h>
#elif defined( __APPLE__ )
#ifdef __header_always_inline
#undef __header_always_inline
#endif
#define __header_always_inline  static inline
#include <libkern/OSAtomic.h>
#elif defined( __FreeBSD__ )
#include <sys/types.h>
#include <machine/atomic.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined( __linux )
    typedef volatile int atomic_t;
#elif defined( __APPLE__ )
    typedef volatile int32_t atomic_t;
#elif defined( __FreeBSD__ )
    typedef volatile u_long atomic_t;
#elif defined( WIN32 ) || defined( WIN64 )
    typedef long volatile atomic_t;
#else
#error unknown platform
#endif
#ifndef ATOMIC_T_LEN
#define ATOMIC_T_LEN                sizeof( atomic_t )
#endif

#if defined( __linux )
#if defined( __x86_64 )
    // 64 bit X86 CPU

    static inline atomic_t atomic_fetch_add ( int v, atomic_t * dst )
    {
        __asm__ __volatile__ (
                               " lock ; xaddl %0, %1; "
                               : "+r" ( v ) : "m" ( *dst ) : "cc", "memory"
                               );
        return v;
    }
#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 1) || (__GNUC__ == 4 && __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ >= 2)
    // GCC including __sync_fetch_and_add function since 4.1.2

    static inline atomic_t atomic_fetch_add ( int v, atomic_t * dst )
    {
        return __sync_fetch_and_add ( dst, v );
    }
#else
    // 32 bit X86 CPU

    static inline atomic_t atomic_fetch_add ( int v, atomic_t * dst )
    {
        asm volatile(
                      "movl        %0,%%eax;"
                      "movl        %1,%%ecx;"
                      "lock xadd   %%eax,(%%ecx);"
                      ::"m"( v ), "m"( dst )
                      );
        return v;
    }
#endif

#elif defined( __APPLE__ )

    static inline atomic_t atomic_fetch_add ( int v, atomic_t * dst )
    {
        return OSAtomicAdd32 ( ( int32_t ) ( v ), ( atomic_t * ) ( dst ) ) - v;
    }

#elif defined( __FreeBSD__ )

    static inline u_int atomic_fetchsub_int ( atomic_t * dst, u_int v )
    {
        __asm __volatile (
                           "   lock ; "
                           "   xsubl %0, %1 ; "
                           "# atomic_fetchsub_int"
                           : "+r" ( v ),
                           "=m" ( *dst )
                           : "m" ( *dst ) );
        return v;
    }

    static inline atomic_t atomic_fetch_add ( int v, atomic_t * dst )
    {
        return ( v >= 0 )
            ? ( atomic_t ) atomic_fetchadd_int ( dst, ( u_int ) ( v ) )
            : ( atomic_t ) atomic_fetchsub_int ( dst, ( u_int ) ( v ) );
    }

#elif defined( WIN32 ) || defined( WIN64 )

    static __inline atomic_t atomic_fetch_add ( int v, atomic_t * dst )
    {
        return InterlockedExchangeAdd ( ( long volatile * )( dst ), ( long ) ( v ) );
    }

#else
#error unknown platform
#endif

#ifdef __cplusplus
}
#endif

#endif // ! #ifndef _base_atomic_h_
