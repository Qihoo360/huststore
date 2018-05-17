#ifndef __compression_20180517155644_h__
#define __compression_20180517155644_h__

#include <stdio.h>

namespace hustdb {

int compress(const char * src, size_t src_len, char * dst, size_t dst_len);
int decompress(const char * src, size_t src_len, char * dst, size_t dst_len);

}

#endif // __compression_20180517155644_h__
