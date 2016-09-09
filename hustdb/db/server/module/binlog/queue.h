#ifndef __HUSTSTORE_BINLOG_QUEUE_H_
#define __HUSTSTORE_BINLOG_QUEUE_H_

#include <string>
#include <deque>

#include "mutex.h"
#include "condition.h"

class task_t;

class queue_t {
public:
    explicit queue_t(size_t max_queue_size);
    ~queue_t();

    bool put_with_check(task_t *);
    bool put(task_t *);
    task_t * take();

private:
    queue_t(const queue_t &);
    const queue_t & operator= (const queue_t &);

    size_t size() const;
    bool is_full();

    size_t _max_queue_size;

    mutable mutex_lock_t _mutex;
    condition_t _cond;

    std::deque<task_t *> _queue;
};

#endif