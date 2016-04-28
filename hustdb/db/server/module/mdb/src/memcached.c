/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  memcached - memory caching daemon
 *
 *       http://www.memcached.org/
 *
 *  Copyright 2003 Danga Interactive, Inc.  All rights reserved.
 *
 *  Use and distribution licensed under the BSD license.  See
 *  the LICENSE file for full text.
 *
 *  Authors:
 *      Anatoly Vorobey <mellon@pobox.com>
 *      Brad Fitzpatrick <brad@danga.com>
 */
#include "../lib/memcached.h"
#include "../lib/hash.h"
#include <sys/stat.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <ctype.h>
#include <stdarg.h>

/* some POSIX systems need the following definition
 * to get mlockall flags out of sys/mman.h.  */
#ifndef _P1003_1B_VISIBLE
#define _P1003_1B_VISIBLE
#endif
/* need this to get IOV_MAX on some platforms. */
#ifndef __need_IOV_MAX
#define __need_IOV_MAX
#endif
#include <pwd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <sysexits.h>
#include <stddef.h>

/* FreeBSD 4.x doesn't have IOV_MAX exposed. */
#ifndef IOV_MAX
#if defined(__FreeBSD__) || defined(__APPLE__)
#define IOV_MAX 1024
#endif
#endif

/*
 * forward declarations
 */

/* defaults */
static void settings_init ( unsigned int );

/** exported globals **/
struct settings settings;
time_t process_started; /* when the process was started */

struct slab_rebalance slab_rebal;
volatile int slab_rebalance_signal;


#define REALTIME_MAXDELTA 60*60*24*30

/*
 * given time value that's either unix time or delta from current unix time, return
 * unix time. Use the fact that delta can't exceed one month (and real time value can't
 * be that low).
 */
static rel_time_t realtime ( const time_t exptime )
{
    /* no. of seconds in 30 days - largest possible delta exptime */

    if ( exptime == 0 ) return 0; /* 0 means never expire */

    if ( exptime > REALTIME_MAXDELTA )
    {
        /* if item expiration is at/before the server started, give it an
           expiration time of 1 second after the server started.
           (because 0 means don't expire).  without this, we'd
           underflow and wrap around to some large value way in the
           future, effectively making items expiring in the past
           really expiring never */
        if ( exptime <= process_started )
            return ( rel_time_t ) 1;
        return ( rel_time_t ) ( exptime - process_started );
    }
    else
    {
        return ( rel_time_t ) ( exptime + current_time );
    }
}

static void settings_init ( unsigned int size )
{
    settings.use_cas = false;
    /* By default this string should be NULL for getaddrinfo() */
    settings.maxbytes = size * 1024 * 1024; /* default is 64MB */
    settings.verbose = 0;
    settings.oldest_live = 0;
    settings.oldest_cas = 0; /* supplements accuracy of oldest_live */
    settings.evict_to_free = 1; /* push old items out of cache when memory runs out */
    settings.factor = 1.25;
    settings.chunk_size = 48; /* space for a modest key and value */
    settings.prefix_delimiter = ':';
    settings.detail_enabled = 0;
    settings.backlog = 1024;
    //settings.binding_protocol = negotiating_prot;
    settings.item_size_max = 1024 * 1024; /* The famous 1MB upper limit. */
    settings.lru_crawler = true;
    settings.lru_crawler_sleep = 100;
    settings.lru_crawler_tocrawl = 0;
    settings.lru_maintainer_thread = true;
    settings.hot_lru_pct = 32;
    settings.warm_lru_pct = 32;
    settings.expirezero_does_not_evict = false;
    settings.hashpower_init = 0;
    settings.slab_reassign = true;
    settings.slab_automove = 0;
    settings.shutdown_command = false;
    settings.tail_repair_time = TAIL_REPAIR_TIME_DEFAULT;
    settings.flush_enabled = true;
    settings.crawls_persleep = 1000;
}



/*
 * We keep the current time of day in a global variable that's updated by a
 * timer event. This saves us a bunch of time() system calls (we really only
 * need to get the time once a second, whereas there can be tens of thousands
 * of requests a second) and allows us to use server-start-relative timestamps
 * rather than absolute UNIX timestamps, a space savings on systems where
 * sizeof(time_t) > sizeof(unsigned int).
 */
