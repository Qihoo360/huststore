#include <iostream>
#include "lib/global.h"

int main ( int argc, char *argv[] )
{
    std::cout << "finish make" << std::endl;
    const char *dir 		= "/data/tmp/logs/";
    const char *status_dir 	= "/data/tmp/status/";
    const char *auth 		= "/data/tmp/auth";

    init (dir, status_dir, auth, 3, 4);

    while ( 1 )
    {
        sleep (5);
    }
    return 0;
}
