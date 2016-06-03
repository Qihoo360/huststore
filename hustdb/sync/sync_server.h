#ifndef __sync_server_20160603171532_h__
#define __sync_server_20160603171532_h__

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include "module/global.h"
#include "module/sync_conf.h"
#include "network/sync_network_utils.h"
#include "network/sync_network.h"

std::string get_abs_path(const char * file);
bool enable_start_server(const std::string& pid_file);
bool register_quit_signal();
bool send_quit_signal(const char * pid_file);
bool run_server(
    const std::string& srv_conf,
    const std::string& pid_file,
    const std::string& log_conf,
    const std::string& log_category);

#endif // __sync_server_20160603171532_h__
