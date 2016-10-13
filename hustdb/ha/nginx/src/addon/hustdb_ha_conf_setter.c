#include "hustdb_ha_utils_inner.h"

static ngx_bool_t __traverse_file(const char * file, ngx_pool_t * pool, hustdb_ha_traverse_line_cb cb, void * data)
{
    FILE * fp = fopen(file, "r");
    if (!fp)
    {
        return false;
    }
    ngx_bool_t rc = true;
    enum { LINE_BUF = 4096 };
    char buf[LINE_BUF + 1] = {0};
    while (fgets(buf, LINE_BUF, fp))
    {
        if (!cb(buf, pool, data))
        {
            rc = false;
            break;
        }
    }
    fclose(fp);
    return rc;
}

static size_t __match_upstream_begin(const ngx_str_array_t * lines)
{
    static ngx_str_t TAG = ngx_string("customized_selector");
    size_t i = 0;
    for (i = 0; i < lines->size; ++i)
    {
        ngx_str_t * item = lines->arr + i;
        if (strstr((const char *)item->data, (const char *)TAG.data))
        {
            return i;
        }
    }
    return 0;
}

static size_t __match_upstream_end(const ngx_str_array_t * lines, size_t begin)
{
    static ngx_str_t KEY = ngx_string("server");
    size_t i = 0;
    for (i = begin; i < lines->size; ++i)
    {
        ngx_str_t * item = lines->arr + i;
        if (!strstr((const char *)item->data, (const char *)KEY.data))
        {
            break;
        }
    }
    return i;
}

static ngx_bool_t __write_conf(
    const ngx_str_array_t * lines,
    const ngx_str_array_t * backends,
    size_t begin,
    size_t end,
    ngx_pool_t * pool,
    const char * path)
{
    static ngx_str_t SPACES = ngx_string("        ");
    FILE * fp = fopen(path, "w");
    if (!fp)
    {
        return false;
    }

    size_t i = 0;
    for (i = 0; i < begin; ++i)
    {
        ngx_str_t * item = lines->arr + i;
        fprintf(fp, "%s", item->data);
    }

    for (i = 0; i < backends->size; ++i)
    {
        ngx_str_t * backend = backends->arr + i;
        fprintf(fp, "%sserver %s;\n", SPACES.data, backend->data);
    }

    for (i = end; i < lines->size; ++i)
    {
        ngx_str_t * item = lines->arr + i;
        fprintf(fp, "%s", item->data);
    }

    fclose(fp);
    return true;
}

ngx_bool_t hustdb_ha_overwrite_backends(const ngx_str_array_t * backends, ngx_pool_t * pool)
{
    static ngx_str_t DEL = ngx_string(".del");
    static ngx_str_t TMP = ngx_string(".tmp");
    ngx_str_t ngx_cf = ngx_cycle->conf_file;
    ngx_str_t ngx_cf_del = hustdb_ha_init_dir(&ngx_cf, &DEL, pool);
    ngx_str_t ngx_cf_tmp = hustdb_ha_init_dir(&ngx_cf, &TMP, pool);

    size_t size = 0;
    if (!__traverse_file((const char *)ngx_cf.data, pool, hustdb_ha_incr_count, &size))
    {
        return false;
    }

    ngx_str_array_t lines = { 0, 0 };
    lines.arr = ngx_palloc(pool, size * sizeof(ngx_str_t));
    if (!lines.arr)
    {
        return false;
    }
    if (!__traverse_file((const char *)ngx_cf.data, pool, hustdb_ha_append_item, &lines))
    {
        return false;
    }

    size_t begin = __match_upstream_begin(&lines);
    if (0 == begin)
    {
        return false;
    }
    ++begin;
    size_t end = __match_upstream_end(&lines, begin);
    if (end <= begin || end > lines.size - 2)
    {
        return false;
    }

    if (!__write_conf(&lines, backends, begin, end, pool, (const char *)ngx_cf_tmp.data))
    {
        return false;
    }
    if (0 != rename((const char *)ngx_cf.data, (const char *)ngx_cf_del.data))
    {
        return false;
    }
    if (0 != rename((const char *)ngx_cf_tmp.data, (const char *)ngx_cf.data))
    {
        return false;
    }
    return true;
}
