SERVER      = $(top_srcdir)/server/

DEPS        = $(top_srcdir)/../../third_party/
DEPS_PREFIX = /opt/huststore/3rd

INCLUDES    = -I $(SERVER)/ -I $(SERVER)/module -I $(SERVER)/include -I $(MDB_SRC)lib -I $(SERVER)/module/rdb/lib -I $(DEPS_PREFIX)/include -I $(DEPS_PREFIX)/include/evhtp -I $(DEPS)/leveldb/include/ -I $(DEPS)/include/curl -I $(DEPS_PREFIX)/include/curl

AM_CFLAGS = -std=gnu99 -fPIC -O0 -g3 -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_TCMALLOC

AM_LDFLAGS   = -export-dynamic -L/usr/lib -L/usr/local/lib -L$(DEPS_PREFIX)/lib -lpthread -lssl -lcrypto -pthread -lrt -levent -levent_pthreads -levent_openssl -levhtp -ldl -lz -ltcmalloc -lidn




STATIC_LIBS = $(DEPS_PREFIX)/lib/libleveldb.a $(DEPS_PREFIX)/lib/libsnappy.a $(DEPS_PREFIX)/lib/libcurl.a






noinst_LTLIBRARIES = libmdb.la
libmdb_la_SOURCES = src/memcached.c lib/memcached.h src/items.c lib/items.h lib/memcached.h src/assoc.c lib/assoc.h lib/memcached.h src/hash.c lib/hash.h lib/jenkins_hash.h lib/murmur3_hash.h lib/memcached.h src/jenkins_hash.c lib/jenkins_hash.h lib/memcached.h src/murmur3_hash.c lib/murmur3_hash.h lib/memcached.h src/slabs.c lib/slabs.h lib/memcached.h src/thread.c lib/memcached.h src/util.c lib/util.h lib/memcached.h mdb.cpp mdb.h lib/memcached.h