volatile rel_time_t current_time;
static pthread_t timer_tid;

static int memory_over_threshold ( int _sys_mem_threshold, int _proc_mem_threshold );
static bool memory_threshold = false;
static int sys_mem_threshold = 0;
static int proc_mem_threshold = 0;

static void save_pid ( const char *pid_file )
{
    FILE *fp;
    if ( access (pid_file, F_OK) == 0 )
    {
        if ( ( fp = fopen (pid_file, "r") ) != NULL )
        {
            char buffer[1024];
            if ( fgets (buffer, sizeof (buffer ), fp) != NULL )
            {
                unsigned int pid;
                if ( safe_strtoul (buffer, &pid) && kill (( pid_t ) pid, 0) == 0 )
                {
                    fprintf (stderr, "WARNING: The pid file contained the following (running) pid: %u\n", pid);
                }
            }
            fclose (fp);
        }
    }

    /* Create the pid file first with a temporary name, then
     * atomically move the file to the real name to avoid a race with
     * another process opening the file to read the pid, but finding
     * it empty.
     */
    char tmp_pid_file[1024];
    snprintf (tmp_pid_file, sizeof (tmp_pid_file ), "%s.tmp", pid_file);

    if ( ( fp = fopen (tmp_pid_file, "w") ) == NULL )
    {
        vperror ("Could not open the pid file %s for writing", tmp_pid_file);
        return;
    }

    fprintf (fp, "%ld\n", ( long ) getpid ());
    if ( fclose (fp) == - 1 )
    {
        vperror ("Could not close the pid file %s", tmp_pid_file);
    }

    if ( rename (tmp_pid_file, pid_file) != 0 )
    {
        vperror ("Could not rename the pid file from %s to %s",
                 tmp_pid_file, pid_file);
    }
}

static void remove_pidfile ( const char *pid_file )
{
    if ( pid_file == NULL )
        return;

    if ( unlink (pid_file) != 0 )
    {
        vperror ("Could not remove the pid file %s", pid_file);
    }

}

/*
 * On systems that supports multiple page sizes we may reduce the
 * number of TLB-misses by using the biggest available page size
 */
static int enable_large_pages ( void )
{
#if defined(HAVE_GETPAGESIZES) && defined(HAVE_MEMCNTL)
    int ret = - 1;
    size_t sizes[32];
    int avail = getpagesizes (sizes, 32);
    if ( avail != - 1 )
    {
        size_t max = sizes[0];
        struct memcntl_mha arg = { 0 };
        int ii;

        for ( ii = 1; ii < avail; ++ ii )
        {
            if ( max < sizes[ii] )
            {
                max = sizes[ii];
            }
        }

        arg.mha_flags = 0;
        arg.mha_pagesize = max;
        arg.mha_cmd = MHA_MAPSIZE_BSSBRK;

        if ( memcntl (0, 0, MC_HAT_ADVISE, ( caddr_t ) & arg, 0, 0) == - 1 )
        {
            fprintf (stderr, "Failed to set large pages: %s\n",
                     strerror (errno));
            fprintf (stderr, "Will use default page size\n");
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        fprintf (stderr, "Failed to get supported pagesizes: %s\n",
                 strerror (errno));
        fprintf (stderr, "Will use default page size\n");
    }

    return ret;
#else
    return - 1;
#endif
}

void set_current_time ( )
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    current_time = ( rel_time_t ) ( tv.tv_sec - process_started );
}
void *timer_thread ( void * );

int start_timer_thread ( void )
{
    int ret;
    if ( ( ret = pthread_create (&timer_tid, NULL, timer_thread, NULL) ) != 0 )
    {
        return - 1;
    }
    return 0;
}

