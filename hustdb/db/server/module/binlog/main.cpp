#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstdlib>

#include "binlog.h"

void callback_func ( void * param )
{
    std::cout << "cb param: " << * ( int * ) param << std::endl;
}

int main()
{
    std::string user ( "huststore" );
    std::string passwd ( "huststore" );
    {
        binlog_t log ( user, passwd );

        if ( !log.init ( 2, 100, 100 ) ) {
            std::cout << "init error" << std::endl;
        }

        log.set_callback ( callback_func );

        std::string host ( "s34.white.shht.qihoo.net:9998" );
        log.add_host ( host );

        host.assign ( "s35.white.shht.qihoo.net:9998" );
        log.add_host ( host );

        std::string table ( "test_binlog" );
        std::string key ( "name" );
        std::string value ( "abc" );
        uint32_t ver = 123;
        uint32_t ttl = 3;
        uint64_t score = 423;
        int8_t opt = 2;
        uint8_t cmd_type = 7;

        sleep ( 5 );

        std::vector<std::string> alive_hosts;
        log.get_alive_hosts ( alive_hosts );

        std::cout << alive_hosts.size() << std::endl;
        alive_hosts.clear();

        int * param = ( int * ) malloc ( sizeof ( int ) );
        *param = 10;
        /*
        if(log.add_task(
            host.c_str(),
            host.size(),
            table.c_str(),
            table.size(),
            key.c_str(),
            key.size(),
            value.c_str(),
            value.size(),
            ver,
            ttl,
            score,
            opt,
            cmd_type,
            (void *)param)){
            std::cout << "success" << std::endl;
        } else {
            std::cout << "fail" << std::endl;
        }
        */
        sleep ( 3 );
        log.get_alive_hosts ( alive_hosts );
        std::cout << alive_hosts.size() << std::endl;
    }

    return 0;
}