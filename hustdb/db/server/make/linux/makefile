# hustdb for linux

CC          = gcc
CPPC        = g++
LINK        = $(CC) -std=gnu99 -lpthread
BIN         = ./o/
SERVER      = ../../
DEPS        = ../../../third_party/
INCLUDE     = -I $(SERVER)include -I $(SERVER)module/mdb/lib -I $(SERVER)module/tdb/serial/rapidjson -I $(SERVER)module/tdb/serial/include -I $(DEPS)leveldb/include
LIB         = -L /usr/lib -lpthread -lssl -lcrypto -pthread -lrt
MALLOC      = -Wl,-wrap,malloc  -Wl,-wrap,free  -Wl,-wrap,realloc
CFLAGS      = -fPIC -O0 -g3 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -fmessage-length=0
LEVELDB_LIB = $(DEPS)leveldb/libleveldb.a $(DEPS)snappy-1.1.1/.libs/libsnappy.a

$(BIN)module_%.o: $(SERVER)module/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)tasks_%.o: $(SERVER)module/tasks/%.cpp
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(BIN)base_%.o: $(SERVER)module/md5/%.c
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
	$(BIN)module_slow_task_thread.o                           \
	$(BIN)module_hustdb.o       		                \
	$(BIN)module_apptool.o        		                \
	$(BIN)module_appini.o        		                \
	$(BIN)base_md5.o   			                \
	$(BIN)tasks_task_export.o

MDB_OBJECTS =                                                   \
    $(BIN)memcached.o                                             \
	$(BIN)items.o                                             \
	$(BIN)assoc.o                                             \
	$(BIN)hash.o                                              \
	$(BIN)jenkins_hash.o                                      \
	$(BIN)murmur3_hash.o                                      \
	$(BIN)slabs.o                                             \
	$(BIN)thread.o                                            \
	$(BIN)util.o                                              \
	$(BIN)mdb.o

TARGET = ./hustdb
INCS = $(INCLUDE) -I $(SERVER)module -I /usr/local/include/evhtp -I /usr/local/include/event2
LIBEVENT = -L /usr/local/lib
LIBEVHTP = /usr/local/lib/libevhtp.a
LIBS =	$(LIB) -levent -levent_pthreads -levent_openssl -levhtp -ldl -lz

all: $(MDB_OBJECTS) $(MODULE_OBJECTS) $(SVR_OBJECTS)

	$(CPPC) -rdynamic -o $(TARGET) $(INCS) \
			$(LIBEVENT) $(LIBEVHTP) \
			$(MODULE_OBJECTS) $(MDB_OBJECTS) $(SVR_OBJECTS) \
			$(LEVELDB_LIB) $(LIBS)

	chmod a+x $(TARGET)
	cp $(SERVER)module/hustdb.conf hustdb.conf

NETWORK     = $(SERVER)network/
$(BIN)main.o:	$(NETWORK)main.cpp
	$(CPPC) -c $(CFLAGS)  $(INCS) -o $(BIN)main.o $(NETWORK)main.cpp

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


$(BIN)memcached.o: $(SERVER)module/mdb/src/memcached.c $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)items.o: $(SERVER)module/mdb/src/items.c $(SERVER)module/mdb/lib/items.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)assoc.o: $(SERVER)module/mdb/src/assoc.c $(SERVER)module/mdb/lib/assoc.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)hash.o: $(SERVER)module/mdb/src/hash.c $(SERVER)module/mdb/lib/hash.h $(SERVER)module/mdb/lib/jenkins_hash.h $(SERVER)module/mdb/lib/murmur3_hash.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)jenkins_hash.o: $(SERVER)module/mdb/src/jenkins_hash.c $(SERVER)module/mdb/lib/jenkins_hash.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)murmur3_hash.o: $(SERVER)module/mdb/src/murmur3_hash.c $(SERVER)module/mdb/lib/murmur3_hash.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)slabs.o: $(SERVER)module/mdb/src/slabs.c $(SERVER)module/mdb/lib/slabs.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)thread.o: $(SERVER)module/mdb/src/thread.c $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)util.o: $(SERVER)module/mdb/src/util.c $(SERVER)module/mdb/lib/util.h $(SERVER)module/mdb/lib/memcached.h
	$(LINK) $(INCLUDE) -c -o $@ $<
	
$(BIN)mdb.o: $(SERVER)module/mdb/mdb.cpp $(SERVER)module/mdb/mdb.h $(SERVER)module/mdb/lib/memcached.h
	$(CPPC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

	
clean:
	rm -fr $(TDB_OBJECTS) $(MDB_OBJECTS) $(MODULE_OBJECTS) $(SVR_OBJECTS)
	rm -fr $(TARGET)
	rm -fr ./hustdb.conf
