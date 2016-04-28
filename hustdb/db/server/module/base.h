#ifndef _base_h_
#define _base_h_

#include "apptool.h"
#include "appini.h"

#ifdef _DEBUG
#define ENABLE_DEBUG                    1
#else
#define ENABLE_DEBUG                    0
#endif

#define ENABLE_PERF_INFO                1

#define RESERVE_BYTES_FOR_RSP_BUFER     1048576

#define HUSTDB_LOG_DIR                  "./LOG"

#define HUSTDB_CONFIG                   "./hustdb.conf"

#define HUSTMQ_INDEX                    "./DATA/HUSTMQ"
#define HUSTTABLE_INDEX                 "./DATA/HUSTTABLE"

#define DB_DATA_ROOT                    "./DATA"
#define DB_DATA_DIR                     "./DATA/DATA"
#define DB_CONTENTS_DIR                 "./DATA/contents"
#define DB_FULLKEY_DIR                  "./DATA/fullkeys"
#define DB_BUCKETS_DIR                  "./DATA/buckets"
#define DB_CONFLICT_DIR                 "./DATA/conflict"
#define DB_FAST_CONFLICT_DIR            "./DATA/fast_conflict"
#define DB_HASH_CONFIG                  "./DATA/DATA/hash.conf"
#define DB_DATA_FILE                    "./DATA/DATA/$(FILE_ID).db"

#define CONFLICT_CACHE_M                256
#define CONFLICT_WRITE_BUFFER_M         256

#define LOG_FATAL( fmt, ... )           log_fatal( (fmt), ##__VA_ARGS__ )
#define LOG_ERROR( fmt, ... )           log_error( (fmt), ##__VA_ARGS__ )
#define LOG_WARNING( fmt, ... )         log_warning( (fmt), ##__VA_ARGS__ )
#define LOG_INFO( fmt, ... )            log_info( (fmt), ##__VA_ARGS__ )
#if ENABLE_DEBUG
#define LOG_DEBUG( fmt, ... )           log_debug( (fmt), ##__VA_ARGS__ )
#else
#define LOG_DEBUG( fmt, ... )
#endif

#endif // #ifndef _base_h_
