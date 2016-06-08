#ifndef __server_utils_20160608190634_h__
#define __server_utils_20160608190634_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <libgen.h>

#include <evhtp.h>
#include "module/hustdb.h"
#include "network/hustdb_network.h"

std::string get_abs_path(const char * file);
bool enable_start_server(const std::string& pid_file);
bool register_quit_signal();
bool send_quit_signal(const char * pid_file);
bool run_server(const std::string& pid_file);

#endif // __server_utils_20160608190634_h__
