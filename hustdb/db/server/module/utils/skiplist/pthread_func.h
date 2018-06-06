#ifndef __pthread_func_h__
#define __pthread_func_h__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "common_define.h"

#ifdef __cplusplus
extern "C" {
#endif

int init_pthread_lock(pthread_mutex_t *pthread_lock);
int init_pthread_attr(pthread_attr_t *pattr, const int stack_size);

int create_work_threads(int *count, void *(*start_func)(void *), \
        void *arg, pthread_t *tids, const int stack_size);
int kill_work_threads(pthread_t *tids, const int count);

#ifdef __cplusplus
}
#endif

#endif // __pthread_func_h__
