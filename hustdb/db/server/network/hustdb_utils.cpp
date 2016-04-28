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
