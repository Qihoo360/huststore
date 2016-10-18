#ifndef __HUSTSTORE_BINLOG_BINLOGTIME_H
#define __HUSTSTORE_BINLOG_BINLOGTIME_H

#include <sys/time.h>
#include <stdint.h>

#include "mutex.h"

class binlog_time_t
{
public:
    static int set_current_time()
    {
        rw_lock_guard_t lock ( _rwlock, WLOCK );
        struct timeval tv;

        if ( gettimeofday ( &tv, NULL ) != 0 ) {
            return -1;
        }

        _current_time = tv.tv_sec;
        return 0;
    }

    static int64_t get_current_time()
    {
        rw_lock_guard_t lock ( _rwlock, RLOCK );
        return _current_time;
    }
private:
    binlog_time_t();
    ~binlog_time_t();
    static int64_t _current_time;
    static rw_lock_t _rwlock;
};

#endif