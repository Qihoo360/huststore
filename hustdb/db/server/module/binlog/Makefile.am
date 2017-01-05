SERVER      = $(top_srcdir)/server/

DEPS        = $(top_srcdir)/../../third_party/
DEPS_PREFIX = /opt/huststore/3rd

INCLUDES    = -I $(SERVER)/ -I $(SERVER)/module -I $(SERVER)/include -I $(MDB_SRC)lib -I $(SERVER)/module/rdb/lib -I $(DEPS)/leveldb/include/ -I $(DEPS)/include/curl -I $(DEPS_PREFIX)/include/curl -I /usr/local/include -I /usr/local/include/evhtp -I $(DEPS)/libevhtp/ -I /usr/local/include/event2 -I $(DEPS_PREFIX)/include

AM_CPPFLAGS = -fPIC -O0 -g3 -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_TCMALLOC $(INCLUDES)
AM_CFLAGS = -std=gnu99 $(AM_CPPFLAGS)

AM_LDFLAGS   = -export-dynamic -L/usr/lib -L/usr/local/lib -L$(DEPS_PREFIX)/lib -lpthread -lssl -lcrypto -pthread -lrt -levent -levent_pthreads -levent_openssl -levhtp -ldl -lz -ltcmalloc -lidn




STATIC_LIBS = $(DEPS_PREFIX)/lib/libleveldb.a $(DEPS_PREFIX)/lib/libsnappy.a $(DEPS_PREFIX)/lib/libcurl.a



noinst_LTLIBRARIES = libbinlog.la
libbinlog_la_SOURCES = binlog.cpp binlog.h singleton.h thread_pool.h host_info.h task.h condition.cpp condition.h host_info.cpp host_info.h queue.h task.h thread_pool.h husthttp.h thread.h singleton.h atomic2.h binlog_time.h http.cpp http.h husthttp.cpp husthttp.h http.h mutex.cpp mutex.h queue.cpp queue.h task.h task.cpp task.h husthttp.h singleton.h host_info.h binlog_time.h thread.cpp thread.h thread_pool.cpp thread_pool.h husthttp.h task.h binlog_time.cpp binlog_time.h mutex.h
libbinlog_la_CPPFLAGS = $(AM_CPPFLAGS)
libbinlog_la_LDFLAGS = $(AM_LDFLAGS) -lcurl
libbinlog_la_LIBADD = $(DEPS_PREFIX)/lib/libcurl.a

