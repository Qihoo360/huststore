CC = g++
CXXFLAGS = -O0 -g3 -Wall -fmessage-length=0
LIBS =	-levent -levent_pthreads -levent_openssl -lssl -lcrypto -levhtp -ldl -lrt -lpthread -lcurl -lzlog
LINK = -L /usr/local/lib  \
	/usr/local/lib/libevhtp.a

BIN  = ./bin/
TARGET = hustdbsync

SERVER = ./
MODULE_SRC = $(SERVER)module/
NETWORK_SRC = $(SERVER)network/
INCLUDE = $(SERVER)include/

OBJS = $(BIN)main.o \
	$(BIN)sync_server.o \
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
	$(BIN)sync_conf.o \
	$(BIN)json_serialization.o \
	$(BIN)sync_network.o \
	$(BIN)sync_handler_frame.o \
	$(BIN)sync_handler.o \
	$(BIN)sync_handler_def.o \
	$(BIN)sync_network_utils.o \
	$(BIN)libevhtp_utils.o

INCS = -I /usr/local/include/evhtp \
	-I /usr/local/include/event2 \
	-I $(INCLUDE)rapidjson \
	-I $(INCLUDE)

DEPS = $(INCLUDE)rapidjson/rapidjson.h

SERVER_CONF = sync_server.json
LOG_CONF = zlog.conf

$(TARGET):	$(OBJS)

	$(CXX) -o $(TARGET) $(INCS) $(LINK) $(OBJS) $(LIBS)

	chmod a+x $(TARGET)
	cp $(MODULE_SRC)$(SERVER_CONF) $(SERVER_CONF)
	cp $(MODULE_SRC)$(LOG_CONF) $(LOG_CONF)

$(BIN)main.o:	$(DEPS) $(SERVER)main.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)main.o $(SERVER)main.cpp

$(BIN)sync_server.o:	$(DEPS) $(SERVER)sync_server.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_server.o $(SERVER)sync_server.cpp

$(BIN)c_base64.o:	$(DEPS) $(MODULE_SRC)c_base64.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)c_base64.o $(MODULE_SRC)c_base64.cpp

$(BIN)cond.o:	$(DEPS) $(MODULE_SRC)cond.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)cond.o $(MODULE_SRC)cond.cpp

$(BIN)mutex.o:	$(DEPS) $(MODULE_SRC)mutex.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)mutex.o $(MODULE_SRC)mutex.cpp

$(BIN)task.o:	$(DEPS) $(MODULE_SRC)task.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)task.o $(MODULE_SRC)task.cpp

$(BIN)http.o:	$(DEPS) $(MODULE_SRC)http.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)http.o $(MODULE_SRC)http.cpp

$(BIN)husthttp.o:	$(DEPS) $(MODULE_SRC)husthttp.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)husthttp.o $(MODULE_SRC)husthttp.cpp

$(BIN)message.o:	$(DEPS) $(MODULE_SRC)message.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)message.o $(MODULE_SRC)message.cpp

$(BIN)item.o:	$(DEPS) $(MODULE_SRC)item.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)item.o $(MODULE_SRC)item.cpp

$(BIN)file.o:	$(DEPS) $(MODULE_SRC)file.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)file.o $(MODULE_SRC)file.cpp

$(BIN)monitor.o:	$(DEPS) $(MODULE_SRC)monitor.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)monitor.o $(MODULE_SRC)monitor.cpp

$(BIN)read_log.o:	$(DEPS) $(MODULE_SRC)read_log.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)read_log.o $(MODULE_SRC)read_log.cpp

$(BIN)timer.o:	$(DEPS) $(MODULE_SRC)timer.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)timer.o $(MODULE_SRC)timer.cpp

$(BIN)check_backend.o:	$(DEPS) $(MODULE_SRC)check_backend.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)check_backend.o $(MODULE_SRC)check_backend.cpp

$(BIN)sync.o:	$(DEPS) $(MODULE_SRC)sync.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync.o $(MODULE_SRC)sync.cpp

$(BIN)threadpool.o:	$(DEPS) $(MODULE_SRC)threadpool.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)threadpool.o $(MODULE_SRC)threadpool.cpp

$(BIN)sync_status.o:	$(DEPS) $(MODULE_SRC)sync_status.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_status.o $(MODULE_SRC)sync_status.cpp

$(BIN)sync_conf.o:	$(DEPS) $(MODULE_SRC)sync_conf.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_conf.o $(MODULE_SRC)sync_conf.cpp

JOS_CPP = $(INCLUDE)/json_serialization.cpp
$(BIN)json_serialization.o:	$(DEPS) $(JOS_CPP)
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)json_serialization.o $(JOS_CPP)

$(BIN)sync_network.o:	$(NETWORK_SRC)sync_network.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_network.o $(NETWORK_SRC)sync_network.cpp

$(BIN)sync_handler_frame.o:	$(NETWORK_SRC)sync_handler_frame.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_handler_frame.o $(NETWORK_SRC)sync_handler_frame.cpp

$(BIN)sync_handler.o:	$(NETWORK_SRC)sync_handler.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_handler.o $(NETWORK_SRC)sync_handler.cpp

$(BIN)sync_handler_def.o:	$(NETWORK_SRC)sync_handler_def.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_handler_def.o $(NETWORK_SRC)sync_handler_def.cpp

$(BIN)sync_network_utils.o:	$(NETWORK_SRC)sync_network_utils.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)sync_network_utils.o $(NETWORK_SRC)sync_network_utils.cpp

$(BIN)libevhtp_utils.o:	$(NETWORK_SRC)libevhtp_utils.cpp
	$(CXX) -c $(CXXFLAGS)  $(INCS) -o $(BIN)libevhtp_utils.o $(NETWORK_SRC)libevhtp_utils.cpp

all:	$(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET) $(SERVER_CONF) $(LOG_CONF)

install:		$(TARGET)
	test -d '/data/hustdbsync' || mkdir -p '/data/hustdbsync'
	test -d '/data/hustdbsync/logs' || mkdir -p '/data/hustdbsync/logs'
	cp $(TARGET) '/data/hustdbsync/$(TARGET)'
	cp stop.py '/data/hustdbsync/'

	test ! -f '/data/hustdbsync/$(SERVER_CONF)' || cp '/data/hustdbsync/$(SERVER_CONF)' '/data/hustdbsync/$(SERVER_CONF).old'
	cp $(SERVER_CONF) '/data/hustdbsync/'

	test ! -f '/data/hustdbsync/$(LOG_CONF)' || cp '/data/hustdbsync/$(LOG_CONF)' '/data/hustdbsync/$(LOG_CONF).old'
	cp $(LOG_CONF) '/data/hustdbsync/'