# hustdb for linux

CC          = gcc -std=gnu99
CPPC        = g++
BIN         = ./o/
SERVER      = ../../
DEPS        = ../../../third_party/
INCLUDE     = -I $(SERVER)include -I $(MDB_SRC)lib -I $(SERVER)module/rdb/lib -I $(DEPS)leveldb/include
LIB         = -L /usr/lib -L /usr/local/lib -lpthread -lssl -lcrypto -pthread -lrt
MALLOC      = -Wl,-wrap,malloc  -Wl,-wrap,free  -Wl,-wrap,realloc
CFLAGS      = -fPIC -O0 -g3 -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DUSE_TCMALLOC
LEVELDB_LIB = $(DEPS)leveldb/libleveldb.a $(DEPS)snappy-1.1.1/.libs/libsnappy.a

$(BIN)module_%.o: $(SERVER)module/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)tasks_%.o: $(SERVER)module/tasks/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)base_%.o: $(SERVER)module/utils/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)kv_%.o: $(SERVER)module/kv/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)kv_leveldb_%.o: $(SERVER)module/kv/leveldb/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)kv_md5db_%.o: $(SERVER)module/kv/md5db/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)kv_array_%.o: $(SERVER)module/kv/kv_array/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

SVR_OBJECTS=                                \
	$(BIN)main.o \
	$(BIN)server_utils.o \
	$(BIN)hustdb_network.o \
	$(BIN)hustdb_handler_frame.o \
	$(BIN)hustdb_handler.o \
	$(BIN)hustdb_handler_def.o \
	$(BIN)hustdb_network_utils.o \
	$(BIN)libevhtp_utils.o \
	$(BIN)hustdb_utils.o

MODULE_OBJECTS=  					        \
	$(BIN)kv_create_kv.o 				        \
	$(BIN)kv_md5db_bucket.o			                \
	$(BIN)kv_md5db_bucket_array.o		                \
	$(BIN)kv_md5db_content.o			                \
	$(BIN)kv_md5db_content_array.o	                        \
	$(BIN)kv_md5db_fast_conflict.o	                        \
	$(BIN)kv_md5db_fast_conflict_array.o	                \
	$(BIN)kv_md5db_fullkey.o			                \
	$(BIN)kv_md5db_fullkey_array.o	                        \
	$(BIN)kv_md5db_conflict.o			                \
	$(BIN)kv_md5db_conflict_array.o	                        \
	$(BIN)kv_md5db_kv_md5db.o			                \
	$(BIN)kv_leveldb_kv_leveldb.o 	                        \
	$(BIN)kv_leveldb_bloom_filter.o	                        \
	$(BIN)kv_array_kv_array.o			                \
	$(BIN)kv_array_kv_config.o		                \
	$(BIN)kv_array_key_hash.o			                \
	$(BIN)module_hustdb.o       		                \
	$(BIN)module_apptool.o        		                \
	$(BIN)module_appini.o        		                \
	$(BIN)base_md5.o   			                \
	$(BIN)base_timer.o   			                \
	$(BIN)tasks_slow_task_thread.o                          \
	$(BIN)tasks_task_export.o                               \
	$(BIN)tasks_task_ttl_scan.o

MDB_OBJECTS =                                                   \
        $(BIN)memcached.o                                             \
	$(BIN)items.o                                             \
	$(BIN)assoc.o                                             \
	$(BIN)hash.o                                              \
	$(BIN)jenkins_hash.o                                      \
	$(BIN)murmur3_hash.o                                      \
	$(BIN)slabs.o                                             \
	$(BIN)thread.o                                            \
	$(BIN)mdb_util.o                                          \
	$(BIN)mdb.o
	
