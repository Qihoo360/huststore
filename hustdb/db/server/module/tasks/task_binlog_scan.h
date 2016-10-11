#ifndef _task_binlog_scan_h_
#define _task_binlog_scan_h_

#include "../hustdb.h"
#include "slow_task_thread.h"

class task_binlog_scan_t : public task2_t
{
public:

    static task_binlog_scan_t * create ( );

    virtual void release ( );

    virtual void process ( );

private:

    void process_binlog_scan ( );

public:

    task_binlog_scan_t ( );
    ~task_binlog_scan_t ( );
};

#endif
