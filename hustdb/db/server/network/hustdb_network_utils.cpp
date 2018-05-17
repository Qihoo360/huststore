#include "hustdb_network_utils.h"
#include <zlib.h>

namespace hustdb_network {

void add_version(uint32_t ver, evhtp_request_t * request)
{
    evhtp::add_numeric_kv("Version", ver, request);
}

void add_ver_err(item_ctxt_t * ctxt, evhtp_request_t * request)
{
    evhtp::add_kv("VerError", (ctxt && ctxt->is_version_error) ? "true" : "false", request);
}

void add_ver_err(bool is_version_error, evhtp_request_t * request)
{
    evhtp::add_kv("VerError", is_version_error ? "true" : "false", request);
}

void send_write_reply(evhtp_res code, uint32_t ver, item_ctxt_t * ctxt, evhtp_request_t * request)
{
    hustdb_network::add_version(ver, request);
    hustdb_network::add_ver_err(ctxt, request);
    evhtp::send_nobody_reply(code, request);
}

void send_write_reply(evhtp_res code, uint32_t ver, bool is_version_error, evhtp_request_t * request)
{
    hustdb_network::add_version(ver, request);
    hustdb_network::add_ver_err(is_version_error, request);
    evhtp::send_nobody_reply(code, request);
}

void add_uniq_sort (unsigned int start, unsigned int end, ip_allow_t * ip_allow_map)
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
    int i = 0;
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

#if IS_LITTLE_ENDIAN
                cur_ip = ntohl (inet_addr (ip_string));
#else
                cur_ip = inet_addr (ip_string);
#endif

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

#if IS_LITTLE_ENDIAN
        cur_ip = ntohl (inet_addr (ip_string));
#else
        cur_ip = inet_addr (ip_string);
#endif

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

} // hustdb_network

namespace gzip {

struct deflate_t
{
    deflate_t(z_stream * strm);
    ~deflate_t();
    int error() { return m_err; }
private:
    z_stream * m_strm;
    int m_err;
};

deflate_t::deflate_t(z_stream * strm) : m_strm(strm)
{
    m_err = deflateInit2(m_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
}

deflate_t::~deflate_t()
{
    deflateEnd(m_strm);
}


int compress(Bytef *data, uLong ndata, Bytef *zdata, uLong nzdata)
{

    int err = 0;

    if (!data || ndata < 1)
    {
        return -1;
    }

    z_stream c_stream;
    c_stream.zalloc = (alloc_func) 0;
    c_stream.zfree = (free_func) 0;
    c_stream.opaque = (voidpf) 0;

    deflate_t def(&c_stream);
    if (def.error() != Z_OK)
    {
        return -1;
    }
    c_stream.next_in = data;
    c_stream.avail_in = ndata;
    c_stream.next_out = zdata;
    c_stream.avail_out = nzdata;
    while (c_stream.avail_in != 0 && c_stream.total_out < nzdata)
    {
        if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
        {
            return -1;
        }
    }
    if (c_stream.avail_in != 0)
    {
        return c_stream.avail_in;
    }
    for (;;)
    {
        if ((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
        {
            break;
        }
        if (err != Z_OK)
        {
            return -1;
        }
    }
    return c_stream.total_out;
}

struct inflate_t
{
    inflate_t(z_stream * strm);
    ~inflate_t();
    int error() { return m_err; }
private:
    z_stream * m_strm;
    int m_err;
};

inflate_t::inflate_t(z_stream * strm) : m_strm(strm)
{
    m_err = inflateInit2(strm, -MAX_WBITS);
}

inflate_t::~inflate_t()
{
    inflateEnd(m_strm);
}

int decompress(Byte *zdata, uLong nzdata, Byte *data, uLong ndata)
{
    int err = 0;
    z_stream d_stream = { 0 };
    static char dummy_head[2] = { 0x8 + 0x7 * 0x10, (((0x8 + 0x7 * 0x10)
        * 0x100 + 30) / 31 * 31) & 0xFF, };
    d_stream.zalloc = (alloc_func) 0;
    d_stream.zfree = (free_func) 0;
    d_stream.opaque = (voidpf) 0;
    d_stream.next_in = zdata;
    d_stream.avail_in = 0;
    d_stream.next_out = data;

    inflate_t inf(&d_stream);
    if (inf.error() != Z_OK)
    {
        return -1;
    }
    while (d_stream.total_out < ndata && d_stream.total_in < nzdata)
    {
        d_stream.avail_in = d_stream.avail_out = 1;
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END)
        {
            break;
        }
        if (err != Z_OK)
        {
            if (err == Z_DATA_ERROR)
            {
                d_stream.next_in = (Bytef*) dummy_head;
                d_stream.avail_in = sizeof(dummy_head);
                if ((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
    }
    return d_stream.total_out;
}

} // gzip


namespace hustdb_network {

int compress(evhtp::c_str_t src, evhtp::c_str_t * dst)
{
    return gzip::compress((Byte *)src.data, (uLong)src.len, (Byte *)dst->data, (uLong)dst->len);
}

int decompress(evhtp::c_str_t src, evhtp::c_str_t * dst)
{
    return gzip::decompress((Byte *)src.data, (uLong)src.len, (Byte *)dst->data, (uLong)dst->len);
}

} // hustdb_network
