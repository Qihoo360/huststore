#include <cstdio>
#include <cstring>
#include <assert.h>
#include <stdint.h>

#include "task.h"
#include "husthttp.h"
#include "singleton.h"
#include "host_info.h"

#define HUSTDB_METHOD_PUT  1
#define HUSTDB_METHOD_DEL  2
#define HUSTDB_METHOD_HSET 3
#define HUSTDB_METHOD_HDEL 4
#define HUSTDB_METHOD_SADD 5
#define HUSTDB_METHOD_SREM 6
#define HUSTDB_METHOD_ZADD 7
#define HUSTDB_METHOD_ZREM 8

task_t::task_t (
                 const char * host,
                 size_t host_len,
                 callback_func_t callback_func,
                 void * param,
                 const std::string & auth )
: _url ( auth )
, _callback_func ( callback_func )
, _cb_param ( param )
, _http_code ( 0 )
{
    _host.assign ( host, host_len );
}

task_t::~ task_t ( )
{
}

bool task_t::make_task (
                         const char * host,
                         size_t host_len,
                         const char * table,
                         size_t table_len,
                         const char * key,
                         size_t key_len,
                         const char * value,
                         size_t value_len,
                         uint32_t ver,
                         uint32_t ttl,
                         uint8_t cmd_type )
{
    if ( ! _url.empty () )
    {
        _url.append ( "@" );
    }

    _url.append ( host, host_len );

    size_t key_safe_len = 3 * key_len + 1;
    char key_safe[key_safe_len];

    if ( ! url_encode_all ( key, int ( key_len ), key_safe, ( int * ) &key_safe_len ) )
    {
        return false;
    }

    char query_string[key_safe_len + 128];

    switch ( cmd_type )
    {
        case HUSTDB_METHOD_PUT:
            _method = "POST";
            _path.assign ( "/hustdb/put" );
            sprintf ( query_string, "key=%s&ver=%u&ttl=%u&is_dup=true", key_safe, ver, ttl );
            _value.assign ( value, value_len );
            break;

        case HUSTDB_METHOD_HSET:
            _method = "POST";
            _path.assign ( "/hustdb/hset" );
            sprintf ( query_string, "key=%s&ver=%u&ttl=%u&tb=%.*s&is_dup=true", key_safe, ver, ttl, int ( table_len ), table );
            _value.assign ( value, value_len );
            break;

        case HUSTDB_METHOD_SADD:
            _method = "POST";
            _path.assign ( "/hustdb/sadd" );
            sprintf ( query_string, "ver=%u&tb=%.*s&is_dup=true", ver, int ( table_len ), table );
            _value.assign ( key, key_len );
            break;

        case HUSTDB_METHOD_ZADD:
            _method = "POST";
            _path.assign ( "/hustdb/zadd" );
            sprintf ( query_string, "ver=%u&tb=%.*s&score=%.*s&opt=0&is_dup=true", ver, int ( table_len ), table, int ( value_len ), value );
            _value.assign ( key, key_len );
            break;

        case HUSTDB_METHOD_DEL:
            _method = "GET";
            _path.assign ( "/hustdb/del" );
            sprintf ( query_string, "key=%s&ver=%u&is_dup=true", key_safe, ver );
            break;

        case HUSTDB_METHOD_HDEL:
            _method = "GET";
            _path.assign ( "/hustdb/hdel" );
            sprintf ( query_string, "key=%s&ver=%u&tb=%.*s&is_dup=true", key_safe, ver, int ( table_len ), table );
            break;

        case HUSTDB_METHOD_SREM:
            _method = "POST";
            _path.assign ( "/hustdb/srem" );
            sprintf ( query_string, "ver=%u&tb=%.*s&is_dup=true", ver, int ( table_len ), table );
            _value.assign ( key, key_len );
            break;

        case HUSTDB_METHOD_ZREM:
            _method = "POST";
            _path.assign ( "/hustdb/zrem" );
            sprintf ( query_string, "ver=%u&tb=%.*s&is_dup=true", ver, int ( table_len ), table );
            _value.assign ( key, key_len );
            break;

        default:
            return false;
    }

    _query_string.assign ( query_string );
    return true;
}

bool task_t::run ( husthttp_t * client )
{
    host_info_t & host_info = singleton_t<host_info_t>::instance ();

    if ( inner_handle ( client ) )
    {
        if ( _callback_func )
        {
            _callback_func ( _cb_param );

            if ( _cb_param != NULL )
            {
                free ( _cb_param );
                _cb_param = NULL;
            }
        }

        host_info.finish_task ( _host );
        return true;
    }
    else
    {
        if ( ! host_info.has_host ( _host ) )
        {
            return true;
        }

        host_info.add_task ( _host, this, false );
    }

    return false;
}

