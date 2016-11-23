#include "hustdb_ha_utils.h"
#include <ngx_md5.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#undef MAX_EXPORT_OFFSET
#define MAX_EXPORT_OFFSET    100000000

#undef MAX_EXPORT_SIZE
#define MAX_EXPORT_SIZE      1000

#undef HUSTDB_HA_KEY_SIZE
#define HUSTDB_HA_KEY_SIZE   64

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
    ngx_str_t out = { 0, 0 };
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
