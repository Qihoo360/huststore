#include "compression.h"
#include <zlib.h>

namespace hustdb {

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


int gzcompress(Bytef *data, uLong ndata, Bytef *zdata, uLong nzdata)
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

int gzdecompress(Byte *zdata, uLong nzdata, Byte *data, uLong ndata)
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

int compress(const char * src, size_t src_len, char * dst, size_t dst_len)
{
    return gzcompress((Byte *)src, (uLong)src_len, (Byte *)dst, (uLong)dst_len);
}

int decompress(const char * src, size_t src_len, char * dst, size_t dst_len)
{
    return gzdecompress((Byte *)src, (uLong)src_len, (Byte *)dst, (uLong)dst_len);
}

} // hustdb
