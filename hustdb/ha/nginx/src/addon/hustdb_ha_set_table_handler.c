#include "hustdb_ha_handler.h"

#define NGX_RESTART_INSTANCE_ERROR    0
#define OVERWRITE_BACKENDS_ERROR      127

#define IDENTIFIER_LEN 21

static ngx_http_hustdb_ha_main_conf_t * g_mcf = NULL;

typedef struct
{
    ngx_queue_t que;
    uint64_t exptime;
    char id[IDENTIFIER_LEN];
} set_table_identifier_t;

static ngx_str_t __gen_identifier(ngx_pool_t * pool)
{
    struct timeval time;
    struct timezone tz;
    gettimeofday(&time, &tz);

    uint64_t key = time.tv_sec * 1000000 + time.tv_usec;

    ngx_str_t id;
    id.data = ngx_palloc(pool, IDENTIFIER_LEN);
    sprintf((char *)id.data, "%lu", key);
    id.len = strlen((const char *)id.data);

    return id;
}

static ngx_int_t __init_identifier_cache(
    size_t cache_size,
    ngx_slab_pool_t * shpool,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    ngx_queue_init(cache);
    ngx_queue_init(free);

    size_t i = 0;
    for (i = 0; i < cache_size; i++)
    {
        set_table_identifier_t * item = ngx_slab_alloc(shpool, sizeof(set_table_identifier_t));
        if (!item)
        {
            return NGX_ERROR;
        }
        memset(item, 0, sizeof(set_table_identifier_t));
        item->exptime = 0;
        ngx_queue_insert_head(free, &item->que);
    }
    return NGX_OK;
}

static set_table_identifier_t * __alloc_identifier(ngx_queue_t * from, ngx_queue_t * to)
{
    if (ngx_queue_empty(from))
    {
        return NULL;
    }

    ngx_queue_t * q = ngx_queue_head(from);
    ngx_queue_remove(q);
    set_table_identifier_t * item = ngx_queue_data(q, set_table_identifier_t, que);

    ngx_queue_insert_head(to, q);

    return item;
}

static ngx_int_t __free_identifier(set_table_identifier_t * item, ngx_queue_t * free)
{
    ngx_queue_remove(&item->que);
    ngx_queue_insert_head(free, &item->que);
    return NGX_OK;
}

ngx_bool_t hustdb_ha_init_identifier_cache(ngx_http_hustdb_ha_main_conf_t * mcf)
{
    g_mcf = mcf;
    ngx_http_addon_shm_ctx_t * ctx = g_mcf->zone->data;
    hustdb_ha_shctx_t * sh = ctx->sh;
    return NGX_OK == __init_identifier_cache(mcf->identifier_cache_size, ctx->shpool, &sh->cache_identifier, &sh->free_identifier);
}

static void __set_error_code(int code, ngx_http_request_t * r)
{
    ngx_str_t val = { 0, 0 };
    val.data = ngx_palloc(r->pool, 11);
    sprintf((char *)val.data, "%d", code);
    val.len = strlen((const char *)val.data);
    static ngx_str_t key = ngx_string("ErrorCode");
    ngx_http_add_field_to_headers_out(&key, &val, r);
}

static ngx_bool_t __set_hash_table(ngx_http_request_t * r, ngx_buf_t * buf, size_t buf_size)
{
    if (!hustdb_ha_lock_table(g_mcf))
    {
        return false;
    }

    int err_code = -1;
    do
    {
        ngx_str_t table_content = { buf_size, buf->pos };
        hustdb_ha_set_table_result_t result = hustdb_ha_overwrite_table(&table_content, r->pool);
        if (SET_TABLE_SUCCESS != result)
        {
            err_code = (int) result;
            break;
        }
        ngx_str_array_t backends;
        if (!hustdb_ha_load_backends(r->pool, &backends))
        {
            err_code = OVERWRITE_BACKENDS_ERROR;
            break;
        }

        if (!hustdb_ha_overwrite_backends(&backends, r->pool))
        {
            err_code = OVERWRITE_BACKENDS_ERROR;
            break;
        }

        if (NGX_OK != ngx_restart_instance())
        {
            err_code = NGX_RESTART_INSTANCE_ERROR;
            break;
        }
    } while (0);

    if (-1 != err_code)
    {
        __set_error_code(err_code, r);
        hustdb_ha_unlock_table(g_mcf);
        return false;
    }
    return true;
}

