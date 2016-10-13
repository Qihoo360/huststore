#include "hustdb_ha_write_inner.h"

typedef struct
{
    hustdb_ha_write_args_t base;
    const char * key;
    const char * tb;
    uint64_t score;
    int8_t opt;
    uint32_t ttl;
} write_ctx_t;

static ngx_bool_t __parse_args(
    ngx_bool_t has_tb,
    ngx_bool_t hash_by_tb,
    ngx_bool_t is_zset,
    ngx_http_request_t *r,
    write_ctx_t * ctx);

hustdb_ha_write_ctx_t * hustdb_ha_create_write_ctx(ngx_http_request_t *r)
{
    hustdb_ha_write_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_write_ctx_t));
    if (!ctx)
    {
        return NULL;
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(hustdb_ha_write_ctx_t));
    return ctx;
}

ngx_bool_t hustdb_ha_init_write_args(const char * key, hustdb_ha_write_args_t * ctx)
{
    ctx->peer = hustdb_ha_get_writelist(key);
    if (!ctx->peer)
    {
        return false;
    }

    ctx->error_count = 0;
    ctx->error_peer = NULL;
    ctx->health_peer = NULL;
    ctx->state = STATE_WRITE_MASTER1;

    ngx_bool_t alive = ngx_http_peer_is_alive(ctx->peer->peer);
    if (!alive) // master1
    {
        ++ctx->error_count;
        ctx->error_peer = ctx->peer;

        ctx->peer = ctx->peer->next;
        ctx->state = STATE_WRITE_MASTER2;
        alive = ngx_http_peer_is_alive(ctx->peer->peer);
        if (!alive) // master2
        {
            return false;
        }
    }
    return true;
}

static ngx_bool_t __set_write_context(
    const char * key,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    write_ctx_t * ctx)
{
    if (key)
    {
        ctx->key = key;
    }
    else
    {
        ctx->key = ngx_http_get_param_val(&r->args, "key", r->pool);
        if (!ctx->key)
        {
            return false;
        }
    }
    return __parse_args(has_tb, false, false, r, ctx);
}

static ngx_bool_t __parse_args(
    ngx_bool_t has_tb,
    ngx_bool_t hash_by_tb,
    ngx_bool_t is_zset,
    ngx_http_request_t *r,
    write_ctx_t * ctx)
{
    ctx->tb = ngx_http_get_param_val(&r->args, "tb", r->pool);
    if (has_tb && !ctx->tb)
    {
        return false;
    }

    ctx->ttl = 0;
    char * val = ngx_http_get_param_val(&r->args, "ttl", r->pool);
    if (val)
    {
        ctx->ttl = atoi(val);
    }

    ctx->score = 0;
    ctx->opt = 0;

    if (is_zset)
    {
        val = ngx_http_get_param_val(&r->args, "score", r->pool);
        if (val)
        {
            char * endptr;
            ctx->score = strtoull(val, &endptr, 10);
        }
        val = ngx_http_get_param_val(&r->args, "opt", r->pool);
        if (val)
        {
            ctx->opt = atoi(val);
        }
    }

    return hustdb_ha_init_write_args(hash_by_tb ? ctx->tb : ctx->key, &ctx->base);
}

static void __copy_data(write_ctx_t * tmp, hustdb_ha_write_ctx_t * ctx)
{

    ctx->base.key = tmp->key;
    ctx->base.tb = tmp->tb;
    ctx->ttl = tmp->ttl;
    ctx->base.score = tmp->score;
    ctx->base.opt = tmp->opt;

    ctx->base.peer = tmp->base.peer;
    ctx->state = tmp->base.state;
    ctx->error_count = tmp->base.error_count;
    ctx->error_peer = tmp->base.error_peer;
    ctx->health_peer = tmp->base.health_peer;
}

ngx_bool_t hustdb_ha_init_write_ctx_by_body(ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx)
{
    do
    {
        char * key = hustdb_ha_get_key_from_body(r);
        if (!key)
        {
            break;
        }

        write_ctx_t tmp;
        if (!__set_write_context(key, ctx->base.has_tb, r, &tmp))
        {
            break;
        }

        __copy_data(&tmp, ctx);

        return true;
    } while (0);

    hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    return false;
}

ngx_bool_t hustdb_ha_parse_args(ngx_bool_t has_tb, ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx)
{
    write_ctx_t tmp;
    if (!__set_write_context(NULL, has_tb, r, &tmp))
    {
        return false;
    }
    __copy_data(&tmp, ctx);
    return true;
}

ngx_bool_t hustdb_ha_parse_zset_args(ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx)
{
    write_ctx_t tmp;
    if (!__parse_args(true, true, true, r, &tmp))
    {
        return false;
    }
    __copy_data(&tmp, ctx);
    return true;
}
