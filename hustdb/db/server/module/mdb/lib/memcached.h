/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/** \file
 * The main memcached header holding commonly used data
 * structures and function prototypes.
 */
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/timerfd.h>

#define ENDIAN_LITTLE 1

    /** Maximum length of a key. */
#define KEY_MAX_LENGTH 250

    /** Size of an incr buf. */
#define INCR_MAX_STORAGE_LEN 24

#define DATA_BUFFER_SIZE 2048
#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
    /* I'm told the max length of a 64-bit num converted to string is 20 bytes.
     * Plus a few for spaces, \r\n, \0 */
#define SUFFIX_SIZE 24

    /** Initial size of list of items being returned by "get". */
#define ITEM_LIST_INITIAL 200

    /** Initial size of list of CAS suffixes appended to "gets" lines. */
#define SUFFIX_LIST_INITIAL 20


    /** High water marks for buffer shrinking */
#define READ_BUFFER_HIGHWAT 8192
#define ITEM_LIST_HIGHWAT 400


    /* Initial power multiplier for the hash table */
#define HASHPOWER_DEFAULT 16

    /*
     * We only reposition items in the LRU queue if they haven't been repositioned
     * in this many seconds. That saves us from churning on frequently-accessed
     * items.
     */
#define ITEM_UPDATE_INTERVAL 60

    /* unistd.h is here */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

    /* Slab sizing definitions. */
#define POWER_SMALLEST 1
#define POWER_LARGEST  256 /* actual cap is 255 */
#define CHUNK_ALIGN_BYTES 8
    /* slab class max is a 6-bit number, -1. */
#define MAX_NUMBER_OF_SLAB_CLASSES (63 + 1)

    /** How long an object can reasonably be assumed to be locked before
        harvesting it on a low memory condition. Default: disabled. */
#define TAIL_REPAIR_TIME_DEFAULT 0

    /* warning: don't use these macros with a function, as it evals its arg twice */
#define ITEM_get_cas(i) (((i)->it_flags & ITEM_CAS) ? \
        (i)->data->cas : (uint64_t)0)

#define ITEM_set_cas(i,v) { \
    if ((i)->it_flags & ITEM_CAS) { \
        (i)->data->cas = v; \
    } \
}

#define ITEM_key(item) (((char*)&((item)->data)) \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_suffix(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_data(item) ((char*) &((item)->data) + (item)->nkey + 1 \
         + (item)->nsuffix \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_ntotal(item) (sizeof(struct _stritem) + (item)->nkey + 1 \
         + (item)->nsuffix + (item)->nbytes \
         + (((item)->it_flags & ITEM_CAS) ? sizeof(uint64_t) : 0))

#define ITEM_clsid(item) ((item)->slabs_clsid & ~(3<<6))

#define STAT_KEY_LEN 128
#define STAT_VAL_LEN 128

    /** Append a simple stat with a stat name, value format and value */
#define APPEND_STAT(name, fmt, val) \
    append_stat(name, add_stats, c, fmt, val);

    /** Append an indexed stat with a stat name (with format), value format
        and value */
#define APPEND_NUM_FMT_STAT(name_fmt, num, name, fmt, val)          \
    klen = snprintf(key_str, STAT_KEY_LEN, name_fmt, num, name);    \
    vlen = snprintf(val_str, STAT_VAL_LEN, fmt, val);               \
    add_stats(key_str, klen, val_str, vlen, c);

    /** Common APPEND_NUM_FMT_STAT format. */
#define APPEND_NUM_STAT(num, name, fmt, val) \
    APPEND_NUM_FMT_STAT("%d:%s", num, name, fmt, val)

