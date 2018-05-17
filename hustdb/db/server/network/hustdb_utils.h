#ifndef __hustdb_utils_20160414141755_h__
#define __hustdb_utils_20160414141755_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define c_make_str(s) { sizeof(s) - 1, (char *) s }
#define c_base64_encoded_length(len)  (((len + 2) / 3) * 4)

struct c_str_t
{
    size_t len;
    char * data;
    void assign(char * data, size_t len);
};

void hustdb_base64_encode(const c_str_t *src, c_str_t *dst);
size_t hustdb_unescape_str(char * str, size_t size);

#endif // __hustdb_utils_20160414141755_h__