RDB_OBJECTS =                                                   \
	$(BIN)adlist.o 	                                        \
	$(BIN)bitops.o 	                                        \
	$(BIN)db.o 		                                \
	$(BIN)dict.o 		                                \
	$(BIN)intset.o 	                                        \
	$(BIN)object.o 	                                        \
	$(BIN)pqsort.o 	                                        \
	$(BIN)quicklist.o                                       \
	$(BIN)sds.o 		                                \
	$(BIN)server.o 	                                        \
	$(BIN)sort.o 		                                \
	$(BIN)t_hash.o 	                                        \
	$(BIN)t_list.o 	                                        \
	$(BIN)t_set.o 	                                        \
	$(BIN)t_string.o 	                                \
	$(BIN)t_zset.o 	                                        \
	$(BIN)rdb_util.o                                        \
	$(BIN)ziplist.o 	                                \
	$(BIN)zmalloc.o 	                                \
	$(BIN)lzf_c.o 	                                        \
	$(BIN)lzf_d.o 	                                        \
	$(BIN)rdb.o

TARGET = ./hustdb
INCS = $(INCLUDE) -I $(SERVER)module -I /usr/local/include -I /usr/local/include/evhtp -I /usr/local/include/event2
LIBS =	$(LIB) -levent -levent_pthreads -levent_openssl -levhtp -ldl -lz -ltcmalloc

all: $(MDB_OBJECTS) $(RDB_OBJECTS) $(MODULE_OBJECTS) $(SVR_OBJECTS)

	$(CPPC) -rdynamic -o $(TARGET) $(INCS) \
			$(MODULE_OBJECTS) $(MDB_OBJECTS) $(RDB_OBJECTS) $(SVR_OBJECTS) \
			$(LEVELDB_LIB) $(LIBS)

	chmod a+x $(TARGET)
	cp $(SERVER)module/hustdb.conf hustdb.conf

NETWORK     = $(SERVER)network/

$(BIN)main.o:	$(SERVER)main.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)main.o $(SERVER)main.cpp

$(BIN)server_utils.o:	$(SERVER)server_utils.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)server_utils.o $(SERVER)server_utils.cpp

$(BIN)hustdb_network.o:	$(NETWORK)hustdb_network.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)hustdb_network.o $(NETWORK)hustdb_network.cpp

$(BIN)hustdb_handler_frame.o:	$(NETWORK)hustdb_handler_frame.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)hustdb_handler_frame.o $(NETWORK)hustdb_handler_frame.cpp

$(BIN)hustdb_handler.o:	$(NETWORK)hustdb_handler.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)hustdb_handler.o $(NETWORK)hustdb_handler.cpp

$(BIN)hustdb_handler_def.o:	$(NETWORK)hustdb_handler_def.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)hustdb_handler_def.o $(NETWORK)hustdb_handler_def.cpp

$(BIN)hustdb_network_utils.o:	$(NETWORK)hustdb_network_utils.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)hustdb_network_utils.o $(NETWORK)hustdb_network_utils.cpp

$(BIN)libevhtp_utils.o:	$(NETWORK)libevhtp_utils.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)libevhtp_utils.o $(NETWORK)libevhtp_utils.cpp

$(BIN)hustdb_utils.o:	$(NETWORK)hustdb_utils.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)hustdb_utils.o $(NETWORK)hustdb_utils.cpp


MDB_SRC     = $(SERVER)module/mdb/

$(BIN)memcached.o: $(MDB_SRC)src/memcached.c $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)items.o: $(MDB_SRC)src/items.c $(MDB_SRC)lib/items.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)assoc.o: $(MDB_SRC)src/assoc.c $(MDB_SRC)lib/assoc.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)hash.o: $(MDB_SRC)src/hash.c $(MDB_SRC)lib/hash.h $(MDB_SRC)lib/jenkins_hash.h $(MDB_SRC)lib/murmur3_hash.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)jenkins_hash.o: $(MDB_SRC)src/jenkins_hash.c $(MDB_SRC)lib/jenkins_hash.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)murmur3_hash.o: $(MDB_SRC)src/murmur3_hash.c $(MDB_SRC)lib/murmur3_hash.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)slabs.o: $(MDB_SRC)src/slabs.c $(MDB_SRC)lib/slabs.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)thread.o: $(MDB_SRC)src/thread.c $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)mdb_util.o: $(MDB_SRC)src/util.c $(MDB_SRC)lib/util.h $(MDB_SRC)lib/memcached.h
	$(CC) $(INCLUDE) -c -o $@ $<
	
