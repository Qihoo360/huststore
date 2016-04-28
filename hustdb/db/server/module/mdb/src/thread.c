/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Thread management for memcached.
 */
#include "../lib/memcached.h"
#include "../lib/hash.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#ifdef __sun
#include <atomic.h>
#endif

#define ITEMS_PER_ALLOC 64

/* Locks for cache LRU operations */
pthread_mutex_t lru_locks[POWER_LARGEST];

#if !defined(HAVE_GCC_ATOMICS) && !defined(__sun)
pthread_mutex_t atomics_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Lock for global stats */
static pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t *item_locks;
/* size of the item lock hash table */
static uint32_t item_lock_count;
unsigned int item_lock_hashpower;
#define hashsize(n) ((unsigned long int)1<<(n))
#define hashmask(n) (hashsize(n)-1)

unsigned short refcount_incr ( unsigned short *refcount )
{
#ifdef HAVE_GCC_ATOMICS
    return __sync_add_and_fetch (refcount, 1);
#elif defined(__sun)
    return atomic_inc_ushort_nv (refcount);
#else
    unsigned short res;
    mutex_lock (&atomics_mutex);
    ( *refcount ) ++;
    res = * refcount;
    mutex_unlock (&atomics_mutex);
    return res;
#endif
}

unsigned short refcount_decr ( unsigned short *refcount )
{
#ifdef HAVE_GCC_ATOMICS
    return __sync_sub_and_fetch (refcount, 1);
#elif defined(__sun)
    return atomic_dec_ushort_nv (refcount);
#else
    unsigned short res;
    mutex_lock (&atomics_mutex);
    ( *refcount ) --;
    res = * refcount;
    mutex_unlock (&atomics_mutex);
    return res;
#endif
}

/* item_lock() must be held for an item before any modifications to either its
 * associated hash bucket, or the structure itself.
 * LRU modifications must hold the item lock, and the LRU lock.
 * LRU's accessing items must item_trylock() before modifying an item.
 * Items accessable from an LRU must not be freed or modified
 * without first locking and removing from the LRU.
 */

void item_lock ( uint32_t hv )
{
    mutex_lock (&item_locks[hv & hashmask (item_lock_hashpower)]);
}

void *item_trylock ( uint32_t hv )
{
    pthread_mutex_t *lock = & item_locks[hv & hashmask (item_lock_hashpower)];
    if ( pthread_mutex_trylock (lock) == 0 )
    {
        return lock;
    }
    return NULL;
}

void item_trylock_unlock ( void *lock )
{
    mutex_unlock (( pthread_mutex_t * ) lock);
}

void item_unlock ( uint32_t hv )
{
    mutex_unlock (&item_locks[hv & hashmask (item_lock_hashpower)]);
}

/* Must not be called with any deeper locks held */
void pause_threads ( enum pause_thread_types type )
{
    switch ( type )
    {
        case PAUSE_ALL_THREADS:
            slabs_rebalancer_pause ();
            lru_crawler_pause ();
            lru_maintainer_pause ();
            //case PAUSE_WORKER_THREADS:
            //    buf[0] = 'p';
            //    pthread_mutex_lock(&worker_hang_lock);
            break;
        case RESUME_ALL_THREADS:
            slabs_rebalancer_resume ();
            lru_crawler_resume ();
            lru_maintainer_resume ();
            //case RESUME_WORKER_THREADS:
            //   pthread_mutex_unlock(&worker_hang_lock);
            break;
        default:
            fprintf (stderr, "Unknown lock type: %d\n", type);
            assert (1 == 0);
            break;
    }

    return;
}

/********************************* ITEM ACCESS *******************************/

/*
 * Allocates a new item.
 */
item *item_alloc ( char *key, size_t nkey, int flags, rel_time_t exptime, char *value, int nbytes )
{
    item *it;
    /* do_item_alloc handles its own locks */
    it = do_item_alloc (key, nkey, flags, exptime, value, nbytes, 0);
    return it;
}

/*
 * Returns an item if it hasn't been marked as expired,
 * lazy-expiring as needed.
 */
