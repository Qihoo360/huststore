CC = g++ -fPIC
CXXFLAGS = -O0 -g3 -Wall -fmessage-length=0
LINK = $(CC) -lpthread -lcurl -shared
TARGET = libsync.so
OBJS = bin/c_base64.o	\
	   bin/cond.o	\
	   bin/mutex.o	\
	   bin/task.o	\
	   bin/http.o	\
	   bin/husthttp.o	\
	   bin/message.o	\
	   bin/item.o	\
	   bin/file.o	\
	   bin/monitor.o	\
	   bin/read_log.o	\
	   bin/timer.o 		\
	   bin/check_backend.o	\
	   bin/sync.o	\
	   bin/threadpool.o	\
	   bin/status_serialization.o \
	   bin/json_serialization.o \
	   bin/json_reader.o 	\
	   bin/json_value.o 	\
	   bin/json_writer.o 	\

INCS = -I json_src/include \
	-I lib
DEPS = json_src/include/json/json.h

$(TARGET): $(OBJS)
	$(LINK) -o $(TARGET) $(OBJS)

bin/c_base64.o: src/c_base64.c lib/c_base64.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/cond.o:	src/cond.cpp lib/cond.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/mutex.o: src/mutex.cpp lib/mutex.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/task.o:	src/task.cpp lib/task.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/http.o:	src/http.c lib/http.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/husthttp.o: src/husthttp.cpp lib/husthttp.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/message.o:	src/message.cpp lib/message.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/item.o:	src/item.cpp lib/item.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/file.o:	src/file.cpp lib/file.h
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/monitor.o: src/monitor.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/read_log.o: src/read_log.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/timer.o:	src/timer.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/check_backend.o: src/check_backend.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/sync.o: src/sync.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@
bin/threadpool.o: src/threadpool.cpp lib/threadpool.h
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/status_serialization.o: src/status_serialization.cpp $(DEPS)
	$(CC) -c $(CXXFLAGS) $< -o $@

bin/json_serialization.o:	$(DEPS) \
	src/json_serialization.cpp
	$(CC) -c $(CXXFLAGS)  $(INCS) \
		-o bin/json_serialization.o \
		src/json_serialization.cpp

JSON_READER = json_src/src/lib_json/json_reader.cpp
bin/json_reader.o:	$(DEPS) \
	$(JSON_READER)
	$(CC) -c $(CXXFLAGS)  $(INCS) \
		-o bin/json_reader.o \
		$(JSON_READER)
	
JSON_VALUE = json_src/src/lib_json/json_value.cpp
bin/json_value.o:	$(DEPS) \
	$(JSON_VALUE)
	$(CC) -c $(CXXFLAGS)  $(INCS) \
		-o bin/json_value.o \
		$(JSON_VALUE)
		
JSON_WRITER = json_src/src/lib_json/json_writer.cpp
bin/json_writer.o:	$(DEPS) \
	$(JSON_WRITER)
	$(CC) -c $(CXXFLAGS)  $(INCS) \
		-o bin/json_writer.o \
		$(JSON_WRITER)

clean:
	rm -rf $(OBJS) $(TARGET)

