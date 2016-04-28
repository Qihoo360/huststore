#ifndef __c_base64_20151117171314_h__
#define __c_base64_20151117171314_h__

#ifndef __typedef_20151013165314_h__
#define __typedef_20151013165314_h__

#include <stdint.h>
#include <stddef.h>

typedef unsigned char uchar_t;
typedef unsigned char c_bool_t;

typedef struct
{
    size_t      len;
    uchar_t     *data;
} c_str_t;

#endif // __typedef_20151013165314_h__

void c_encode_base64(const c_str_t *src, c_str_t *dst);
c_bool_t c_decode_base64(const c_str_t *src, c_str_t *dst);

#endif // __c_base64_20151117171314_h__
