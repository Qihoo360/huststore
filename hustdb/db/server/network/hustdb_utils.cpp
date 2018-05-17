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
