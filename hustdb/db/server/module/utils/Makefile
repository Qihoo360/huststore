# Example:
# INCLUDE		= -I ./include
# LIB			= -L /usr/lib -lpthread
# STATIC_LIB	= $./lib/libxxx.a
# INCS			= $(INCLUDE)
# LIBS			= $(LIB) -ldl -lidn
CC = gcc -std=gnu99
CXX = g++
BIN = ./
INCLUDE = 
LIB = 
FLAGS = -fPIC -O0 -g3 -Wall -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
STATIC_LIB = 

OBJECTS		= $(BIN)main.o \
	$(BIN)skiplist.o \

TARGET		= ./utils_test
INCS		= $(INCLUDE)
LIBS		= $(LIB)

all: $(OBJECTS)

	$(CC) -rdynamic -o $(TARGET) $(INCS) $(OBJECTS) $(STATIC_LIB) $(LIBS)

CWD			= ./

$(BIN)main.o:	$(CWD)main.c
	$(CC) $(FLAGS) $(INCS) -c -o $@ $<

$(BIN)skiplist.o:	$(CWD)skiplist.c
	$(CC) $(FLAGS) $(INCS) -c -o $@ $<

clean:
	rm -rf $(TARGET) $(OBJECTS)

# install: $(TARGET)
# 	test -d $(DESTDIR)$(prefix) || mkdir -p		$(DESTDIR)$(prefix)
# 	install -v -m 0755 $(TARGET) $(DESTDIR)$(prefix)