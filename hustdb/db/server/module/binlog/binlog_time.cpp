#include "binlog_time.h"
#include "mutex.h"

int64_t binlog_time_t::_current_time = 0;
rw_lock_t binlog_time_t::_rwlock = rw_lock_t();