DEPS_PREFIX = /opt/huststore/3rd

AM_CPPFLAGS = -O0 -g3 -Wall -fmessage-length=0
AM_LDFLAGS  = -L $(DEPS_PREFIX)/lib -L /usr/local/lib -L /usr/lib64 -L /usr/lib
AM_LDFLAGS += -levent -levent_pthreads -levent_openssl -lssl -lcrypto -levhtp -ldl -lrt -lpthread -lzlog -lz -lidn

AM_CPPFLAGS += \
	-I $(DEPS_PREFIX)/include/ \
	-I $(DEPS_PREFIX)/include/evhtp \
	-I $(DEPS_PREFIX)/include/event2 \
	-I $(DEPS_PREFIX)/include/curl \
	-I $(top_srcdir)/include/ \
	-I $(top_srcdir)/include/rapidjson


STATIC_LIBS = \
		$(DEPS_PREFIX)/lib/libcurl.a \
		$(DEPS_PREFIX)/lib/libevhtp.a



bin_PROGRAMS    = hustdbsync
hustdbsync_LDFLAGS  = $(AM_LDFLAGS)
hustdbsync_LDADD = $(STATIC_LIBS)
hustdbsync_SOURCES = \
		$(top_srcdir)/include/rapidjson/rapidjson.h \
		$(top_srcdir)/main.cpp \
		$(top_srcdir)/sync_server.cpp \
		$(top_srcdir)/module/c_base64.cpp \
		$(top_srcdir)/module/cond.cpp \
		$(top_srcdir)/module/mutex.cpp \
		$(top_srcdir)/module/task.cpp \
		$(top_srcdir)/module/http.cpp \
		$(top_srcdir)/module/husthttp.cpp \
		$(top_srcdir)/module/message.cpp \
		$(top_srcdir)/module/item.cpp \
		$(top_srcdir)/module/file.cpp \
		$(top_srcdir)/module/monitor.cpp \
		$(top_srcdir)/module/read_log.cpp \
		$(top_srcdir)/module/timer.cpp \
		$(top_srcdir)/module/check_backend.cpp \
		$(top_srcdir)/module/sync.cpp \
		$(top_srcdir)/module/threadpool.cpp \
		$(top_srcdir)/module/sync_status.cpp \
		$(top_srcdir)/module/sync_conf.cpp \
		$(top_srcdir)/include/json_serialization.cpp \
		$(top_srcdir)/network/sync_network.cpp \
		$(top_srcdir)/network/sync_handler_frame.cpp \
		$(top_srcdir)/network/sync_handler.cpp \
		$(top_srcdir)/network/sync_handler_def.cpp \
		$(top_srcdir)/network/sync_network_utils.cpp \
		$(top_srcdir)/network/libevhtp_utils.cpp



confdir = $(prefix)/
conf_DATA = module/sync_server.json module/zlog.conf





