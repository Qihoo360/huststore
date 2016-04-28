#include "cjson_serialization_base.h"

static void *(*g_json_malloc)(size_t sz) = malloc;
static void (*g_json_free)(void *ptr) = free;

void json_init_hooks(json_hooks_t* hooks)
{
	if (!hooks)
	{
		g_json_malloc = malloc;
		g_json_free = free;
	    return;
	}

	g_json_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	g_json_free = (hooks->free_fn)?hooks->free_fn:free;
}

void * json_malloc(size_t size)
{
	return g_json_malloc(size);
}

void json_free(void * obj)
{
	return g_json_free(obj);
}

static char * __load_from_file(const char * path)
{
	if (!path)
	{
		return NULL;
	}
	FILE * f = fopen(path, "rb");
	if (!f)
	{
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char * data = json_malloc(len + 1);
	fread(data, 1, len, f);
	data[len] = '\0';
	fclose(f);
	return data;
}

static json_bool_t __save_to_file(const char * data, const char * path)
{
	if (!data || !path)
	{
		return false;
	}
	FILE * f = fopen(path, "wb");
	if (!f)
	{
		return false;
	}
	size_t len = strlen(data);
	fwrite(data, 1, len, f);
	fclose(f);
	return true;
}

json_bool_t json_is_array(const json_t * json_val)
{
	return json_val && cJSON_Array == json_val->type;
}

json_bool_t json_is_object(const json_t * json_val)
{
	return json_val && cJSON_Object == json_val->type;
}

json_t * json_object()
{
	return cJSON_CreateObject();
}

json_t * json_array()
{
	return cJSON_CreateArray();
}

size_t json_array_size(const json_t * json_val)
{
	return cJSON_GetArraySize((json_t *)json_val);
}

json_t * json_array_get_item(const json_t * json_val, size_t index)
{
	return cJSON_GetArrayItem((json_t *)json_val, index);
}

json_bool_t json_array_append_item(json_t * json_val, json_t * item)
{
	if (!json_is_array(json_val) || !item)
	{
		return false;
	}
	cJSON_AddItemToArray(json_val, item);
	return true;
}

json_t * json_object_get_field(const json_t * json_val, const char * key)
{
	return cJSON_GetObjectItem((json_t *)json_val, key);
}

json_bool_t json_object_set_field(json_t * json_val, const char * key, json_t * child)
{
	if (!json_val || !key || !child || !json_is_object(json_val))
	{
		return false;
	}
	cJSON_AddItemToObject(json_val, key, child);
	return true;
}

json_t * json_load_from_str(const char * json_val)
{
	return cJSON_Parse(json_val);
}

char * json_dump_to_str(const json_t * json_val)
{
	return cJSON_PrintUnformatted((json_t *)json_val);
}

json_t * json_load_from_file(const char * path)
{
	char * data = __load_from_file(path);
	if (!data)
	{
		return NULL;
	}
	json_t * json_val = json_load_from_str(data);
	json_free(data);
	return json_val;
}

json_bool_t json_dump_to_file(const json_t * json_val, const char * path)
{
	char * data = cJSON_Print((json_t *)json_val);
	if (!data)
	{
		return false;
	}
	json_bool_t result = __save_to_file(data, path);
	json_free(data);
	return result;
}

json_bool_t cjson_copy_to_string(const char * src, json_str_t * des)
{
    if (!src || !des)
    {
        return false;
    }
    des->len = strlen(src);
	des->buf = json_malloc(des->len + 1);
	memcpy(des->buf, src, des->len);
	(des->buf)[des->len] = '\0';
	return true;
}

json_bool_t cjson_set_integer(const json_int_t * src, json_int_t * des)
{
    if (!src || !des)
    {
        return false;
    }
    *des = *src;
    return true;
}

json_bool_t cjson_set_double(const double * src, double * des)
{
    if (!src || !des)
    {
        return false;
    }
    *des = *src;
    return true;
}

json_bool_t cjson_set_string(const json_str_t * src, json_str_t * des)
{
    if (!src || !des)
    {
        return false;
    }
    return cjson_copy_to_string(src->buf, des);
}

json_bool_t cjson_eq_integer(const json_int_t * src, json_int_t * des)
{
	if (!src || !des)
	{
		return false;
	}
	return *src == *des;
}

json_bool_t cjson_eq_double(const double * src, double * des)
{
	if (!src || !des)
	{
		return false;
	}
	return *src == *des;
}

json_bool_t cjson_eq_string(const json_str_t * src, json_str_t * des)
{
	if (!src || !des)
	{
		return false;
	}
	if (src->len != des->len)
	{
		return false;
	}
	return 0 == memcmp(src->buf, des->buf, src->len);
}

void cjson_dispose_integer(json_int_t * obj_val)
{
	if (obj_val)
	{
		*obj_val = 0;
	}
}

void cjson_dispose_double(double * obj_val)
{
	if (obj_val)
	{
		*obj_val = 0.0;
	}
}

void cjson_dispose_string(json_str_t * obj_val)
{
	if (obj_val && obj_val->buf)
	{
		json_free(obj_val->buf);
		obj_val->buf = NULL;
		obj_val->len = 0;
	}
}

json_t * cjson_serialize_integer(const json_int_t * obj_val)
{
	return obj_val ? cJSON_CreateNumber(*obj_val) : NULL;
}

json_t * cjson_serialize_double(const double * obj_val)
{
	return obj_val ? cJSON_CreateNumber(*obj_val) : NULL;
}

json_t * cjson_serialize_string(const json_str_t * obj_val)
{
	return obj_val ? cJSON_CreateString(obj_val->buf) : NULL;
}

json_bool_t cjson_deserialize_integer(const json_t * json_val, json_int_t * obj_val)
{
	if (!json_val || !obj_val)
	{
		return false;
	}
	*obj_val = json_val->valueint;
	return true;
}

json_bool_t cjson_deserialize_double(const json_t * json_val, double * obj_val)
{
	if (!json_val || !obj_val)
	{
		return false;
	}
	*obj_val = json_val->valuedouble;
	return true;
}

json_bool_t cjson_deserialize_string(const json_t * json_val, json_str_t * obj_val)
{
	if (!json_val || !json_val->valuestring || !obj_val)
	{
		return false;
	}
	return cjson_copy_to_string(json_val->valuestring, obj_val);
}

void cjson_dispose_json_value(json_t * json_val)
{
	if (!json_val)
	{
		return;
	}
	cJSON_Delete(json_val);
}
