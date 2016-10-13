#include "hustdb_ha_utils_inner.h"

static ngx_str_t g_zlog_conf_path = { 0, 0 };
static ngx_str_t g_zlog_default_level = ngx_string("debug");

void zlog_init_conf_path()
{
    ngx_str_t name = ngx_string("zlog.conf");
    g_zlog_conf_path = ngx_http_get_conf_path((ngx_cycle_t *)ngx_cycle, &name);
}

int zlog_load_conf(zlog_handler handler)
{
    return handler((const char *)g_zlog_conf_path.data);
}

typedef void (*zlog_write_t) (zlog_category_t * category, const char * data);

void zlog_write_fatal(zlog_category_t * category, const char * data)
{
    zlog_fatal(category, "%s", data);
}

void zlog_write_error(zlog_category_t * category, const char * data)
{
    zlog_error(category, "%s", data);
}

void zlog_write_warn(zlog_category_t * category, const char * data)
{
    zlog_warn(category, "%s", data);
}

void zlog_write_notice(zlog_category_t * category, const char * data)
{
    zlog_notice(category, "%s", data);
}

void zlog_write_info(zlog_category_t * category, const char * data)
{
    zlog_info(category, "%s", data);
}

void zlog_write_debug(zlog_category_t * category, const char * data)
{
    zlog_debug(category, "%s", data);
}

typedef struct
{
    const char * level;
    zlog_write_t writer;
} hustdb_ha_level_log_t;

static hustdb_ha_level_log_t hustdb_ha_level_writer_dict[] =
{
    { "fatal",  zlog_write_fatal  },
    { "error",  zlog_write_error  },
    { "warn",   zlog_write_warn   },
    { "notice", zlog_write_notice },
    { "info",   zlog_write_info   },
    { "debug",  zlog_write_debug  }
};

static size_t hustdb_ha_level_writer_dict_len = sizeof(hustdb_ha_level_writer_dict) / sizeof(hustdb_ha_level_log_t);

hustdb_ha_level_log_t * get_hustdb_ha_level_writer_item(const char * level)
{
    size_t i = 0;
    for (i = 0; i < hustdb_ha_level_writer_dict_len; ++i)
    {
        if (0 == strcmp(level, hustdb_ha_level_writer_dict[i].level))
        {
            return hustdb_ha_level_writer_dict + i;
        }
    }
    return NULL;
}

typedef struct
{
    const char * level;
    const char * uri;
    const char * data;
} zlog_item_t;

static ngx_bool_t __write_log_item(const zlog_item_t * item, const char * zlog_mdc)
{
    if (!item)
    {
        return false;
    }

    hustdb_ha_level_log_t * writer = get_hustdb_ha_level_writer_item(item->level);
    if (!writer)
    {
        return false;
    }
    zlog_category_t * category = zlog_get_category(item->uri);
    if (!category)
    {
        return false;
    }
    zlog_put_mdc(zlog_mdc, item->uri);
    writer->writer(category, item->data);
    return true;
}

ngx_bool_t hustdb_ha_write_log(
    const ngx_str_t * zlog_mdc,
    const ngx_str_t * uri,
    ngx_buf_t * buf,
    ngx_pool_t * pool)
{
    size_t buf_size = ngx_http_get_buf_size(buf);
    size_t dst_len = ngx_base64_encoded_length(buf_size) + 1;
    u_char * base64_dst = ngx_pcalloc(pool, dst_len);
    base64_dst[dst_len] = '\0';
    ngx_str_t ngx_base64_dst = {dst_len, base64_dst};
    ngx_str_t ngx_base64_src = {buf_size, buf->pos};
    ngx_encode_base64(&ngx_base64_dst, &ngx_base64_src);
    zlog_item_t item = {
        (const char *)g_zlog_default_level.data,
        (const char *)uri->data,
        (const char *)(ngx_base64_dst.data)
    };
    return __write_log_item(&item, (const char *)zlog_mdc->data);
}

static ngx_bool_t __mkdir(const ngx_str_t * prefix, const char * uri, ngx_pool_t * pool)
{
    size_t uri_len = strlen(uri);
    ngx_str_t path = ngx_string("logs/");
    size_t size = prefix->len + path.len + uri_len;
    char * full_path = ngx_pcalloc(pool, size + 1);
    memcpy(full_path, prefix->data, prefix->len);
    memcpy(full_path + prefix->len, path.data, path.len);
    memcpy(full_path + prefix->len + path.len, uri, uri_len);
    full_path[size] = '\0';
    if (0 == access(full_path, F_OK))
    {
        return true;
    }
    if (-1 == mkdir(full_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        return false;
    }
    return true;
}

ngx_bool_t hustdb_ha_init_log_dirs(const ngx_str_t * prefix, ngx_pool_t * pool)
{
    size_t i = 0;
    size_t count = hustdb_ha_get_peer_array_count();
    for (i = 0; i < count; ++i)
    {
        const char * uri = hustdb_ha_get_peer_item_uri(i);
        if (!__mkdir(prefix, uri, pool))
        {
            return false;
        }
    }
    return true;
}