ngx_queue_t * __match_id(ngx_queue_t * queue, char * id)
{
    ngx_queue_t * it = NULL;
    for (it = ngx_queue_head(queue); it != ngx_queue_sentinel(queue); it = ngx_queue_next(it))
    {
        set_table_identifier_t * item = ngx_queue_data(it, set_table_identifier_t, que);
        if (0 == strcmp(id, item->id))
        {
            return it;
        }
    }
    return NULL;
}

static ngx_bool_t __post_body_cb(
    ngx_http_request_t * r, ngx_buf_t * buf, size_t buf_size)
{
    ngx_http_addon_shm_ctx_t * ctx = g_mcf->zone->data;
    hustdb_ha_shctx_t * sh = ctx->sh;

    char * key = (char *) ngx_http_get_addon_module_ctx(r);

    ngx_shmtx_lock(&ctx->shpool->mutex);
    ngx_queue_t * item = __match_id(&sh->cache_identifier, key);
    ngx_shmtx_unlock(&ctx->shpool->mutex);

    if (!item)
    {
        return false;
    }

    set_table_identifier_t * id = ngx_queue_data(item, set_table_identifier_t, que);

    ngx_shmtx_lock(&ctx->shpool->mutex);
    id->exptime = 0;
    __free_identifier(id, &sh->free_identifier);
    ngx_shmtx_unlock(&ctx->shpool->mutex);

    return __set_hash_table(r, buf, buf_size);
}

static void __post_body_handler(ngx_http_request_t * r)
{
    ngx_http_post_body_handler(r, __post_body_cb);
}

static void __refresh_expired(uint64_t now, hustdb_ha_shctx_t * sh)
{
    set_table_identifier_t * expire_item = NULL;
    ngx_queue_t * it = NULL;
    for (it = ngx_queue_head(&sh->cache_identifier); it != ngx_queue_sentinel(&sh->cache_identifier); it = ngx_queue_next(it))
    {
        set_table_identifier_t * item = ngx_queue_data(it, set_table_identifier_t, que);
        if (now > item->exptime && item->exptime > 0)
        {
            expire_item = item;
            break;
        }
    }

    if (expire_item)
    {
        expire_item->exptime = 0;
        __free_identifier(expire_item, &sh->free_identifier);
    }
}

static ngx_int_t __generate_identifier(hustdb_ha_shctx_t * sh, ngx_http_request_t *r)
{
    ngx_time_t * tp = ngx_timeofday();
    uint64_t now = (uint64_t) tp->sec * 1000 + tp->msec;
    __refresh_expired(now, sh);

    uint64_t exptime = now + g_mcf->identifier_timeout;

    ngx_str_t identifier = __gen_identifier(r->pool);
    if (__match_id(&sh->cache_identifier, (char *)identifier.data))
    {
        return ngx_http_send_response_imp(NGX_HTTP_CONFLICT, NULL, r);
    }

    set_table_identifier_t * id = __alloc_identifier(&sh->free_identifier, &sh->cache_identifier);
    if (!id)
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }

    ngx_str_t cipher_id = hustdb_ha_rsa_encrypt(&identifier, (const char *)g_mcf->public_pem_full_path.data, r->pool);
    if (cipher_id.len < 1)
    {
        __free_identifier(id, &sh->free_identifier);
        return ngx_http_send_response_imp(NGX_HTTP_NOT_ALLOWED, NULL, r);
    }

    memcpy(id->id, identifier.data, identifier.len);
    id->id[identifier.len] = '\0';
    id->exptime = exptime;

    return ngx_http_send_response_imp(NGX_HTTP_OK, &cipher_id, r);
}

ngx_int_t hustdb_ha_set_table_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    char * val = ngx_http_get_param_val(&r->args, "id", r->pool);
    if (!val)
    {
        ngx_http_addon_shm_ctx_t * ctx = g_mcf->zone->data;
        ngx_shmtx_lock(&ctx->shpool->mutex);
        ngx_int_t rc = __generate_identifier(ctx->sh, r);
        ngx_shmtx_unlock(&ctx->shpool->mutex);
        return rc;
    }

    ngx_http_set_addon_module_ctx(r, val);

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        return rc;
    }
    return NGX_DONE;
}
