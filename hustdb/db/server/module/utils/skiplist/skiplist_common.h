#ifndef __skiplist_common_h__
#define __skiplist_common_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_define.h"

#define SKIPLIST_DEFAULT_MIN_ALLOC_ELEMENTS_ONCE 128

typedef int (*skiplist_compare_func)(const void *p1, const void *p2);
typedef void (*skiplist_free_func)(void *ptr);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif // __skiplist_common_h__
