#ifndef __c_dict_20151016212221_h__
#define __c_dict_20151016212221_h__

#include <string.h>

struct c_dict_node_t;
typedef struct c_dict_node_t c_dict_node_t;

typedef struct
{
    c_dict_node_t **buckets;
    unsigned nbuckets, nnodes;
} c_dict_base_t;

typedef struct
{
    unsigned bucketidx;
    c_dict_node_t *node;
} c_dict_iter_t;


#define c_dict_t(T) \
  struct \
  { \
      c_dict_base_t base; \
      T *ref; \
      T tmp; \
  }

#define c_dict_init(dict) \
  memset(dict, 0, sizeof(*(dict)))

#define c_dict_deinit(dict) \
  c_dict_deinit_(&(dict)->base)

#define c_dict_get(dict, key) \
  ( (dict)->ref = c_dict_get_(&(dict)->base, key) )

#define c_dict_set(dict, key, value) \
  ( (dict)->tmp = (value), \
    c_dict_set_(&(dict)->base, key, &(dict)->tmp, sizeof((dict)->tmp)) )

#define c_dict_remove(dict, key) \
  c_dict_remove_(&(dict)->base, key)

#define c_dict_iter(dict) \
  c_dict_iter_()

#define c_dict_next(dict, iter) \
  c_dict_next_(&(dict)->base, iter)


void c_dict_deinit_(c_dict_base_t *dict);
void *c_dict_get_(c_dict_base_t *dict, const char *key);
int c_dict_set_(c_dict_base_t *dict, const char *key, void *value, int vsize);
void c_dict_remove_(c_dict_base_t *dict, const char *key);
c_dict_iter_t c_dict_iter_(void);
const char *c_dict_next_(c_dict_base_t *dict, c_dict_iter_t *iter);


typedef c_dict_t(void*) c_dict_void_t;
typedef c_dict_t(char*) c_dict_str_t;
typedef c_dict_t(int) c_dict_int_t;
typedef c_dict_t(char) c_dict_char_t;
typedef c_dict_t(float) c_dict_float_t;
typedef c_dict_t(double) c_dict_double_t;

#endif // __c_dict_20151016212221_h__
