#include "hustmq_ha_utils.h"

static ngx_bool_t __is_valid(const char c)
{
	if (c >= '0' && c <= '9')
	{
		return true;
	}
	if (c >= 'a' && c <= 'z')
	{
		return true;
	}
	if (c >= 'A' && c <= 'Z')
	{
		return true;
	}
	if ('_' == c || ':' == c || '.' == c)
	{
		return true;
	}
	return false;
}

ngx_bool_t hustmq_ha_check_queue(const char * qname, const int qname_len)
{
	if (!qname || qname_len <= 0 || qname_len > HUSTMQ_HA_QUEUE_SIZE)
	{
		return false;
	}

	const char * str = qname;
	int i = 0;
	for (i = 0; i < qname_len; ++i)
	{
		if (!__is_valid(str[i]))
		{
			return false;
		}
	}
	return true;
}

int hustmq_ha_get_ready_sum(const int * arr, size_t size)
{
	int sum = 0;
	size_t i = 0;
	for (i = 0; i < size; ++i)
	{
		sum += *(arr + i);
	}
	return sum;
}

unsigned int hustmq_ha_get_idx_cycle(unsigned int start, unsigned int end)
{
    if (start <= end)
    {
        return end - start;
    }
    else if (start > end && start > MAX_QUEUE_ITEM_NUM)
    {
        return CYCLE_QUEUE_ITEM_NUM - start + end;
    }
    return 0;
}

void hustmq_ha_copy_from_integer_array(const IntegerArray * src, int * des, size_t max_size)
{
	if (!src || !des)
	{
		return;
	}
	size_t size = src->size < max_size ? src->size : max_size;
	size_t i = 0;
	for (i = 0; i < size; ++i)
	{
		*(des + i) = *(src->arr + i);
	}
}

void hustmq_ha_copy_from_json_str(const json_str_t * src, char * des, size_t max_size)
{
	if (!src || !src->buf || !des)
	{
		return;
	}
	size_t size = src->len < max_size ? src->len : max_size;
	memcpy(des, src->buf, size);
	*(des + size) = '\0';
}

void hustmq_ha_clean_invalid_items(c_dict_base_t * dict, hustmq_ha_is_invalid_t is_invalid, size_t * dict_size)
{
    enum { MAX_KEYS = 1024 };
    const char * keys[MAX_KEYS];
    size_t size = 0;

    const char * key;
    c_dict_iter_t iter = c_dict_iter_();
    while ((key = c_dict_next_(dict, &iter)))
    {
        if (is_invalid(dict, key))
        {
            keys[size++] = key;
        }
        if (size > MAX_KEYS - 1)
        {
            break;
        }
    }

    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        c_dict_remove_(dict, keys[i]);
        --*dict_size;
    }
}

size_t hustmq_ha_get_queue_size(ngx_queue_t * queue)
{
    size_t size = 0;
    ngx_queue_t * q = NULL;
    for (q = ngx_queue_head(queue); q != ngx_queue_sentinel(queue); q = ngx_queue_next(q))
    {
        ++size;
    }
    return size;
}