#define MEMCACHED_ASSOC_DELETE(arg0, arg1, arg2)
#define MEMCACHED_ASSOC_DELETE_ENABLED() (0)
#define MEMCACHED_ASSOC_FIND(arg0, arg1, arg2)
#define MEMCACHED_ASSOC_FIND_ENABLED() (0)
#define MEMCACHED_ASSOC_INSERT(arg0, arg1, arg2)
#define MEMCACHED_ASSOC_INSERT_ENABLED() (0)
#define MEMCACHED_COMMAND_ADD(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_ADD_ENABLED() (0)
#define MEMCACHED_COMMAND_APPEND(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_APPEND_ENABLED() (0)
#define MEMCACHED_COMMAND_CAS(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_CAS_ENABLED() (0)
#define MEMCACHED_COMMAND_DECR(arg0, arg1, arg2, arg3)
#define MEMCACHED_COMMAND_DECR_ENABLED() (0)
#define MEMCACHED_COMMAND_DELETE(arg0, arg1, arg2)
#define MEMCACHED_COMMAND_DELETE_ENABLED() (0)
#define MEMCACHED_COMMAND_GET(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_GET_ENABLED() (0)
#define MEMCACHED_COMMAND_TOUCH(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_TOUCH_ENABLED() (0)
#define MEMCACHED_COMMAND_INCR(arg0, arg1, arg2, arg3)
#define MEMCACHED_COMMAND_INCR_ENABLED() (0)
#define MEMCACHED_COMMAND_PREPEND(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_PREPEND_ENABLED() (0)
#define MEMCACHED_COMMAND_REPLACE(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_REPLACE_ENABLED() (0)
#define MEMCACHED_COMMAND_SET(arg0, arg1, arg2, arg3, arg4)
#define MEMCACHED_COMMAND_SET_ENABLED() (0)
#define MEMCACHED_CONN_ALLOCATE(arg0)
#define MEMCACHED_CONN_ALLOCATE_ENABLED() (0)
#define MEMCACHED_CONN_CREATE(arg0)
#define MEMCACHED_CONN_CREATE_ENABLED() (0)
#define MEMCACHED_CONN_DESTROY(arg0)
#define MEMCACHED_CONN_DESTROY_ENABLED() (0)
#define MEMCACHED_CONN_DISPATCH(arg0, arg1)
#define MEMCACHED_CONN_DISPATCH_ENABLED() (0)
#define MEMCACHED_CONN_RELEASE(arg0)
#define MEMCACHED_CONN_RELEASE_ENABLED() (0)
#define MEMCACHED_ITEM_LINK(arg0, arg1, arg2)
#define MEMCACHED_ITEM_LINK_ENABLED() (0)
#define MEMCACHED_ITEM_REMOVE(arg0, arg1, arg2)
#define MEMCACHED_ITEM_REMOVE_ENABLED() (0)
#define MEMCACHED_ITEM_REPLACE(arg0, arg1, arg2, arg3, arg4, arg5)
#define MEMCACHED_ITEM_REPLACE_ENABLED() (0)
#define MEMCACHED_ITEM_UNLINK(arg0, arg1, arg2)
#define MEMCACHED_ITEM_UNLINK_ENABLED() (0)
#define MEMCACHED_ITEM_UPDATE(arg0, arg1, arg2)
#define MEMCACHED_ITEM_UPDATE_ENABLED() (0)
#define MEMCACHED_PROCESS_COMMAND_END(arg0, arg1, arg2)
#define MEMCACHED_PROCESS_COMMAND_END_ENABLED() (0)
#define MEMCACHED_PROCESS_COMMAND_START(arg0, arg1, arg2)
#define MEMCACHED_PROCESS_COMMAND_START_ENABLED() (0)
#define MEMCACHED_SLABS_ALLOCATE(arg0, arg1, arg2, arg3)
#define MEMCACHED_SLABS_ALLOCATE_ENABLED() (0)
#define MEMCACHED_SLABS_ALLOCATE_FAILED(arg0, arg1)
#define MEMCACHED_SLABS_ALLOCATE_FAILED_ENABLED() (0)
#define MEMCACHED_SLABS_FREE(arg0, arg1, arg2)
#define MEMCACHED_SLABS_FREE_ENABLED() (0)
#define MEMCACHED_SLABS_SLABCLASS_ALLOCATE(arg0)
#define MEMCACHED_SLABS_SLABCLASS_ALLOCATE_ENABLED() (0)
#define MEMCACHED_SLABS_SLABCLASS_ALLOCATE_FAILED(arg0)
#define MEMCACHED_SLABS_SLABCLASS_ALLOCATE_FAILED_ENABLED() (0)



    /**
     * Callback for any function producing stats.
     *
     * @param key the stat's key
     * @param klen length of the key
     * @param val the stat's value in an ascii form (e.g. text form of a number)
     * @param vlen length of the value
     * @parm cookie magic callback cookie
     */
    typedef void (*ADD_STAT )( const char *key, const uint16_t klen,
                               const char *val, const uint32_t vlen,
                               const void *cookie );

    /*
     * NOTE: If you modify this table you _MUST_ update the function state_text
     */

    enum protocol
    {
        ascii_prot = 3, /* arbitrary value. */
        binary_prot,
        negotiating_prot /* Discovering the protocol */
    };

    enum pause_thread_types
    {
        PAUSE_WORKER_THREADS = 0,
        PAUSE_ALL_THREADS,
        RESUME_ALL_THREADS,
        RESUME_WORKER_THREADS
    };


