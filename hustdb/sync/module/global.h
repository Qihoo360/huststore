#pragma once

#include <vector>
#include <deque>
#include <string>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <sys/time.h>
#include <cstdlib>
#include <map>
#include <iostream>


#define rel_time_t unsigned int


#include "threadpool.h"


extern pthread_mutex_t file_queue_mutex;

extern pthread_mutex_t release_lock;

extern std::vector<std::string> hosts;

extern std::vector<std::string> log_dirs;

extern std::vector<std::string> status_dirs;

extern std::vector<char *> total_status_addrs;

extern std::vector<int *> pipe_fds;

extern time_t current_time;

extern std::vector<std::deque<File *> >file_queue;

extern std::deque<File *>release_queue;

extern std::map<std::string, File *> file_map;

extern ThreadPool *tp;

extern char *total_status_addr;

extern char status_buf[FILE_BIT_MAX];

extern char passwd[256];

extern int release_interval;
extern int checkdb_interval;
extern int gen_log_interval;

typedef unsigned char c_bool_t;
c_bool_t init ( const char *, const char *, const char *, int, int, int, int );
c_bool_t stop_sync ( );
bool get_status (int hosts_size, std::string& resp);