$(BIN)mdb.o: $(MDB_SRC)mdb.cpp $(MDB_SRC)mdb.h $(MDB_SRC)lib/memcached.h
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

RDB_SRC     = $(SERVER)module/rdb/

$(BIN)adlist.o : $(RDB_SRC)src/adlist.c $(RDB_SRC)src/adlist.h $(RDB_SRC)src/zmalloc.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)bitops.o : $(RDB_SRC)src/bitops.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)db.o : $(RDB_SRC)src/db.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)dict.o : $(RDB_SRC)src/dict.c $(RDB_SRC)src/dict.h $(RDB_SRC)src/zmalloc.h $(RDB_SRC)lib/redis.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)intset.o : $(RDB_SRC)src/intset.c $(RDB_SRC)src/intset.h $(RDB_SRC)src/zmalloc.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)object.o : $(RDB_SRC)src/object.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)pqsort.o : $(RDB_SRC)src/pqsort.c $(RDB_SRC)src/pqsort.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)quicklist.o : $(RDB_SRC)src/quicklist.c $(RDB_SRC)src/quicklist.h $(RDB_SRC)src/zmalloc.h $(RDB_SRC)src/ziplist.h $(RDB_SRC)src/util.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)sds.o : $(RDB_SRC)src/sds.c $(RDB_SRC)src/sds.h $(RDB_SRC)src/sdsalloc.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)server.o : $(RDB_SRC)src/server.c $(RDB_SRC)src/server.h $(RDB_SRC)src/sds.h $(RDB_SRC)src/dict.h $(RDB_SRC)src/adlist.h $(RDB_SRC)src/zmalloc.h $(RDB_SRC)src/ziplist.h $(RDB_SRC)src/intset.h $(RDB_SRC)src/util.h $(RDB_SRC)src/quicklist.h $(RDB_SRC)lib/redis.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)sort.o : $(RDB_SRC)src/sort.c $(RDB_SRC)src/server.h $(RDB_SRC)src/pqsort.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)t_hash.o : $(RDB_SRC)src/t_hash.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)t_list.o : $(RDB_SRC)src/t_list.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)t_set.o : $(RDB_SRC)src/t_set.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)t_string.o : $(RDB_SRC)src/t_string.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)t_zset.o : $(RDB_SRC)src/t_zset.c $(RDB_SRC)src/server.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)rdb_util.o : $(RDB_SRC)src/util.c $(RDB_SRC)src/util.h $(RDB_SRC)src/fmacros.h	
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)ziplist.o : $(RDB_SRC)src/ziplist.c $(RDB_SRC)src/ziplist.h $(RDB_SRC)src/util.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)zmalloc.o : $(RDB_SRC)src/zmalloc.c $(RDB_SRC)src/zmalloc.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)lzf_c.o : $(RDB_SRC)src/lzf_c.c $(RDB_SRC)src/lzf.h $(RDB_SRC)src/lzfP.h
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN)lzf_d.o : $(RDB_SRC)src/lzf_d.c $(RDB_SRC)src/lzf.h $(RDB_SRC)src/lzfP.h
	$(CC) -c $(CFLAGS) $< -o $@
	
$(BIN)rdb.o: $(RDB_SRC)rdb.cpp $(RDB_SRC)rdb.h $(RDB_SRC)lib/redis.h
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<
	
clean:
	rm -fr $(TDB_OBJECTS) $(MDB_OBJECTS) $(RDB_OBJECTS) $(MODULE_OBJECTS) $(SVR_OBJECTS)
	rm -fr $(TARGET)
	rm -fr ./hustdb.conf
