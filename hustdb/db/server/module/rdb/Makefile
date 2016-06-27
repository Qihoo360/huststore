CC = gcc -std=gnu99
CPPC = g++
CXXFLAGS = -O0 -g3 -Wall -fmessage-length=0 -DUSE_TCMALLOC
LINK = $(CPPC) -lpthread -ltcmalloc
RDB = src/

TARGET = ./rdb

OBJS =  bin/main.o 		\
		bin/adlist.o 	\
		bin/bitops.o 	\
		bin/db.o 		\
		bin/dict.o 		\
		bin/intset.o 	\
		bin/object.o 	\
		bin/pqsort.o 	\
		bin/quicklist.o \
		bin/sds.o 		\
		bin/server.o 	\
		bin/sort.o 		\
		bin/t_hash.o 	\
		bin/t_list.o 	\
		bin/t_set.o 	\
		bin/t_string.o 	\
		bin/t_zset.o 	\
		bin/util.o 		\
		bin/ziplist.o 	\
		bin/zmalloc.o 	\
		bin/lzf_c.o 	\
		bin/lzf_d.o 	

$(TARGET) : $(OBJS)
	$(LINK) -o $(TARGET) $(OBJS)

bin/main.o : main.cpp rdb.h
	$(CPPC) -c $(CXXFLAGS) $< -o $@

bin/adlist.o : $(RDB)adlist.c $(RDB)adlist.h $(RDB)zmalloc.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/bitops.o : $(RDB)bitops.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/db.o : $(RDB)db.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/dict.o : $(RDB)dict.c $(RDB)dict.h $(RDB)zmalloc.h rdb.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/intset.o : $(RDB)intset.c $(RDB)intset.h $(RDB)zmalloc.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/object.o : $(RDB)object.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/pqsort.o : $(RDB)pqsort.c $(RDB)pqsort.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/quicklist.o : $(RDB)quicklist.c $(RDB)quicklist.h $(RDB)zmalloc.h $(RDB)ziplist.h $(RDB)util.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/sds.o : $(RDB)sds.c $(RDB)sds.h $(RDB)sdsalloc.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/server.o : $(RDB)server.c $(RDB)server.h $(RDB)sds.h $(RDB)dict.h $(RDB)adlist.h $(RDB)zmalloc.h $(RDB)ziplist.h $(RDB)intset.h $(RDB)util.h $(RDB)quicklist.h rdb.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/sort.o : $(RDB)sort.c $(RDB)server.h $(RDB)pqsort.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/t_hash.o : $(RDB)t_hash.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/t_list.o : $(RDB)t_list.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/t_set.o : $(RDB)t_set.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/t_string.o : $(RDB)t_string.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/t_zset.o : $(RDB)t_zset.c $(RDB)server.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/util.o : $(RDB)util.c $(RDB)util.h $(RDB)fmacros.h	
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/ziplist.o : $(RDB)ziplist.c $(RDB)ziplist.h $(RDB)util.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/zmalloc.o : $(RDB)zmalloc.c $(RDB)zmalloc.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/lzf_c.o : $(RDB)lzf_c.c $(RDB)lzf.h $(RDB)lzfP.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/lzf_d.o : $(RDB)lzf_d.c $(RDB)lzf.h $(RDB)lzfP.h
	$(CC) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
