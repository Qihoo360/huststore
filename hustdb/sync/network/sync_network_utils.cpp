#include "sync_network_utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace sync_network {

void add_uniq_sort (unsigned long int, unsigned int end, ip_allow_t * ip_allow_map)
{
    int i = 0;
    int pos = - 1;

    for ( i = 0; i < ip_allow_map->size; i ++ )
    {
        if ( start == ip_allow_map->ip_map[ i ].start )
        {
            if ( end > ip_allow_map->ip_map[ i ].end )
            {
                ip_allow_map->ip_map[ i ].end = end;
            }

            return;
        }

        if ( start < ip_allow_map->ip_map[ i ].start )
        {
            pos = i;
            break;
        }
    }

    if ( pos < 0 )
    {
        ip_allow_map->ip_map[ ip_allow_map->size ].start = start;
        ip_allow_map->ip_map[ ip_allow_map->size ].end = end;
        ip_allow_map->size ++;
        return;
    }

    for ( i = ip_allow_map->size; i > pos; i -- )
    {
        ip_allow_map->ip_map[ i ] = ip_allow_map->ip_map[ i - 1 ];
    }

    ip_allow_map->ip_map[ pos ].start = start;
    ip_allow_map->ip_map[ pos ].end = end;
    ip_allow_map->size ++;
}

bool get_ip_allow_map(const char * ip_allow_string, unsigned int ip_allow_string_length, ip_allow_t * ip_allow_map)
{
    unsigned int i = 0;
    int pos = 0;
    char flag = ',';
    unsigned int prev_ip = 0;
    unsigned int cur_ip = 0;
    char ip_string [16] = { };

    if ( ! ip_allow_string || ip_allow_string_length <= 0 || ip_allow_string_length > 8192 )
    {
        return false;
    }

    for ( i = 0; i < ip_allow_string_length; i ++ )
    {
        if ( ! ( ( ip_allow_string[i] >= '0' ) && ( ip_allow_string[i] <= '9' ) ) && ip_allow_string[i] != '-' && ip_allow_string[i] != '.' && ip_allow_string[i] != ',' )
        {
            return false;
        }
    }

    for ( i = 0; i < ip_allow_string_length && ip_allow_map->size < 1023; i ++ )
    {
        if ( ip_allow_string[ i ] == ',' || ip_allow_string[ i ] == '-' )
        {
            memset (ip_string, 0, sizeof ( ip_string ));
            memcpy (ip_string, ip_allow_string + pos, i - pos);

            cur_ip = ntohl (inet_addr (ip_string));

            if ( flag == ',' && ip_allow_string[ i ] == ',' )
            {
                add_uniq_sort (cur_ip, cur_ip, ip_allow_map);
            }
            else if ( flag == '-' )
            {
                add_uniq_sort (prev_ip, cur_ip, ip_allow_map);
            }

            pos = i + 1;
            flag = ip_allow_string[ i ];
            prev_ip = cur_ip;
        }
    }

    memset (ip_string, 0, sizeof ( ip_string ));
    memcpy (ip_string, ip_allow_string + pos, i - pos);

    cur_ip = ntohl (inet_addr (ip_string));

    if ( flag == ',' )
    {
        add_uniq_sort (cur_ip, cur_ip, ip_allow_map);
    }
    else if ( flag == '-' )
    {
        add_uniq_sort (prev_ip, cur_ip, ip_allow_map);
    }

    return true;
}

bool can_access (struct sockaddr_in * addr, ip_allow_t * ip_allow_map)
{
    unsigned long ip = ntohl (addr->sin_addr.s_addr);

    int low = 0;
    int mid = 0;
    int high = ip_allow_map->size - 1;
    while ( low <= high )
    {
        mid = ( low + high ) / 2;
        if ( ip_allow_map->ip_map[ mid ].start <= ip && ip_allow_map->ip_map[ mid ].end >= ip )
        {
            return true;
        }
        else if ( ip_allow_map->ip_map[ mid ].start > ip )
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }

    return false;
}

}
