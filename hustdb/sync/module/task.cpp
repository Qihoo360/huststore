#include "task.h"
#include "husthttp.h"
#include <assert.h>
#include <cstdio>

#include <string>

time_t current_time;
extern char passwd[256];

static bool do_sync ( husthttp_t *, Item * );
static int construct_item ( Message *, Item * );
static uint32_t fast_crc32 ( const char *, int );
static bool url_encode_all ( const char *, int, char *, int * );

Task::Task ( bool (*fn_ptr )( void * ) , void *args )	\
 : _fn_ptr ( fn_ptr ), _args ( args )
{
}

Task::~ Task ( )
{
}

bool Task::operator() ( void *param )
{
    Message *msg = ( Message* ) _args;
    Item *item = new Item (msg->get_pos (), ( File * ) msg->get_file ());

    int item_success = construct_item (msg, item);
    switch ( item_success )
    {
        case 0:
            break;
        case - 1:
            delete item;
            return false;
        case 1:
            delete msg;
            item->get_file ()->set_bitmap (item->get_bitmap_index ());
            delete item;
            return true;
    }

    husthttp_t *client = ( husthttp_t* ) param;
    if ( do_sync (client, item) )
    {
        delete item;
        delete msg;
        return true;
    }
    else
    {
        delete item;
        return false;
    }
}

void Task::run ( )
{
    bool success;
    success = ( *_fn_ptr )( _args );
    if ( success )
    {
        //std::cout << "task success\n" << std::endl; 
    }
}

static bool do_sync ( husthttp_t *client, Item *item )
{
    std::string body, head;
    int http_code;

    std::string method (item->get_method ());
    std::string path (item->get_path ());
    std::string query_string (item->get_query_string ());
    std::string value (item->get_value ());

    int index = item->get_file ()->get_index ();
    int passwd_size = strlen (passwd);
    std::string url (passwd, passwd_size - 1);
    url.push_back ('@');
    url.append (hosts[index]);

    client->set_host (url.c_str (), url.size ());


    if ( ! client->open2 (method, path, query_string, value, 5, 5) )
    {
        return false;
    }
    int _tmp;
    if ( ! client->process (&_tmp) )
    {
        return false;
    }
    if ( ! client->get_response (body, head, http_code) )
    {
        return false;
    }
    if ( http_code != 200 && http_code != 400 && http_code != 412 )
    {
        return false;
    }
    item->get_file ()->set_bitmap (item->get_bitmap_index ());
    return true;
}