item *item_get ( const char *key, const size_t nkey )
{
    item *it;
    uint32_t hv;
    hv = hash (key, nkey);
    item_lock (hv);
    it = do_item_get (key, nkey, hv);
    item_unlock (hv);
    return it;
}

item *item_touch ( const char *key, size_t nkey, uint32_t exptime )
{
    item *it;
    uint32_t hv;
    hv = hash (key, nkey);
    item_lock (hv);
    it = do_item_touch (key, nkey, exptime, hv);
    item_unlock (hv);
    return it;
}

/*
 * Links an item into the LRU and hashtable.
 */
int item_link ( item *item )
{
    int ret;
    uint32_t hv;

    hv = hash (ITEM_key (item), item->nkey);
    item_lock (hv);
    ret = do_item_link (item, hv);
    item_unlock (hv);
    return ret;
}

/*
 * Decrements the reference count on an item and adds it to the freelist if
 * needed.
 */
void item_remove ( item *item )
{
    uint32_t hv;
    hv = hash (ITEM_key (item), item->nkey);

    item_lock (hv);
    do_item_remove (item);
    item_unlock (hv);
}

/*
 * Replaces one item with another in the hashtable.
 * Unprotected by a mutex lock since the core server does not require
 * it to be thread-safe.
 */
int item_replace ( item *old_it, item *new_it, const uint32_t hv )
{
    return do_item_replace (old_it, new_it, hv);
}

/*
 * Unlinks an item from the LRU and hashtable.
 */
void item_unlink ( item *item )
{
    uint32_t hv;
    hv = hash (ITEM_key (item), item->nkey);
    item_lock (hv);
    do_item_unlink (item, hv);
    item_unlock (hv);
}

/*
 * Moves an item to the back of the LRU queue.
 */
void item_update ( item *item )
{
    uint32_t hv;
    hv = hash (ITEM_key (item), item->nkey);

    item_lock (hv);
    do_item_update (item);
    item_unlock (hv);
}

/*
 * Stores an item in the cache (high level, obeys set/add/replace semantics)
 */
enum store_item_type store_item ( item *item, int comm )
{
    enum store_item_type ret;
    uint32_t hv;

    hv = hash (ITEM_key (item), item->nkey);
    item_lock (hv);
    ret = do_store_item (item, comm, hv);
    item_unlock (hv);
    return ret;
}

/******************************* GLOBAL STATS ******************************/

void STATS_LOCK ( )
{
    pthread_mutex_lock (&stats_lock);
}

void STATS_UNLOCK ( )
{
    pthread_mutex_unlock (&stats_lock);
}

/*
 * Initializes the thread subsystem, creating various worker threads.
 *
 * nthreads  Number of worker event handler threads to spawn
 * main_base Event base for main thread
 */
void memcached_thread_init ( )
{
    int i;
    int power;
    int nthreads = 2;

    for ( i = 0; i < POWER_LARGEST; i ++ )
    {
        pthread_mutex_init (&lru_locks[i], NULL);
    }

    /* Want a wide lock table, but don't waste memory */
    if ( nthreads < 3 )
    {
        power = 10;
    }
    else if ( nthreads < 4 )
    {
        power = 11;
    }
    else if ( nthreads < 5 )
    {
        power = 12;
    }
    else
    {
        /* 8192 buckets, and central locks don't scale much past 5 threads */
        power = 13;
    }

    if ( power >= hashpower )
    {
        fprintf (stderr, "Hash table power size (%d) cannot be equal to or less than item lock table (%d)\n", hashpower, power);
        fprintf (stderr, "Item lock table grows with `-t N` (worker threadcount)\n");
        fprintf (stderr, "Hash table grows with `-o hashpower=N` \n");
        exit (1);
    }

    item_lock_count = hashsize (power);
    item_lock_hashpower = power;

    item_locks = calloc (item_lock_count, sizeof (pthread_mutex_t ));
    if ( ! item_locks )
    {
        perror ("Can't allocate item locks");
        exit (1);
    }
    for ( i = 0; i < item_lock_count; i ++ )
    {
        pthread_mutex_init (&item_locks[i], NULL);
    }
}

