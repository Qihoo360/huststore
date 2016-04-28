#pragma once

#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include "item.h"
#include "message.h"
#include "c_base64.h"
#include <string>
#include <vector>

extern std::vector<std::string> hosts;

#define HUSTDB_METHOD_PUT 1
#define HUSTDB_METHOD_DEL 2
#define HUSTDB_METHOD_HSET 3
#define HUSTDB_METHOD_HDEL 4
#define HUSTDB_METHOD_SADD 5
#define HUSTDB_METHOD_SREM 6
#define HUSTDB_METHOD_TB_PUT 7
#define HUSTDB_METHOD_TB_UPDATE 8
#define HUSTDB_METHOD_TB_DELETE 9

class Task
{
public:
    Task ( bool (*fn_ptr )( void * ), void *args );
    ~Task ( );
    bool operator() ( void * );
    void run ( );
private:
    bool (*_fn_ptr )( void * );
    void *_args;
};
