CC = g++
CXXFLAGS = -O0 -g3 -Wall -fmessage-length=0
LINK = $(CC) -lpthread -lcurl
TARGET = sync
BIN = bin/
SYNC_SRC = src/
OBJS = $(BIN)main.o \
	$(BIN)c_base64.o \
	$(BIN)cond.o \
	$(BIN)mutex.o	\
	$(BIN)task.o \
	$(BIN)http.o \
	$(BIN)husthttp.o \
	$(BIN)message.o \
	$(BIN)item.o \
	$(BIN)file.o \
	$(BIN)monitor.o \
	$(BIN)read_log.o \
	$(BIN)timer.o \
	$(BIN)check_backend.o \
	$(BIN)sync.o \
	$(BIN)threadpool.o \
	$(BIN)sync_status.o \
	$(BIN)json_serialization.o

INCS = -I include/rapidjson \
	-I include

DEPS = include/rapidjson/rapidjson.h

$(TARGET):	$(OBJS)

	$(LINK) -o $(TARGET) $(OBJS)


$(BIN)main.o:	$(DEPS) main.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)main.o main.cpp

$(BIN)c_base64.o:	$(DEPS) $(SYNC_SRC)c_base64.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)c_base64.o $(SYNC_SRC)c_base64.cpp

$(BIN)cond.o:	$(DEPS) $(SYNC_SRC)cond.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)cond.o $(SYNC_SRC)cond.cpp

$(BIN)mutex.o:	$(DEPS) $(SYNC_SRC)mutex.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)mutex.o $(SYNC_SRC)mutex.cpp

$(BIN)task.o:	$(DEPS) $(SYNC_SRC)task.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)task.o $(SYNC_SRC)task.cpp

$(BIN)http.o:	$(DEPS) $(SYNC_SRC)http.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)http.o $(SYNC_SRC)http.cpp

$(BIN)husthttp.o:	$(DEPS) $(SYNC_SRC)husthttp.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)husthttp.o $(SYNC_SRC)husthttp.cpp

$(BIN)message.o:	$(DEPS) $(SYNC_SRC)message.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)message.o $(SYNC_SRC)message.cpp

$(BIN)item.o:	$(DEPS) $(SYNC_SRC)item.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)item.o $(SYNC_SRC)item.cpp

$(BIN)file.o:	$(DEPS) $(SYNC_SRC)file.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)file.o $(SYNC_SRC)file.cpp

$(BIN)monitor.o:	$(DEPS) $(SYNC_SRC)monitor.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)monitor.o $(SYNC_SRC)monitor.cpp

$(BIN)read_log.o:	$(DEPS) $(SYNC_SRC)read_log.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)read_log.o $(SYNC_SRC)read_log.cpp

$(BIN)timer.o:	$(DEPS) $(SYNC_SRC)timer.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)timer.o $(SYNC_SRC)timer.cpp

$(BIN)check_backend.o:	$(DEPS) $(SYNC_SRC)check_backend.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)check_backend.o $(SYNC_SRC)check_backend.cpp

$(BIN)sync.o:	$(DEPS) $(SYNC_SRC)sync.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync.o $(SYNC_SRC)sync.cpp

$(BIN)threadpool.o:	$(DEPS) $(SYNC_SRC)threadpool.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)threadpool.o $(SYNC_SRC)threadpool.cpp

$(BIN)sync_status.o:	$(DEPS) $(SYNC_SRC)sync_status.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_status.o $(SYNC_SRC)sync_status.cpp

JOS_CPP = include/json_serialization.cpp
$(BIN)json_serialization.o:	$(DEPS) $(JOS_CPP)
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)json_serialization.o $(JOS_CPP)

all:	$(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)

