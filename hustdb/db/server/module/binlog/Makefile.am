SERVER      = $(top_srcdir)/server/

DEPS_PREFIX = /opt/huststore/3rd

INCLUDES    = \
    -I $(SERVER)/ \
    -I $(SERVER)/module \
    -I $(SERVER)/include \
    -I $(SERVER)/module/mdb/lib \
    -I $(SERVER)/module/rdb/lib \
    -I $(DEPS_PREFIX)/include \
    -I $(DEPS_PREFIX)/include/evhtp \
    -I $(DEPS_PREFIX)/include/gperftools \
    -I $(DEPS_PREFIX)/include/google \
    -I $(DEPS_PREFIX)/include/leveldb \
    -I $(DEPS_PREFIX)/include/event2 \
    -I $(DEPS_PREFIX)/include/curl

AM_CPPFLAGS = -fPIC -O0 -g3 -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_TCMALLOC $(INCLUDES)
AM_CFLAGS = -std=gnu99 $(AM_CPPFLAGS)

AM_LDFLAGS   = -export-dynamic -L$(DEPS_PREFIX)/lib -L/usr/lib -L/usr/local/lib -lpthread -lssl -lcrypto -pthread -lrt -levent -levent_pthreads -levent_openssl -levhtp -ldl -lz -ltcmalloc -lidn



noinst_LTLIBRARIES = libbinlog.la
libbinlog_la_LIBADD = $(DEPS_PREFIX)/lib/libcurl.la
libbinlog_la_LDFLAGS = $(AM_LDFLAGS)

libbinlog_la_SOURCES = binlog.cpp binlog.h singleton.h thread_pool.h host_info.h task.h condition.cpp condition.h host_info.cpp host_info.h queue.h task.h thread_pool.h husthttp.h thread.h singleton.h atomic2.h binlog_time.h http.cpp http.h husthttp.cpp husthttp.h http.h mutex.cpp mutex.h queue.cpp queue.h task.h task.cpp task.h husthttp.h singleton.h host_info.h binlog_time.h thread.cpp thread.h thread_pool.cpp thread_pool.h husthttp.h task.h binlog_time.cpp binlog_time.h mutex.h
