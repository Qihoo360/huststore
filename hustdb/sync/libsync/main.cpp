#include <iostream>
#include "lib/global.h"

int main ( int argc, char *argv[] )
{
    const char *dir 		= "/data/tmp/logs/";
    const char *status_dir 	= "/data/tmp/status/";
    const char *auth 		= "/data/tmp/auth";
    int pool_size           = 4;
    int release_interval    = 5;
    int checkdb_alive_interval    = 5;
    int gen_log_interval    = 60;

    init (dir, status_dir, auth, pool_size, release_interval, checkdb_alive_interval, gen_log_interval);

    while ( 1 )
    {
        sleep (5);
    }
    return 0;
}