static int construct_item ( Message *msg, Item *item )
{
    const char *data = msg->get_data ().c_str ();
    size_t len = msg->get_data ().size ();
    c_str_t enc_src = { len, ( uchar_t * ) data };
    uchar_t dec_buf[2 * len];
    memset (dec_buf, 0, 2 * len);
    c_str_t dec_str = { 0, dec_buf };
    if ( ! c_decode_base64 (&enc_src, &dec_str) )
    {
        return - 1;
    }


    uint32_t head_len;
    uint8_t method;
    uint32_t ver;
    uint32_t key_len;
    const char *key;
    uint32_t key_crc;
    uint32_t ttl;
    uint32_t tb_len;
    const char *tb;
    uint32_t tb_crc;
    uint64_t score;
    int8_t opt;

    typedef struct
    {
        const void *data;
        size_t len;
    } head_item_t;

    typedef struct
    {
        const char *data;
        uint32_t *len;
    } key_tb_t;

    head_item_t heads[] = {
        {&head_len,  sizeof (head_len ) },
        {&method,    sizeof (method ) },
        {&ver,       sizeof (ver ) },
        {&key_len,   sizeof (key_len ) },
        {NULL,       0 },
        {&key_crc,   sizeof (key_crc ) },
        {&ttl,       sizeof (ttl ) },
        {&tb_len,     sizeof (tb_len ) },
        {NULL,       0 },
        {&tb_crc,    sizeof (tb_crc ) },
        {&score,     sizeof (score ) },
        {&opt,       sizeof (opt ) }
    };

    key_tb_t kt[] = {
        {NULL, &key_len },
        {NULL, &tb_len }
    };

    size_t size = sizeof (heads ) / sizeof (head_item_t );

    const char *pos = ( const char * ) dec_str.data;
    size_t binlog_len = ( size_t ) dec_str.len;

    for ( size_t i = 0, j = 0; i < size; i ++ )
    {
        if ( ! heads[i].data )
        {
            kt[j].data = pos;
            pos += * ( kt[j].len );
            j ++;
        }
        else
        {
            memcpy (( void* ) heads[i].data, pos, heads[i].len);
            pos += heads[i].len;
        }
    }

    if ( ttl != 0 )
    {
        if ( ttl > current_time )
        {
            ttl -= current_time;
        }
        else
        {
            return 1;
        }
    }

    key = kt[0].data;
    if ( fast_crc32 (key, key_len) != key_crc )
    {
        return - 1;
    }
    if ( tb_len != 0 )
    {
        tb = kt[1].data;
        if ( fast_crc32 (tb, tb_len) != tb_crc )
        {
            return - 1;
        }
    }


    char *value;
    uint32_t value_len;

    if ( method == HUSTDB_METHOD_PUT ||
         method == HUSTDB_METHOD_HSET ||
         method == HUSTDB_METHOD_TB_PUT ||
         method == HUSTDB_METHOD_TB_UPDATE )
    {

        std::string _method ("POST");
        item->set_method (_method);
        value_len = binlog_len - head_len;
        if ( value_len < 0 )
            return - 1;
        value = ( char * ) dec_str.data + head_len;
        std::string _value (value, value_len);
        item->set_value (_value);
    }
    else
    {
        std::string _method ("GET");
        item->set_method (_method);
    }

    int key_safe_len = 3 * key_len + 1;
    char key_safe[key_safe_len];
    if ( ! url_encode_all (key, key_len, key_safe, &key_safe_len) )
        return - 1;

    std::string _path;
    std::string tb_str (tb, tb_len);
    char query_string[key_safe_len + 50];
    switch ( method )
    {
        case HUSTDB_METHOD_PUT:
            _path = "/hustdb/put";
            sprintf (query_string, "key=%s&ver=%u&ttl=%u&is_dup=true", key_safe, ver, ttl);
            break;
        case HUSTDB_METHOD_HSET:
            _path = "/hustdb/hset";
            sprintf (query_string, "key=%s&ver=%u&ttl=%u&tb=%s&is_dup=true", key_safe, ver, ttl, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_SADD:
            _path = "/hustdb/sadd";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_ZADD:
            _path = "/hustdb/zadd";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&score=%ld&opt=%d&is_dup=true", key_safe, ver, tb_str.c_str (), score, opt);
            break;
        case HUSTDB_METHOD_TB_PUT:
            _path = "/husttb/put";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_TB_UPDATE:
            _path = "/husttb/update";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_DEL:
            _path = "/hustdb/del";
            sprintf (query_string, "key=%s&ver=%u&is_dup=true", key_safe, ver);
            break;
        case HUSTDB_METHOD_HDEL:
            _path = "/hustdb/hdel";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_SREM:
            _path = "/hustdb/srem";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_ZREM:
            _path = "/hustdb/zrem";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        case HUSTDB_METHOD_TB_DELETE:
            _path = "/husttb/delete";
            sprintf (query_string, "key=%s&ver=%u&tb=%s&is_dup=true", key_safe, ver, tb_str.c_str ());
            break;
        default:
            return - 1;
    }
    item->set_path (_path);
    std::string query (query_string);
    item->set_query_string (query);

    return 0;
}

static uint32_t fast_crc32 ( const char *data, int len )
{
    uint32_t sum;
    for ( sum = 0; len; len -- )
    {
        sum = sum >> 1 | sum << 31;
        sum += * data ++;
    }
    return sum;
}

static bool url_encode_all ( const char *src, int src_len, char *dst, int *dst_len )
{

    typedef struct url_encode_item_t
    {
        int             len;
        const char *    ptr;
    } url_encode_item_t;

    static const url_encode_item_t tables[] = {
        { 3, "%00" },	// 0x00
        { 3, "%01" },	// 0x01
        { 3, "%02" },	// 0x02
        { 3, "%03" },	// 0x03
        { 3, "%04" },	// 0x04
        { 3, "%05" },	// 0x05
        { 3, "%06" },	// 0x06
        { 3, "%07" },	// 0x07
        { 3, "%08" },	// 0x08
        { 3, "%09" },	// 0x09
        { 3, "%0A" },	// 0x0A
        { 3, "%0B" },	// 0x0B
        { 3, "%0C" },	// 0x0C
        { 3, "%0D" },	// 0x0D
        { 3, "%0E" },	// 0x0E
        { 3, "%0F" },	// 0x0F
        { 3, "%10" },	// 0x10
        { 3, "%11" },	// 0x11
        { 3, "%12" },	// 0x12
        { 3, "%13" },	// 0x13
        { 3, "%14" },	// 0x14
        { 3, "%15" },	// 0x15
        { 3, "%16" },	// 0x16
        { 3, "%17" },	// 0x17
        { 3, "%18" },	// 0x18
        { 3, "%19" },	// 0x19
        { 3, "%1A" },	// 0x1A
        { 3, "%1B" },	// 0x1B
        { 3, "%1C" },	// 0x1C
        { 3, "%1D" },	// 0x1D
        { 3, "%1E" },	// 0x1E
        { 3, "%1F" },	// 0x1F
        { 3, "%20" },	// SPACE
        { 3, "%21" },	// 0x21   !
        { 3, "%22" },	// "
        { 3, "%23" },	// 0x23   #
        { 3, "%24" },	// 0x24   $
        { 3, "%25" },	// 0x25   %
        { 3, "%26" },	// 0x26   &
        { 1, "'  " },	// 0x27
        { 1, "(  " },	// 0x28
        { 1, ")  " },	// 0x29
        { 3, "%2A" },	// 0x2A   *
        { 3, "%2B" },	// +
        { 1, ",  " },	// 0x2C
        { 1, "-  " },	// 0x2D
        { 1, ".  " },	// 0x2E
        { 3, "%2F" },	// 0x2F   /
        { 1, "0  " },	// 0x30
        { 1, "1  " },	// 0x31
        { 1, "2  " },	// 0x32
        { 1, "3  " },	// 0x33
        { 1, "4  " },	// 0x34
        { 1, "5  " },	// 0x35
        { 1, "6  " },	// 0x36
        { 1, "7  " },	// 0x37
        { 1, "8  " },	// 0x38
        { 1, "9  " },	// 0x39
        { 3, "%3A" },	// 0x3A   :
        { 3, "%3B" },	// 0x3B   ;
        { 3, "%3C" },	// <
        { 3, "%3D" },	// 0x3D   =
        { 3, "%3E" },	// >
        { 3, "%3F" },	// 0x3F   ?
        { 3, "%40" },	// 0x40   @
        { 1, "A  " },	// 0x41
        { 1, "B  " },	// 0x42
        { 1, "C  " },	// 0x43
        { 1, "D  " },	// 0x44
        { 1, "E  " },	// 0x45
        { 1, "F  " },	// 0x46
        { 1, "G  " },	// 0x47
        { 1, "H  " },	// 0x48
        { 1, "I  " },	// 0x49
        { 1, "J  " },	// 0x4A
        { 1, "K  " },	// 0x4B
        { 1, "L  " },	// 0x4C
        { 1, "M  " },	// 0x4D
        { 1, "N  " },	// 0x4E
        { 1, "O  " },	// 0x4F
        { 1, "P  " },	// 0x50
        { 1, "Q  " },	// 0x51
        { 1, "R  " },	// 0x52
        { 1, "S  " },	// 0x53
        { 1, "T  " },	// 0x54
        { 1, "U  " },	// 0x55
        { 1, "V  " },	// 0x56
        { 1, "W  " },	// 0x57
        { 1, "X  " },	// 0x58
        { 1, "Y  " },	// 0x59
        { 1, "Z  " },	// 0x5A
        { 1, "[  " },	// 0x5B
        { 3, "%5C" },	// '\'
        { 1, "]  " },	// 0x5D
        { 3, "%5E" },	// ^
        { 1, "_  " },	// 0x5F
        { 3, "%60" },	// `
        { 1, "a  " },	// 0x61
        { 1, "b  " },	// 0x62
        { 1, "c  " },	// 0x63
        { 1, "d  " },	// 0x64
        { 1, "e  " },	// 0x65
        { 1, "f  " },	// 0x66
        { 1, "g  " },	// 0x67
        { 1, "h  " },	// 0x68
        { 1, "i  " },	// 0x69
        { 1, "j  " },	// 0x6A
        { 1, "k  " },	// 0x6B
        { 1, "l  " },	// 0x6C
        { 1, "m  " },	// 0x6D
        { 1, "n  " },	// 0x6E
        { 1, "o  " },	// 0x6F
        { 1, "p  " },	// 0x70
        { 1, "q  " },	// 0x71
        { 1, "r  " },	// 0x72
        { 1, "s  " },	// 0x73
        { 1, "t  " },	// 0x74
        { 1, "u  " },	// 0x75
        { 1, "v  " },	// 0x76
        { 1, "w  " },	// 0x77
        { 1, "x  " },	// 0x78
        { 1, "y  " },	// 0x79
        { 1, "z  " },	// 0x7A
        { 3, "%7B" },	// {
        { 1, "|  " },	// 0x7C
        { 3, "%7D" },	// }
        { 3, "%7E" },	// ~
        { 3, "%7F" },	// 0x7F
        { 3, "%80" },	// 0x80
        { 3, "%81" },	// 0x81
        { 3, "%82" },	// 0x82
        { 3, "%83" },	// 0x83
        { 3, "%84" },	// 0x84
        { 3, "%85" },	// 0x85
        { 3, "%86" },	// 0x86
        { 3, "%87" },	// 0x87
        { 3, "%88" },	// 0x88
        { 3, "%89" },	// 0x89
        { 3, "%8A" },	// 0x8A
        { 3, "%8B" },	// 0x8B
        { 3, "%8C" },	// 0x8C
        { 3, "%8D" },	// 0x8D
        { 3, "%8E" },	// 0x8E
        { 3, "%8F" },	// 0x8F
        { 3, "%90" },	// 0x90
        { 3, "%91" },	// 0x91
        { 3, "%92" },	// 0x92
        { 3, "%93" },	// 0x93
        { 3, "%94" },	// 0x94
        { 3, "%95" },	// 0x95
        { 3, "%96" },	// 0x96
        { 3, "%97" },	// 0x97
        { 3, "%98" },	// 0x98
        { 3, "%99" },	// 0x99
        { 3, "%9A" },	// 0x9A
        { 3, "%9B" },	// 0x9B
        { 3, "%9C" },	// 0x9C
        { 3, "%9D" },	// 0x9D
        { 3, "%9E" },	// 0x9E
        { 3, "%9F" },	// 0x9F
        { 3, "%A0" },	// 0xA0
        { 3, "%A1" },	// 0xA1
        { 3, "%A2" },	// 0xA2
        { 3, "%A3" },	// 0xA3
        { 3, "%A4" },	// 0xA4
        { 3, "%A5" },	// 0xA5
        { 3, "%A6" },	// 0xA6
        { 3, "%A7" },	// 0xA7
        { 3, "%A8" },	// 0xA8
        { 3, "%A9" },	// 0xA9
        { 3, "%AA" },	// 0xAA
        { 3, "%AB" },	// 0xAB
        { 3, "%AC" },	// 0xAC
        { 3, "%AD" },	// 0xAD
        { 3, "%AE" },	// 0xAE
        { 3, "%AF" },	// 0xAF
        { 3, "%B0" },	// 0xB0
        { 3, "%B1" },	// 0xB1
        { 3, "%B2" },	// 0xB2
        { 3, "%B3" },	// 0xB3
        { 3, "%B4" },	// 0xB4
        { 3, "%B5" },	// 0xB5
        { 3, "%B6" },	// 0xB6
        { 3, "%B7" },	// 0xB7
        { 3, "%B8" },	// 0xB8
        { 3, "%B9" },	// 0xB9
        { 3, "%BA" },	// 0xBA
        { 3, "%BB" },	// 0xBB
        { 3, "%BC" },	// 0xBC
        { 3, "%BD" },	// 0xBD
        { 3, "%BE" },	// 0xBE
        { 3, "%BF" },	// 0xBF
        { 3, "%C0" },	// 0xC0
        { 3, "%C1" },	// 0xC1
        { 3, "%C2" },	// 0xC2
        { 3, "%C3" },	// 0xC3
        { 3, "%C4" },	// 0xC4
        { 3, "%C5" },	// 0xC5
        { 3, "%C6" },	// 0xC6
        { 3, "%C7" },	// 0xC7
        { 3, "%C8" },	// 0xC8
        { 3, "%C9" },	// 0xC9
        { 3, "%CA" },	// 0xCA
        { 3, "%CB" },	// 0xCB
        { 3, "%CC" },	// 0xCC
        { 3, "%CD" },	// 0xCD
        { 3, "%CE" },	// 0xCE
        { 3, "%CF" },	// 0xCF
        { 3, "%D0" },	// 0xD0
        { 3, "%D1" },	// 0xD1
        { 3, "%D2" },	// 0xD2
        { 3, "%D3" },	// 0xD3
        { 3, "%D4" },	// 0xD4
        { 3, "%D5" },	// 0xD5
        { 3, "%D6" },	// 0xD6
        { 3, "%D7" },	// 0xD7
        { 3, "%D8" },	// 0xD8
        { 3, "%D9" },	// 0xD9
        { 3, "%DA" },	// 0xDA
        { 3, "%DB" },	// 0xDB
        { 3, "%DC" },	// 0xDC
        { 3, "%DD" },	// 0xDD
        { 3, "%DE" },	// 0xDE
        { 3, "%DF" },	// 0xDF
        { 3, "%E0" },	// 0xE0
        { 3, "%E1" },	// 0xE1
        { 3, "%E2" },	// 0xE2
        { 3, "%E3" },	// 0xE3
        { 3, "%E4" },	// 0xE4
        { 3, "%E5" },	// 0xE5
        { 3, "%E6" },	// 0xE6
        { 3, "%E7" },	// 0xE7
        { 3, "%E8" },	// 0xE8
        { 3, "%E9" },	// 0xE9
        { 3, "%EA" },	// 0xEA
        { 3, "%EB" },	// 0xEB
        { 3, "%EC" },	// 0xEC
        { 3, "%ED" },	// 0xED
        { 3, "%EE" },	// 0xEE
        { 3, "%EF" },	// 0xEF
        { 3, "%F0" },	// 0xF0
        { 3, "%F1" },	// 0xF1
        { 3, "%F2" },	// 0xF2
        { 3, "%F3" },	// 0xF3
        { 3, "%F4" },	// 0xF4
        { 3, "%F5" },	// 0xF5
        { 3, "%F6" },	// 0xF6
        { 3, "%F7" },	// 0xF7
        { 3, "%F8" },	// 0xF8
        { 3, "%F9" },	// 0xF9
        { 3, "%FA" },	// 0xFA
        { 3, "%FB" },	// 0xFB
        { 3, "%FC" },	// 0xFC
        { 3, "%FD" },	// 0xFD
        { 3, "%FE" },	// 0xFE
        { 3, "%FF" },	// 0xFF
    };

    const unsigned char *       source;
    const unsigned char *       source_end;
    unsigned char *       dest;
    const url_encode_item_t *   p;

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
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 2
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 3
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 4
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 5
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 6
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 7
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;

        // 8
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;
    }
    // 
    while ( source < source_end )
    {
        p = & tables[ * source ++ ];
        * ( ( unsigned int* ) dest ) = * ( ( unsigned int* ) p->ptr );
        dest += p->len;
    }
    * dest = 0;

    * dst_len = ( int ) ( dest - ( unsigned char * ) dst );
    return true;
}
