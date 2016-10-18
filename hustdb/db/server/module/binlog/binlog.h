#ifndef __HUSTSTORE_BINLOG_BINLOG_H_
#define __HUSTSTORE_BINLOG_BINLOG_H_

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

typedef void ( * binlog_callback_func_t ) ( void * param );

class thread_pool_t;

class binlog_t
{
public:
    explicit binlog_t ( );
    ~binlog_t ( );

    bool init (
                int num_threads,
                size_t max_queue_size,
                size_t max_backup_queue_size,
                size_t alive_time,
                const char * username = NULL,
                const char * password = NULL
                );

    void get_alives (
                      std::map<std::string, char> & alives
                      );

    bool remove_host (
                       const std::string & host
                       );

    bool add_host (
                    const std::string & host
                    );

    void set_callback (
                        binlog_callback_func_t callback_func
                        );

    bool add_task (
                    const char * host,
                    size_t host_len,
                    const char * table,
                    size_t table_len,
                    const char * key,
                    size_t key_len,
                    const char * value,
                    size_t value_len,
                    uint32_t ver,
                    uint32_t ttl,
                    uint8_t cmd_type,
                    void * param
                    );

    void get_status (
                      std::string & res
                      );

private:
    binlog_t ( const binlog_t & );
    const binlog_t & operator= ( const binlog_t & );

    thread_pool_t * _pool;

    binlog_callback_func_t _callback_func;

    std::string _auth;

    size_t _alive_time;
};

#endif