void *timer_thread ( void *arg )
{
    int timerfd = timerfd_create (CLOCK_REALTIME, 0);
    if ( timerfd == - 1 )
    {
        fprintf (stderr, "timerfd create fail\n");
        return NULL;
    }

    struct itimerspec tv;
    memset (&tv, 0, sizeof (tv ));
    tv.it_value.tv_sec = 1;
    tv.it_interval.tv_sec = 1;

    if ( timerfd_settime (timerfd, 0, &tv, NULL) == - 1 )
    {
        fprintf (stderr, "timerfd settime failed\n");
        return NULL;
    }

    struct pollfd pfd;
    pfd.fd = timerfd;
    pfd.events = POLLIN;

    uint64_t val;
    int ret;
    while ( 1 )
    {
        ret = poll (&pfd, 1, 5000);
        if ( ret == - 1 )
        {
            if ( errno == EINTR )
            {
                continue;
            }
            return NULL;
        }
        else if ( ret == 0 )
        {
            fprintf (stderr, "time out\n");
        }
        if ( pfd.revents & POLLIN )
        {
            read (timerfd, &val, sizeof (val ));
            set_current_time ();
            if ( current_time % 10 == 0 )
            {
                if ( sys_mem_threshold != 0 || proc_mem_threshold != 0 )
                {
                    memory_over_threshold (sys_mem_threshold, proc_mem_threshold);
                }
            }
        }
    }
    close (timerfd);
    return NULL;
}

int process_delete_command ( char *key, size_t nkey )
{
    item *it;
    it = item_get (key, nkey);
    if ( it )
    {
        item_unlink (it);
        item_remove (it);
        return true;
    }
    else
    {
        //item_remove(it);
        return false;
    }
}

int process_touch_command ( char *key, size_t nkey, int32_t exptime_int )
{
    item *it;
    it = item_touch (key, nkey, realtime (exptime_int));
    if ( it )
    {
        item_update (it);
        item_remove (it);
        return true;
    }
    else
    {
        //item_remove(it);
        return false;
    }
}

int process_get_command ( char *key, size_t nkey, char *dst, int *len )
{
    item *it;
    char *src = NULL;
    it = item_get (key, nkey);
    if ( it )
    {
        item_update (it);
        src = ITEM_data (it);
        *len = it->nbytes - 2;
        memcpy (dst, src, it->nbytes);
        item_remove (it);
        return true;
    }
    else
    {
        //item_remove(it);
        return false;
    }
}

int process_update_command ( char *key, size_t nkey, uint32_t flags, char *value, size_t nbytes, int32_t exptime_int, int comm )
{
    item *it;
    enum store_item_type ret;
    int32_t vlen = ( int32_t ) nbytes;
    if ( exptime_int < 0 )
        exptime_int = REALTIME_MAXDELTA + 1;
    vlen += 2;
    if ( vlen < 0 || vlen - 2 < 0 )
    {
        return false;
    }
    it = item_alloc (key, nkey, flags, realtime (exptime_int), value, vlen);
    if ( it == 0 )
    {
        if ( comm == NREAD_SET )
        {
            it = item_get (key, nkey);
            if ( it )
            {
                item_unlink (it);
                item_remove (it);
            }
        }
        return false;
    }
    ret = store_item (it, comm);
    item_remove (it);
    if ( ret != STORED )
    {
        return false;
    }
    return true;
}

enum store_item_type do_store_item ( item *it, int comm, const uint32_t hv )
{
    char *key = ITEM_key (it);
    item *old_it = do_item_get (key, it->nkey, hv);
    enum store_item_type stored = NOT_STORED;

    item *new_it = NULL;
    int flags;

