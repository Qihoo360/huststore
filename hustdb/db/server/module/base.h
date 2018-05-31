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

#define HUSTDB_EXPORT_DIR               "./EXPORT"

#define DB_DATA_ROOT                    "./DATA"
#define DB_DATA_DIR                     "./DATA/DATA"
#define DB_META_INDEX_DIR               "./DATA/meta_index"
#define DB_CONTENTS_DIR                 "./DATA/contentdb"
#define DB_FULLKEY_DIR                  "./DATA/fullkey"
#define DB_BUCKETS_DIR                  "./DATA/bucket"
#define DB_CONFLICT_DIR                 "./DATA/conflictdb"
#define DB_FAST_CONFLICT_DIR            "./DATA/fast_conflictdb"
#define DB_HASH_CONFIG                  "./DATA/DATA/hash.conf"
#define DB_DATA_FILE                    "./DATA/DATA/$(FILE_ID).db"

#define HUSTMQ_QUEUE                    "./DATA/meta_index/mq_queue"
#define HUSTDB_TABLE                    "./DATA/meta_index/db_table"
#define HUSTSTORE_INVARIANT             "./DATA/meta_index/invariant"

#define PAGE                            1024

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