bool task_t::inner_handle ( husthttp_t * client )
{
    if ( ! client )
    {
        return false;
    }

    client->set_host ( _url.c_str (), _url.size () );

    if ( ! client->open2 ( _method, _path, _query_string, _value, 5, 5 ) )
    {
        return false;
    }

    int _tmp;

    if ( ! client->process ( &_tmp ) )
    {
        return false;
    }

    if ( ! client->get_response ( _body, _head, _http_code ) )
    {
        return false;
    }

    return is_success ( &_http_code );
}

bool task_t::is_success ( int * http_code )
{
    return ( *http_code == 200 || * http_code == 400 || * http_code == 412 );
}

bool task_t::url_encode_all ( const char * src, int src_len, char * dst, int * dst_len )
{

    typedef struct url_encode_item_t
    {
        int             len;
        const char   *  ptr;
    } url_encode_item_t;

    static const url_encode_item_t tables[] = {
        { 3, "%00" },   // 0x00
        { 3, "%01" },   // 0x01
        { 3, "%02" },   // 0x02
        { 3, "%03" },   // 0x03
        { 3, "%04" },   // 0x04
        { 3, "%05" },   // 0x05
        { 3, "%06" },   // 0x06
        { 3, "%07" },   // 0x07
        { 3, "%08" },   // 0x08
        { 3, "%09" },   // 0x09
        { 3, "%0A" },   // 0x0A
        { 3, "%0B" },   // 0x0B
        { 3, "%0C" },   // 0x0C
        { 3, "%0D" },   // 0x0D
        { 3, "%0E" },   // 0x0E
        { 3, "%0F" },   // 0x0F
        { 3, "%10" },   // 0x10
        { 3, "%11" },   // 0x11
        { 3, "%12" },   // 0x12
        { 3, "%13" },   // 0x13
        { 3, "%14" },   // 0x14
        { 3, "%15" },   // 0x15
        { 3, "%16" },   // 0x16
        { 3, "%17" },   // 0x17
        { 3, "%18" },   // 0x18
        { 3, "%19" },   // 0x19
        { 3, "%1A" },   // 0x1A
        { 3, "%1B" },   // 0x1B
        { 3, "%1C" },   // 0x1C
        { 3, "%1D" },   // 0x1D
        { 3, "%1E" },   // 0x1E
        { 3, "%1F" },   // 0x1F
        { 3, "%20" },   // SPACE
        { 3, "%21" },   // 0x21   !
        { 3, "%22" },   // "
        { 3, "%23" },   // 0x23   #
        { 3, "%24" },   // 0x24   $
        { 3, "%25" },   // 0x25   %
        { 3, "%26" },   // 0x26   &
        { 1, "'  " },   // 0x27
        { 1, "(  " },   // 0x28
        { 1, ")  " },   // 0x29
        { 3, "%2A" },   // 0x2A   *
        { 3, "%2B" },   // +
        { 1, ",  " },   // 0x2C
        { 1, "-  " },   // 0x2D
        { 1, ".  " },   // 0x2E
        { 3, "%2F" },   // 0x2F   /
        { 1, "0  " },   // 0x30
        { 1, "1  " },   // 0x31
        { 1, "2  " },   // 0x32
        { 1, "3  " },   // 0x33
        { 1, "4  " },   // 0x34
        { 1, "5  " },   // 0x35
        { 1, "6  " },   // 0x36
        { 1, "7  " },   // 0x37
        { 1, "8  " },   // 0x38
        { 1, "9  " },   // 0x39
        { 3, "%3A" },   // 0x3A   :
        { 3, "%3B" },   // 0x3B   ;
        { 3, "%3C" },   // <
        { 3, "%3D" },   // 0x3D   =
        { 3, "%3E" },   // >
        { 3, "%3F" },   // 0x3F   ?
        { 3, "%40" },   // 0x40   @
        { 1, "A  " },   // 0x41
        { 1, "B  " },   // 0x42
        { 1, "C  " },   // 0x43
        { 1, "D  " },   // 0x44
        { 1, "E  " },   // 0x45
        { 1, "F  " },   // 0x46
        { 1, "G  " },   // 0x47
        { 1, "H  " },   // 0x48
        { 1, "I  " },   // 0x49
        { 1, "J  " },   // 0x4A
        { 1, "K  " },   // 0x4B
        { 1, "L  " },   // 0x4C
        { 1, "M  " },   // 0x4D
        { 1, "N  " },   // 0x4E
        { 1, "O  " },   // 0x4F
        { 1, "P  " },   // 0x50
        { 1, "Q  " },   // 0x51
        { 1, "R  " },   // 0x52
        { 1, "S  " },   // 0x53
        { 1, "T  " },   // 0x54
        { 1, "U  " },   // 0x55
        { 1, "V  " },   // 0x56
        { 1, "W  " },   // 0x57
        { 1, "X  " },   // 0x58
        { 1, "Y  " },   // 0x59
        { 1, "Z  " },   // 0x5A
        { 1, "[  " },   // 0x5B
        { 3, "%5C" },   // '\'
        { 1, "]  " },   // 0x5D
        { 3, "%5E" },   // ^
        { 1, "_  " },   // 0x5F
        { 3, "%60" },   // `
        { 1, "a  " },   // 0x61
        { 1, "b  " },   // 0x62
        { 1, "c  " },   // 0x63
        { 1, "d  " },   // 0x64
        { 1, "e  " },   // 0x65
        { 1, "f  " },   // 0x66
        { 1, "g  " },   // 0x67
        { 1, "h  " },   // 0x68
        { 1, "i  " },   // 0x69
        { 1, "j  " },   // 0x6A
        { 1, "k  " },   // 0x6B
        { 1, "l  " },   // 0x6C
        { 1, "m  " },   // 0x6D
        { 1, "n  " },   // 0x6E
        { 1, "o  " },   // 0x6F
        { 1, "p  " },   // 0x70
        { 1, "q  " },   // 0x71
        { 1, "r  " },   // 0x72
        { 1, "s  " },   // 0x73
        { 1, "t  " },   // 0x74
        { 1, "u  " },   // 0x75
        { 1, "v  " },   // 0x76
        { 1, "w  " },   // 0x77
        { 1, "x  " },   // 0x78
        { 1, "y  " },   // 0x79
        { 1, "z  " },   // 0x7A
        { 3, "%7B" },   // {
        { 1, "|  " },   // 0x7C
        { 3, "%7D" },   // }
        { 3, "%7E" },   // ~
        { 3, "%7F" },   // 0x7F
        { 3, "%80" },   // 0x80
        { 3, "%81" },   // 0x81
        { 3, "%82" },   // 0x82
        { 3, "%83" },   // 0x83
        { 3, "%84" },   // 0x84
        { 3, "%85" },   // 0x85
        { 3, "%86" },   // 0x86
        { 3, "%87" },   // 0x87
        { 3, "%88" },   // 0x88
        { 3, "%89" },   // 0x89
        { 3, "%8A" },   // 0x8A
        { 3, "%8B" },   // 0x8B
        { 3, "%8C" },   // 0x8C
        { 3, "%8D" },   // 0x8D
        { 3, "%8E" },   // 0x8E
        { 3, "%8F" },   // 0x8F
        { 3, "%90" },   // 0x90
        { 3, "%91" },   // 0x91
        { 3, "%92" },   // 0x92
        { 3, "%93" },   // 0x93
        { 3, "%94" },   // 0x94
        { 3, "%95" },   // 0x95
        { 3, "%96" },   // 0x96
        { 3, "%97" },   // 0x97
        { 3, "%98" },   // 0x98
        { 3, "%99" },   // 0x99
        { 3, "%9A" },   // 0x9A
        { 3, "%9B" },   // 0x9B
        { 3, "%9C" },   // 0x9C
        { 3, "%9D" },   // 0x9D
        { 3, "%9E" },   // 0x9E
        { 3, "%9F" },   // 0x9F
        { 3, "%A0" },   // 0xA0
        { 3, "%A1" },   // 0xA1
        { 3, "%A2" },   // 0xA2
        { 3, "%A3" },   // 0xA3
        { 3, "%A4" },   // 0xA4
        { 3, "%A5" },   // 0xA5
        { 3, "%A6" },   // 0xA6
        { 3, "%A7" },   // 0xA7
        { 3, "%A8" },   // 0xA8
        { 3, "%A9" },   // 0xA9
        { 3, "%AA" },   // 0xAA
        { 3, "%AB" },   // 0xAB
        { 3, "%AC" },   // 0xAC
        { 3, "%AD" },   // 0xAD
        { 3, "%AE" },   // 0xAE
        { 3, "%AF" },   // 0xAF
        { 3, "%B0" },   // 0xB0
        { 3, "%B1" },   // 0xB1
        { 3, "%B2" },   // 0xB2
        { 3, "%B3" },   // 0xB3
        { 3, "%B4" },   // 0xB4
        { 3, "%B5" },   // 0xB5
        { 3, "%B6" },   // 0xB6
        { 3, "%B7" },   // 0xB7
        { 3, "%B8" },   // 0xB8
        { 3, "%B9" },   // 0xB9
        { 3, "%BA" },   // 0xBA
        { 3, "%BB" },   // 0xBB
        { 3, "%BC" },   // 0xBC
        { 3, "%BD" },   // 0xBD
        { 3, "%BE" },   // 0xBE
        { 3, "%BF" },   // 0xBF
        { 3, "%C0" },   // 0xC0
        { 3, "%C1" },   // 0xC1
        { 3, "%C2" },   // 0xC2
        { 3, "%C3" },   // 0xC3
        { 3, "%C4" },   // 0xC4
        { 3, "%C5" },   // 0xC5
        { 3, "%C6" },   // 0xC6
        { 3, "%C7" },   // 0xC7
        { 3, "%C8" },   // 0xC8
        { 3, "%C9" },   // 0xC9
        { 3, "%CA" },   // 0xCA
        { 3, "%CB" },   // 0xCB
        { 3, "%CC" },   // 0xCC
        { 3, "%CD" },   // 0xCD
        { 3, "%CE" },   // 0xCE
        { 3, "%CF" },   // 0xCF
        { 3, "%D0" },   // 0xD0
        { 3, "%D1" },   // 0xD1
        { 3, "%D2" },   // 0xD2
        { 3, "%D3" },   // 0xD3
        { 3, "%D4" },   // 0xD4
        { 3, "%D5" },   // 0xD5
        { 3, "%D6" },   // 0xD6
        { 3, "%D7" },   // 0xD7
        { 3, "%D8" },   // 0xD8
        { 3, "%D9" },   // 0xD9
        { 3, "%DA" },   // 0xDA
        { 3, "%DB" },   // 0xDB
        { 3, "%DC" },   // 0xDC
        { 3, "%DD" },   // 0xDD
        { 3, "%DE" },   // 0xDE
        { 3, "%DF" },   // 0xDF
        { 3, "%E0" },   // 0xE0
        { 3, "%E1" },   // 0xE1
        { 3, "%E2" },   // 0xE2
        { 3, "%E3" },   // 0xE3
        { 3, "%E4" },   // 0xE4
        { 3, "%E5" },   // 0xE5
        { 3, "%E6" },   // 0xE6
        { 3, "%E7" },   // 0xE7
        { 3, "%E8" },   // 0xE8
        { 3, "%E9" },   // 0xE9
        { 3, "%EA" },   // 0xEA
        { 3, "%EB" },   // 0xEB
        { 3, "%EC" },   // 0xEC
        { 3, "%ED" },   // 0xED
        { 3, "%EE" },   // 0xEE
        { 3, "%EF" },   // 0xEF
        { 3, "%F0" },   // 0xF0
        { 3, "%F1" },   // 0xF1
        { 3, "%F2" },   // 0xF2
        { 3, "%F3" },   // 0xF3
        { 3, "%F4" },   // 0xF4
        { 3, "%F5" },   // 0xF5
        { 3, "%F6" },   // 0xF6
        { 3, "%F7" },   // 0xF7
        { 3, "%F8" },   // 0xF8
        { 3, "%F9" },   // 0xF9
        { 3, "%FA" },   // 0xFA
        { 3, "%FB" },   // 0xFB
        { 3, "%FC" },   // 0xFC
        { 3, "%FD" },   // 0xFD
        { 3, "%FE" },   // 0xFE
        { 3, "%FF" },   // 0xFF
    };

    const unsigned char    *    source;
    const unsigned char    *    source_end;
    unsigned char    *    dest;
    const url_encode_item_t  *  p;

    if ( unlikely ( NULL == src || NULL == dst || NULL == dst_len ) )
    {
        if ( dst_len )
        {
            * dst_len = 0;
        }

        return false;
    }

    if ( src_len < 0 )
    {
        src_len = ( int ) strlen ( src );
    }

    source = ( const unsigned char * ) src;
    source_end = source + src_len;

    assert ( dst && dst_len );

    if ( unlikely ( * dst_len < src_len * 3 + 1 ) )
    {
        * dst_len = 0;
        return false;
    }

    dest = ( unsigned char * ) dst;

    while ( source + 8 < source_end )
    {
        // 1
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 2
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 3
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 4
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 5
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 6
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 7
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;

        // 8
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;
    }

    //
    while ( source < source_end )
    {
        p = & tables[ * source ++ ];
        * ( ( unsigned int * ) dest ) = * ( ( unsigned int * ) p->ptr );
        dest += p->len;
    }

    * dest = 0;

    * dst_len = ( int ) ( dest - ( unsigned char * ) dst );
    return true;
}
