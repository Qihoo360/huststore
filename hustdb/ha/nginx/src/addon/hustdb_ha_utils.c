#include "hustdb_ha_utils.h"
#include <ngx_md5.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

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


ngx_str_t hustdb_ha_make_str(ngx_str_t * val, ngx_http_request_t * r)
{
    u_char * tmp = ngx_palloc(r->pool, val->len + 1);
    memcpy(tmp, val->data, val->len);
    tmp[val->len] = '\0';
    ngx_str_t rc = { val->len, tmp };
    return rc;
}

ngx_str_t hustdb_ha_strcat(const ngx_str_t * str, int num, ngx_pool_t * pool)
{
    ngx_str_t out;
    size_t len = str->len + 11;
    out.data = ngx_palloc(pool, len);
    if (!out.data)
    {
        return out;
    }
    sprintf((char *)out.data, "%s%d", str->data, num);
    out.len = strlen((const char *)out.data);
    return out;
}

static ngx_http_peer_array_t g_peer_array;
static hustdb_ha_peers_t g_peers;

ngx_bool_t hustdb_ha_init_peer_array(ngx_pool_t * pool)
{
    ngx_bool_t rc = ngx_http_init_peer_array(pool, ngx_http_get_backends(), ngx_http_get_backend_count(), &g_peer_array);
    if (!rc)
    {
        return false;
    }
    g_peers.size = g_peer_array.size;
    g_peers.peers = ngx_palloc(pool, g_peers.size * sizeof(const char *));
    size_t i = 0;
    for (i = 0; i < g_peers.size; ++i)
    {
        g_peers.peers[i] = (const char *)g_peer_array.arr[i]->server.data;
    }
    return true;
}

hustdb_ha_peers_t * hustdb_ha_get_peers()
{
    return &g_peers;
}

ngx_http_upstream_rr_peer_t * hustdb_ha_get_peer(ngx_http_request_t *r)
{
    char * val = ngx_http_get_param_val(&r->args, PEER_KEY, r->pool);
    if (!val)
    {
        return NULL;
    }
    size_t index = atoi(val);
    return ngx_http_get_peer_by_index(&g_peer_array, index);
}

