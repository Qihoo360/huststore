CXX		= g++
LINK 	= $(CXX) -lpthread -lssl -lcrypto -lrt -lz -lidn
CXXFLAGS = -O0 -g3 -Wall -fmessage-length=0 

BIN		= ./o/
BINLOG_SRC = ./

CURL_INCLUDE = -I $(BINLOG_SRC)curl-7.45.0/include/

CURL_LIB = $(BINLOG_SRC)curl-7.45.0/lib/.libs/libcurl.a


BINLOG_OBJS =	$(BIN)main.o 		\
				$(BIN)binlog.o 		\
				$(BIN)condition.o 	\
				$(BIN)host_info.o 	\
				$(BIN)http.o 		\
				$(BIN)husthttp.o 	\
				$(BIN)mutex.o 		\
				$(BIN)queue.o 		\
				$(BIN)task.o 		\
				$(BIN)thread.o 		\
				$(BIN)thread_pool.o 
TARGET = binlog

$(TARGET): $(BINLOG_OBJS)
	$(LINK) -o $(TARGET) $(BINLOG_OBJS) $(CURL_LIB)


$(BIN)main.o: main.cpp $(BINLOG_SRC)binlog.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)binlog.o: $(BINLOG_SRC)binlog.cpp $(BINLOG_SRC)binlog.h $(BINLOG_SRC)singleton.h $(BINLOG_SRC)thread_pool.h $(BINLOG_SRC)host_info.h $(BINLOG_SRC)task.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)condition.o: $(BINLOG_SRC)condition.cpp $(BINLOG_SRC)condition.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)host_info.o: $(BINLOG_SRC)host_info.cpp $(BINLOG_SRC)host_info.h $(BINLOG_SRC)queue.h $(BINLOG_SRC)task.h $(BINLOG_SRC)thread_pool.h $(BINLOG_SRC)husthttp.h $(BINLOG_SRC)thread.h $(BINLOG_SRC)singleton.h $(BINLOG_SRC)atomic.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)http.o: $(BINLOG_SRC)http.cpp $(BINLOG_SRC)http.h
	$(CXX) -c $(CXXFLAGS) $(CURL_INCLUDE) -o $@ $<

$(BIN)husthttp.o: $(BINLOG_SRC)husthttp.cpp $(BINLOG_SRC)husthttp.h $(BINLOG_SRC)http.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)mutex.o: $(BINLOG_SRC)mutex.cpp $(BINLOG_SRC)mutex.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)queue.o: $(BINLOG_SRC)queue.cpp $(BINLOG_SRC)queue.h $(BINLOG_SRC)task.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)task.o: $(BINLOG_SRC)task.cpp $(BINLOG_SRC)task.h $(BINLOG_SRC)husthttp.h $(BINLOG_SRC)singleton.h $(BINLOG_SRC)host_info.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)thread.o: $(BINLOG_SRC)thread.cpp $(BINLOG_SRC)thread.h 
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BIN)thread_pool.o: $(BINLOG_SRC)thread_pool.cpp $(BINLOG_SRC)thread_pool.h $(BINLOG_SRC)husthttp.h $(BINLOG_SRC)task.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<


clean:
	rm -rf $(BINLOG_OBJS) $(TARGET)