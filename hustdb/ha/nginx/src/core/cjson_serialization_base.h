#ifndef __cjson_serialization_base_20150617094436_h__
#define __cjson_serialization_base_20150617094436_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cJSON.h"

#undef true
#define true 1

#undef false
#define false 0

typedef unsigned char json_bool_t;
typedef int           json_int_t;
typedef cJSON         json_t;

typedef struct
{
	char * buf;
	size_t len;
} json_str_t;

typedef struct
{
      void *(*malloc_fn)(size_t size);
      void (*free_fn)(void * obj);
} json_hooks_t;

void json_init_hooks(json_hooks_t* hooks);
void * json_malloc(size_t size);
void json_free(void * obj);

json_bool_t json_is_array(const json_t * json_val);
json_bool_t json_is_object(const json_t * json_val);
json_t * json_object();
json_t * json_array();
size_t json_array_size(const json_t * json_val);
json_t * json_array_get_item(const json_t * json_val, size_t index);
json_bool_t json_array_append_item(json_t * json_val, json_t * item);
json_t * json_object_get_field(const json_t * json_val, const char * key);
json_bool_t json_object_set_field(json_t * json_val, const char * key, json_t * child);
json_t * json_load_from_str(const char * json_val);
char * json_dump_to_str(const json_t * json_val);
json_t * json_load_from_file(const char * path);
json_bool_t json_dump_to_file(const json_t * json_val, const char * path);

json_bool_t cjson_copy_to_string(const char * src, json_str_t * des);

json_bool_t cjson_set_integer(const json_int_t * src, json_int_t * des);
json_bool_t cjson_set_double(const double * src, double * des);
json_bool_t cjson_set_string(const json_str_t * src, json_str_t * des);

json_bool_t cjson_eq_integer(const json_int_t * src, json_int_t * des);
json_bool_t cjson_eq_double(const double * src, double * des);
json_bool_t cjson_eq_string(const json_str_t * src, json_str_t * des);

void cjson_dispose_integer(json_int_t * obj_val);
void cjson_dispose_double(double * obj_val);
void cjson_dispose_string(json_str_t * obj_val);

json_t * cjson_serialize_integer(const json_int_t * obj_val);
json_t * cjson_serialize_double(const double * obj_val);
json_t * cjson_serialize_string(const json_str_t * obj_val);

json_bool_t cjson_deserialize_integer(const json_t * json_val, json_int_t * obj_val);
json_bool_t cjson_deserialize_double(const json_t * json_val, double * obj_val);
json_bool_t cjson_deserialize_string(const json_t * json_val, json_str_t * obj_val);

void cjson_dispose_json_value(json_t * json_val);

#endif // __cjson_serialization_base_20150617094436_h__