#define NREAD_ADD 1
#define NREAD_SET 2
#define NREAD_REPLACE 3
#define NREAD_APPEND 4
#define NREAD_PREPEND 5
#define NREAD_CAS 6

    enum store_item_type
    {
        NOT_STORED = 0, STORED, EXISTS, NOT_FOUND
    };

    enum delta_result_type
    {
        OK, NON_NUMERIC, EOM, DELTA_ITEM_NOT_FOUND, DELTA_ITEM_CAS_MISMATCH
    };

    /** Time relative to server start. Smaller than time_t on 64-bit systems. */
    typedef unsigned int rel_time_t;

#define MAX_VERBOSITY_LEVEL 2

    /* When adding a setting, be sure to update process_stat_settings */

    /**
     * Globally accessible settings as derived from the commandline.
     */
    struct settings
    {
        size_t maxbytes;
        char *inter;
        int verbose;
        rel_time_t oldest_live; /* ignore existing items older than this */
        uint64_t oldest_cas; /* ignore existing items with CAS values lower than this */
        int evict_to_free;
        double factor; /* chunk size growth factor */
        int chunk_size;
        char prefix_delimiter; /* character that marks a key prefix (for stats) */
        int detail_enabled; /* nonzero if we're collecting detailed stats */
        bool use_cas;
        enum protocol binding_protocol;
        int backlog;
        int item_size_max; /* Maximum item size, and upper end for slabs */
        bool lru_crawler; /* Whether or not to enable the autocrawler thread */
        bool lru_maintainer_thread; /* LRU maintainer background thread */
        bool slab_reassign; /* Whether or not slab reassignment is allowed */
        int slab_automove; /* Whether or not to automatically move slabs */
        int hashpower_init; /* Starting hash power level */
        bool shutdown_command; /* allow shutdown command */
        int tail_repair_time; /* LRU tail refcount leak repair time */
        bool flush_enabled; /* flush_all enabled */
        char *hash_algorithm; /* Hash algorithm in use */
        int lru_crawler_sleep; /* Microsecond sleep between items */
        uint32_t lru_crawler_tocrawl; /* Number of items to crawl per run */
        int hot_lru_pct; /* percentage of slab space for HOT_LRU */
        int warm_lru_pct; /* percentage of slab space for WARM_LRU */
        int crawls_persleep; /* Number of LRU crawls to run before sleeping */
        bool expirezero_does_not_evict; /* exptime == 0 goes into NOEXP_LRU */
    };

    extern time_t process_started;
    extern struct settings settings;

#define ITEM_LINKED 1
#define ITEM_CAS 2

    /* temp */
#define ITEM_SLABBED 4

    /* Item was fetched at least once in its lifetime */
#define ITEM_FETCHED 8
    /* Appended on fetch, removed on LRU shuffling */
