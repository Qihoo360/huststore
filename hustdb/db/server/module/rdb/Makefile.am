SERVER      = $(top_srcdir)/server/

DEPS        = $(top_srcdir)/../../third_party/
DEPS_PREFIX = /opt/huststore/3rd

INCLUDES    = -I $(SERVER)/ -I $(SERVER)/module -I $(SERVER)/include -I $(MDB_SRC)lib -I $(SERVER)/module/rdb/lib -I $(DEPS)/leveldb/include/ -I $(DEPS)/include/curl -I $(DEPS_PREFIX)/include/curl -I /usr/local/include -I /usr/local/include/evhtp -I $(DEPS)/libevhtp/ -I /usr/local/include/event2 -I $(DEPS_PREFIX)/include

AM_CFLAGS = -std=gnu99 -fPIC -O0 -g3 -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_TCMALLOC

AM_LDFLAGS   = -export-dynamic -L/usr/lib -L/usr/local/lib -L$(DEPS_PREFIX)/lib -lpthread -lssl -lcrypto -pthread -lrt -levent -levent_pthreads -levent_openssl -levhtp -ldl -lz -ltcmalloc -lidn




STATIC_LIBS = $(DEPS_PREFIX)/lib/libleveldb.a $(DEPS_PREFIX)/lib/libsnappy.a $(DEPS_PREFIX)/lib/libcurl.a




noinst_LTLIBRARIES = librdb.la
librdb_la_SOURCES = src/adlist.c lib/adlist.h lib/zmalloc.h src/bitops.c lib/server.h src/db.c lib/server.h src/dict.c lib/dict.h lib/zmalloc.h lib/redis.h src/intset.c lib/intset.h lib/zmalloc.h src/object.c lib/server.h src/pqsort.c lib/pqsort.h src/quicklist.c lib/quicklist.h lib/zmalloc.h lib/ziplist.h lib/util.h src/sds.c lib/sds.h lib/sdsalloc.h src/server.c lib/server.h lib/sds.h lib/dict.h lib/adlist.h lib/zmalloc.h lib/ziplist.h lib/intset.h lib/util.h lib/quicklist.h lib/redis.h src/sort.c lib/server.h lib/pqsort.h src/t_hash.c lib/server.h src/t_list.c lib/server.h src/t_set.c lib/server.h src/t_string.c lib/server.h src/t_zset.c lib/server.h src/util.c lib/util.h lib/fmacros.h src/ziplist.c lib/ziplist.h lib/util.h src/zmalloc.c lib/zmalloc.h src/lzf_c.c lib/lzf.h lib/lzfP.h src/lzf_d.c lib/lzf.h lib/lzfP.h rdb.cpp rdb.h lib/redis.h

