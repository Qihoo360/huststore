#ifndef _task_ttl_scan_h_
#define _task_ttl_scan_h_

#include "../hustdb.h"
#include "../slow_task_thread.h"

class task_ttl_scan_t : public task2_t
{
public:

    static task_ttl_scan_t * create (
                                    uint32_t size,
                                    uint16_t start = 0,
                                    uint16_t end = MAX_BUCKET_NUM
                                    );

    virtual void release ( );

    virtual void process ( );

private:

    void process_ttl_scan ( );

private:

    uint32_t m_size;
    uint16_t m_start;
    uint16_t m_end;

public:

    task_ttl_scan_t (
                    uint32_t size,
                    uint16_t start,
                    uint16_t end
                    );
    ~task_ttl_scan_t ( );
};

#endif