ngx_str_t hustdb_ha_init_dir(const ngx_str_t * prefix, const ngx_str_t * file, ngx_pool_t * pool)
{
    ngx_str_t path = { 0, 0 };
    path.len = prefix->len + file->len;
    path.data  = ngx_pcalloc(pool, path.len + 1);
    memcpy(path.data, prefix->data, prefix->len);
    memcpy(path.data + prefix->len, file->data, file->len);
    path.data[path.len] = '\0';
    return path;
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
    for (i = 0; i < g_peer_array.size; ++i)
    {
        const char * uri = (const char *) g_peer_array.arr[i]->server.data;
        if (!__mkdir(prefix, uri, pool))
        {
            return false;
        }
    }
    return true;
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

ngx_bool_t hustdb_ha_check_key(const char * key)
{
    size_t key_len = strlen(key);
    if (!key || key_len <= 0 || key_len > HUSTDB_HA_KEY_SIZE)
    {
        return false;
    }

    const char * str = key;
    size_t i = 0;
    for (i = 0; i < key_len; ++i)
    {
        if (!__is_valid(str[i]))
        {
            return false;
        }
    }
    return true;
}

ngx_bool_t hustdb_ha_check_export(int offset, int size)
{
    if (offset < 0 || offset > MAX_EXPORT_OFFSET)
    {
        return false;
    }
    if (size < 0 || size > MAX_EXPORT_SIZE)
    {
        return false;
    }
    return true;
}

ngx_bool_t hustdb_ha_check_hash(int file, int start, int end)
{
    if (file < 0 ||
            start < 0 ||
            end < 0 ||
            start >= end ||
            end > HUSTDB_TABLE_SIZE)
    {
        return false;
    }
    return true;
}

ngx_bool_t hustdb_ha_check_tb(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    char * tb = ngx_http_get_param_val(&r->args, "tb", r->pool);
    if (!tb)
    {
        return false;
    }
    if (!hustdb_ha_check_key(tb))
    {
        return false;
    }
    return true;
}

ngx_bool_t hustdb_ha_check_keys(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    char * val = ngx_http_get_param_val(&r->args, "offset", r->pool);
    if (!val)
    {
        return false;
    }
    int offset = atoi(val);
    val = ngx_http_get_param_val(&r->args, "size", r->pool);
    if (!val)
    {
        return false;
    }
    int size = atoi(val);

    if (!hustdb_ha_check_export(offset, size))
    {
        return false;
    }

    return hustdb_ha_check_tb(backend_uri, r);
}

char * hustdb_ha_get_key_from_body(ngx_http_request_t * r)
{
    ngx_buf_t * buf = ngx_http_get_request_body(r);
    if (!buf)
    {
        return NULL;
    }

    size_t buf_size = ngx_http_get_buf_size(buf);
    if (buf_size < 1)
    {
        return NULL;
    }
    char * key = ngx_palloc(r->pool, buf_size + 1);
    memcpy(key, buf->pos, buf_size);
    key[buf_size] = '\0';
    return key;
}

char * hustdb_ha_get_key(ngx_http_request_t * r)
{
    char * key = hustdb_ha_get_key_from_body(r);
    if (key)
    {
        return key;
    }
    return ngx_http_get_param_val(&r->args, "key", r->pool);
}

char * hustdb_ha_read_file(const char * path, ngx_pool_t * pool)
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
    char * data = (char *)ngx_palloc(pool, len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    return data;
}

ngx_bool_t hustdb_ha_lock_table(ngx_http_hustdb_ha_main_conf_t * mcf)
{
    ngx_http_addon_shm_ctx_t * ctx = mcf->zone->data;
    hustdb_ha_shctx_t * sh = ctx->sh;
    ngx_bool_t rc = true;
    ngx_shmtx_lock(&ctx->shpool->mutex);

    do
    {
        if (sh->table_locked)
        {
            rc = false;
            break;
        }
        sh->table_locked = true;
    } while (0);

    ngx_shmtx_unlock(&ctx->shpool->mutex);
    return rc;
}

void hustdb_ha_unlock_table(ngx_http_hustdb_ha_main_conf_t * mcf)
{
    ngx_http_addon_shm_ctx_t * ctx = mcf->zone->data;
    hustdb_ha_shctx_t * sh = ctx->sh;
    ngx_shmtx_lock(&ctx->shpool->mutex);
    sh->table_locked = false;
    ngx_shmtx_unlock(&ctx->shpool->mutex);
}

ngx_str_t hustdb_ha_rsa_encrypt(const ngx_str_t * plain, const char * public_pem_file, ngx_pool_t * pool)
{
    ngx_str_t cipher = { 0, 0 };
    if (!plain || !plain->data || !public_pem_file)
    {
        return cipher;
    }
    FILE * f = NULL;
    RSA* rsa = NULL;
    int rc = true;
    do
    {
        f = fopen(public_pem_file, "rb");
        if (!f)
        {
            rc = false;
            break;
        }
        rsa = RSA_new();
        if(!PEM_read_RSA_PUBKEY(f, &rsa, 0, 0))
        {
            rc = false;
            break;
        }
        cipher.len = RSA_size(rsa);
        cipher.data = ngx_palloc(pool, cipher.len);
        if (RSA_public_encrypt(plain->len, (const unsigned char *)plain->data, (unsigned char *)cipher.data, rsa, RSA_PKCS1_PADDING) < 0)
        {
            rc = false;
            break;
        }
    } while (0);

    if (rsa)
    {
        RSA_free(rsa);
    }
    if (f)
    {
        fclose(f);
    }
    CRYPTO_cleanup_all_ex_data();
    if (!rc)
    {
        cipher.len = 0;
    }
    return cipher;
}

ngx_bool_t hustdb_ha_incr_count(const char * line, ngx_pool_t * pool, void * data)
{
    size_t * count = (size_t *)data;
    ++*count;
    return true;
}

ngx_bool_t hustdb_ha_append_item(const char * line, ngx_pool_t * pool, void * data)
{
    if (!line || !pool || !data)
    {
        return false;
    }
    ngx_str_array_t * arr = (ngx_str_array_t *)data;
    ngx_str_t * item = arr->arr + arr->size;
    item->len = strlen(line);
    item->data = ngx_palloc(pool, item->len + 1);
    if (!item->data)
    {
        return false;
    }
    memcpy(item->data, line, item->len);
    item->data[item->len] = '\0';
    ++arr->size;
    return true;
}

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
