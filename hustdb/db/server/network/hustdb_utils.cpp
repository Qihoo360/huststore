#include "hustdb_utils.h"

void c_str_t::assign(char * data, size_t len)
{
    this->data = data;
    this->len = len;
}

void base64_encode_internal(const c_str_t *src, const uint8_t *basis, uintptr_t padding, c_str_t *dst)
{
    uint8_t *d, *s;
    size_t len;

    len = src->len;
    s = (uint8_t *)src->data;
    d = (uint8_t *)dst->data;

    while (len > 2)
    {
        *d++ = basis[(s[0] >> 2) & 0x3f];
        *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len)
    {
        *d++ = basis[(s[0] >> 2) & 0x3f];

        if (len == 1)
        {
            *d++ = basis[(s[0] & 3) << 4];
            if (padding)
            {
                *d++ = '=';
            }

        }
        else
        {
            *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis[(s[1] & 0x0f) << 2];
        }

        if (padding)
        {
            *d++ = '=';
        }
    }

    dst->len = d - (uint8_t *)dst->data;
}

void hustdb_base64_encode(const c_str_t *src, c_str_t *dst)
{
    if (!src || !src->data || !dst || !dst->data)
    {
        return;
    }
    static uint8_t basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    base64_encode_internal(src, basis64, 1, dst);
}

#define C_UNESCAPE_URI       1
#define C_UNESCAPE_REDIRECT  2

size_t c_unescape_uri_internal(uint8_t **dst, uint8_t **src, size_t size, int type)
{
    uint8_t  *d, *s, ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    uint8_t * start = *dst;

    d = *dst;
    s = *src;

    state = sw_usual;
    decoded = 0;

    while (size--) {

        ch = *s++;

        switch (state) {
        case sw_usual:
            if (ch == '?'
                && (type & (C_UNESCAPE_URI|C_UNESCAPE_REDIRECT)))
            {
                *d++ = ch;
                goto done;
            }

            if (ch == '%') {
                state = sw_quoted;
                break;
            }

            *d++ = ch;
            break;

        case sw_quoted:

            if (ch >= '0' && ch <= '9') {
                decoded = (uint8_t) (ch - '0');
                state = sw_quoted_second;
                break;
            }

            c = (uint8_t) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                decoded = (uint8_t) (c - 'a' + 10);
                state = sw_quoted_second;
                break;
            }

            // the invalid quoted character

            state = sw_usual;

            *d++ = ch;

            break;

        case sw_quoted_second:

            state = sw_usual;

            if (ch >= '0' && ch <= '9') {
                ch = (uint8_t) ((decoded << 4) + ch - '0');

                if (type & C_UNESCAPE_REDIRECT) {
                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);

                    break;
                }

                *d++ = ch;

                break;
            }

            c = (uint8_t) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (uint8_t) ((decoded << 4) + c - 'a' + 10);

                if (type & C_UNESCAPE_URI) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    *d++ = ch;
                    break;
                }

                if (type & C_UNESCAPE_REDIRECT) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
                    break;
                }

                *d++ = ch;

                break;
            }

            // the invalid quoted character

            break;
        }
    }

done:

    *dst = d;
    *src = s;

    return d - start;
}

size_t c_unescape_uri(const char * src, size_t size, char * dst)
{
    return c_unescape_uri_internal((uint8_t **)&dst, (uint8_t **)&src, size, C_UNESCAPE_URI);
}

size_t hustdb_unescape_str(char * str, size_t size)
{
    size_t len = c_unescape_uri(str, size, str);
    str[len] = '\0';
    return len;
}


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
    // `windowBits` must be `MAX_WBITS + 16` to support header & trailer
    m_err = deflateInit2(strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY);
}

deflate_t::~deflate_t()
{
    deflateEnd(m_strm);
}

int hustdb_gzcompress(Bytef * src, uLong src_len, Bytef * dst, uLong dst_len)
{
    if (!src || src_len < 1)
    {
        return -1;
    }

    z_stream c_stream;
    int err = 0;

    c_stream.zalloc = NULL;
    c_stream.zfree = NULL;
    c_stream.opaque = NULL;

    deflate_t def(&c_stream);
    if (def.error() != Z_OK)
    {
        return -1;
    }
    c_stream.next_in = src;
    c_stream.avail_in = src_len;
    c_stream.next_out = dst;
    c_stream.avail_out = dst_len;
    while (c_stream.avail_in != 0 && c_stream.total_out < dst_len)
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
    // `windowBits` must be `MAX_WBITS + 16` to support header & trailer
    m_err = inflateInit2(strm, MAX_WBITS + 16);
}

inflate_t::~inflate_t()
{
    inflateEnd(m_strm);
}

int hustdb_gzdecompress(Byte * src, uLong src_len, Byte * dst, uLong dst_len)
{
    int err = 0;
    z_stream d_stream = { 0 };
    static char dummy_head[2] = { 0x8 + 0x7 * 0x10, (((0x8 + 0x7 * 0x10)
        * 0x100 + 30) / 31 * 31) & 0xFF, };
    d_stream.zalloc = NULL;
    d_stream.zfree = NULL;
    d_stream.opaque = NULL;
    d_stream.next_in = src;
    d_stream.avail_in = 0;
    d_stream.next_out = dst;

    inflate_t inf(&d_stream);
    if (inf.error() != Z_OK)
    {
        return -1;
    }
    while (d_stream.total_out < dst_len && d_stream.total_in < src_len)
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