#define ITEM_ACTIVE 16

    /**
     * Structure for storing items within memcached.
     */
    typedef struct _stritem
    {
        /* Protected by LRU locks */
        struct _stritem *next;
        struct _stritem *prev;
        /* Rest are protected by an item lock */
        struct _stritem *h_next; /* hash chain next */
        rel_time_t time; /* least recent access */
        rel_time_t exptime; /* expire time */
        int nbytes; /* size of data */
        unsigned short refcount;
        uint8_t nsuffix; /* length of flags-and-length string */
        uint8_t it_flags; /* ITEM_* above */
        uint8_t slabs_clsid; /* which slab class we're in */
        uint8_t nkey; /* key length, w/terminating null and padding */

        /* this odd type prevents type-punning issues when we do
         * the little shuffle to save space when not using CAS. */
        union
        {
            uint64_t cas;
            char end;
        } data[];
        /* if it_flags & ITEM_CAS we have 8 bytes CAS */
        /* then null-terminated key */
        /* then " flags length\r\n" (no terminating null) */
        /* then data with terminating \r\n (no terminating null; it's binary!) */
    } item;

    typedef struct
    {
        struct _stritem *next;
        struct _stritem *prev;
        struct _stritem *h_next; /* hash chain next */
        rel_time_t time; /* least recent access */
        rel_time_t exptime; /* expire time */
        int nbytes; /* size of data */
        unsigned short refcount;
        uint8_t nsuffix; /* length of flags-and-length string */
        uint8_t it_flags; /* ITEM_* above */
        uint8_t slabs_clsid; /* which slab class we're in */
        uint8_t nkey; /* key length, w/terminating null and padding */
        uint32_t remaining; /* Max keys to crawl per slab per invocation */
    } crawler;


    /* current time of day (updated periodically) */
    extern volatile rel_time_t current_time;

    /* TODO: Move to slabs.h? */
    extern volatile int slab_rebalance_signal;

    struct slab_rebalance
    {
        void *slab_start;
        void *slab_end;
        void *slab_pos;
        int s_clsid;
        int d_clsid;
        int busy_items;
        uint8_t done;
    };

    extern struct slab_rebalance slab_rebal;

    /*
     * Functions
     */
    enum store_item_type do_store_item ( item *item, int comm, const uint32_t hv );

#define mutex_lock(x) pthread_mutex_lock(x)
#define mutex_unlock(x) pthread_mutex_unlock(x)

#include "slabs.h"
#include "assoc.h"
#include "items.h"
#include "util.h"

    /*
     * Functions such as the libevent-related calls that need to do cross-thread
     * communication in multithreaded mode (rather than actually doing the work
     * in the current thread) are called via "dispatch_" frontends, which are
     * also #define-d to directly call the underlying code in singlethreaded mode.
     */

    void memcached_thread_init ( );

    /* Lock wrappers for cache functions that are called from main loop. */

    item *item_alloc ( char *key, size_t nkey, int flags, rel_time_t exptime, char *value, int nbytes );
    item *item_get ( const char *key, const size_t nkey );
    item *item_touch ( const char *key, const size_t nkey, uint32_t exptime );
    int item_link ( item *it );
    void item_remove ( item *it );
    int item_replace ( item *it, item *new_it, const uint32_t hv );
    void item_unlink ( item *it );
    void item_update ( item *it );

    void item_lock ( uint32_t hv );
    void *item_trylock ( uint32_t hv );
    void item_trylock_unlock ( void *arg );
    void item_unlock ( uint32_t hv );
    void pause_threads ( enum pause_thread_types type );
    unsigned short refcount_incr ( unsigned short *refcount );
    unsigned short refcount_decr ( unsigned short *refcount );
    void STATS_LOCK ( void );
    void STATS_UNLOCK ( void );

    enum store_item_type store_item ( item *item, int comm );

#if HAVE_DROP_PRIVILEGES
    extern void drop_privileges ( void );
#else
#define drop_privileges()
#endif

    /* If supported, give compiler hints for branch prediction. */
#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)
#define __builtin_expect(x, expected_value) (x)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

    int process_delete_command ( char *key, size_t nkey );
    int process_touch_command ( char *key, size_t nkey, int32_t exptime_int );
    int process_get_command ( char *key, size_t nkey, char *dst, int *len );
    int process_update_command ( char *key, size_t nkey, uint32_t flags, char *value, size_t nbytes, int32_t exptime_int, int comm );
    int init ( unsigned int size );
    unsigned int get_maxbytes ( );
    uint32_t mdb_timestamp ( );
    bool is_memory_over_threshold ( );
    void set_mem_threshold ( int sys_mem_threshold, int proc_mem_threshold );

#ifdef __cplusplus
}
#endif