    if ( old_it != NULL && comm == NREAD_ADD )
    {
        do_item_update (old_it);
    }
    else if ( ! old_it && ( comm == NREAD_REPLACE || comm == NREAD_APPEND || comm == NREAD_PREPEND ) )
    {

    }
    else
    {
        if ( comm == NREAD_APPEND || comm == NREAD_PREPEND )
        {
            if ( stored == NOT_STORED )
            {
                flags = ( int ) strtol (ITEM_suffix (old_it), ( char ** ) NULL, 10);
                new_it = do_item_alloc (key, it->nkey, flags, old_it->exptime, ITEM_data (it), it->nbytes + old_it->nbytes - 2, hv);
                if ( ! new_it )
                {
                    if ( old_it )
                        do_item_remove (old_it);
                    return NOT_STORED;
                }
                if ( comm == NREAD_APPEND )
                {
                    memcpy (ITEM_data (new_it), ITEM_data (old_it), old_it->nbytes);
                    memcpy (ITEM_data (new_it) + old_it->nbytes - 2, ITEM_data (it), it->nbytes);
                }
                else
                {
                    memcpy (ITEM_data (new_it), ITEM_data (it), it->nbytes);
                    memcpy (ITEM_data (new_it) + it->nbytes - 2, ITEM_data (old_it), old_it->nbytes);
                }
                it = new_it;
            }
        }
        if ( stored == NOT_STORED )
        {
            if ( old_it != NULL )
            {
                item_replace (old_it, it, hv);
            }
            else
            {
                do_item_link (it, hv);
            }
            stored = STORED;
        }
    }
    if ( old_it != NULL )
    {
        do_item_remove (old_it);
    }
    if ( new_it != NULL )
    {
        do_item_remove (new_it);
    }
    return stored;
}

int init ( unsigned int size )
{
    enum hashfunc_type hash_type = JENKINS_HASH;
    bool preallocate = false;
    bool start_lru_crawler = true;
    process_started = time (0);

    settings_init (size);
    init_lru_crawler ();
    init_lru_maintainer ();
    setbuf (stderr, NULL);
    if ( hash_init (hash_type) != 0 )
    {
        fprintf (stderr, "Failed to initialize hash_algorithm!\n");
        return - 1;
    }
    assoc_init (settings.hashpower_init);
    if ( enable_large_pages () == 0 )
    {
        preallocate = true;
    }
    slabs_init (settings.maxbytes, settings.factor, preallocate);

    memcached_thread_init ();


    if ( start_assoc_maintenance_thread () == - 1 )
    {
        return - 1;
    }
    if ( start_lru_crawler && start_lru_maintainer_thread () != 0 )
    {
        fprintf (stderr, "Failed to enable LRU maintainer thread\n");
        return - 1;
    }
    if ( settings.slab_reassign && start_slab_maintenance_thread () == - 1 )
    {
        return - 1;
    }
    if ( start_timer_thread () != 0 )
    {
        return - 1;
    }
    return 0;
}

unsigned int get_maxbytes ( )
{
    return settings.maxbytes / ( 1024 * 1024 );
}

uint32_t mdb_timestamp ( )
{
    return current_time + ( uint32_t ) process_started;
}

void set_mem_threshold ( int _sys_mem_threshold, int _proc_mem_threshold )
{
    sys_mem_threshold = _sys_mem_threshold;
    proc_mem_threshold = _proc_mem_threshold;
}

static int memory_over_threshold ( int _sys_mem_threshold, int _proc_mem_threshold )
{
    long rss = 0L;
    FILE *fp = NULL, *meminfo_fp = NULL;
    long mem[4] = { 0L };

    if ( ( meminfo_fp = fopen ("/proc/meminfo", "r") ) == NULL )
    {
        return - 1;
    }
    for ( int i = 0; i < 4; i ++ )
    {
        if ( fscanf (meminfo_fp, "%*s%ld%*s", mem + i) != 1 )
        {
            fclose (meminfo_fp);
            return - 1;
        }
    }
    fclose (meminfo_fp);

    unsigned long freeram = ( mem[1] + mem[2] + mem[3] ) * 1024;
    unsigned long totalram = mem[0] * 1024;

    if ( _sys_mem_threshold != 0 && ( 100 * freeram / totalram ) < ( 100 - _sys_mem_threshold ) )
    {
        memory_threshold = true;
        return 1;
    }

    if ( ( fp = fopen ("/proc/self/statm", "r") ) == NULL )
    {
        return - 1;
    }

    if ( fscanf (fp, "%*s%ld", &rss) != 1 )
    {
        fclose (fp);
        return - 1;
    }
    fclose (fp);
    size_t process_total = ( size_t ) rss * ( size_t ) sysconf (_SC_PAGESIZE);
    if ( _proc_mem_threshold != 0 && ( 100 * process_total / totalram ) > _proc_mem_threshold )
    {
        memory_threshold = true;
        return 1;
    }
    memory_threshold = false;
    return 0;
}

bool is_memory_over_threshold ( )
{
    return memory_threshold;
}