#include "hustdb_ha_write_handler.h"

// |------------------------------|
// |uint32_t | head_len | 4 bytes |
// |------------------------------|
// |uint8_t  | method   | 1 byte  |
// |------------------------------|
// |uint32_t | ver      | 4 bytes |
// |------------------------------|
// |uint32_t | key_len  | 4 bytes |
// |------------------------------|
// |char *   | key      | n bytes |
// |------------------------------|
// |uint32_t | key_crc  | 4 bytes |
// |------------------------------|
// |uint32_t | ttl      | 4 bytes |
// |------------------------------|
// |uint32_t | tb_len   | 4 bytes |
// |------------------------------|
// |char *   | tb       | n bytes |
// |------------------------------|
// |uint32_t | tb_crc   | 4 bytes |
// |------------------------------|
// |uint64_t | score    | 8 bytes |
// |------------------------------|
// |int8_t   | opt      | 1 bytes |
// |------------------------------|

static ngx_buf_t * __encode_head(
    uint8_t method,
    ngx_bool_t has_tb,
    const ngx_str_t * version,
    const char * key,
    const char * tb,
    uint32_t ttl,
    uint64_t score,
    int8_t opt,
    ngx_pool_t * pool)
{
    if (!key || !pool || !version)
    {
        return NULL;
    }
    uint32_t ver = version->data ? ngx_atoi(version->data, version->len) : 0;
    uint32_t key_len = strlen(key);
    uint32_t key_crc = ngx_crc((u_char *) key, (size_t) key_len);
    uint32_t tb_len = has_tb ? strlen(tb) : 0;
    uint32_t tb_crc = has_tb ? ngx_crc((u_char *) tb, (size_t) tb_len) : 0;
    uint32_t head_len = 0;

    typedef struct
    {
        const void * data;
        size_t len;
    } head_item_t;

    head_item_t heads[] = {
        { &head_len, sizeof(head_len) },
        { &method,   sizeof(method)   },
        { &ver,      sizeof(ver)      },
        { &key_len,  sizeof(key_len)  },
        { key,       (size_t) key_len },
        { &key_crc,  sizeof(key_crc)  },
        { &ttl,      sizeof(ttl)      },
        { &tb_len,   sizeof(tb_len)   },
        { tb,        (size_t) tb_len  },
        { &tb_crc,   sizeof(tb_crc)   },
        { &score,    sizeof(score)    },
        { &opt,      sizeof(opt)      }
    };
    size_t size = sizeof(heads) / sizeof(head_item_t);

    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        head_len += heads[i].len;
    }

    ngx_buf_t * buf = ngx_create_temp_buf(pool, head_len + 1);
    if (!buf)
    {
        return NULL;
    }
    memset(buf->pos, 0, head_len + 1);

    buf->last = buf->pos;
    for (i = 0; i < size; ++i)
    {
        if (heads[i].data && heads[i].len > 0)
        {
            buf->last = ngx_copy(buf->last, heads[i].data, heads[i].len);
        }
    }
    return buf;
}

static hustdb_ha_write_ctx_t * __create_write_ctx(ngx_http_request_t *r)
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

typedef struct
{
    ngx_http_subrequest_peer_t * peer;
    ngx_http_subrequest_peer_t * error_peer;
    ngx_http_subrequest_peer_t * health_peer;
    int error_count;
    hustdb_write_state_t state;
} write_base_ctx_t;

typedef struct
{
    write_base_ctx_t base;
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

static ngx_bool_t __init_write_base_ctx(const char * key, write_base_ctx_t * ctx)
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

    return __init_write_base_ctx(hash_by_tb ? ctx->tb : ctx->key, &ctx->base);
}

static void __copy_data(write_ctx_t * tmp, ngx_http_hustdb_ha_write_ctx_t * ctx)
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

static ngx_bool_t __init_write_ctx_by_body(ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx)
{
    do
    {
        char * key = hustdb_ha_get_key_from_body(r);
        if (!key)
        {
            break;
        }

        write_ctx_t tmp;
        if (!__set_write_context(key, ctx->has_tb, r, &tmp))
        {
            break;
        }

        __copy_data(&tmp, &ctx->base);

        return true;
    } while (0);

    hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    return false;
}

static void __post_handler(ngx_http_request_t *r)
{
    --r->main->count;

    hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ngx_bool_t rc = true;
    if (ctx->key_in_body)
    {
        if (!__init_write_ctx_by_body(r, ctx))
        {
            rc = false;
        }
    }

    if (!rc)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        return;
    }

    ngx_http_gen_subrequest(
        ctx->base.base.base.backend_uri,
        r,
        ctx->base.base.peer->peer,
        &ctx->base.base.base,
        hustdb_ha_on_subrequest_complete);
}

ngx_int_t hustdb_ha_start_post(
    ngx_bool_t support_post_only,
    ngx_bool_t key_in_body,
    ngx_bool_t has_tb,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_write_ctx_t * ctx = __create_write_ctx(r);
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    ctx->base.base.base.backend_uri = backend_uri;

    if (support_post_only && !(r->method & NGX_HTTP_POST))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (key_in_body)
    {
        ctx->key_in_body = true;
        ctx->has_tb = has_tb;
    }
    else
    {
        write_ctx_t tmp;
        if (!__set_write_context(NULL, has_tb, r, &tmp))
        {
            return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        }
        __copy_data(&tmp, &ctx->base);
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

ngx_int_t hustdb_ha_start_del(
    ngx_bool_t support_post_only,
    ngx_bool_t key_in_body,
    ngx_bool_t has_tb,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    if (key_in_body)
    {
        return hustdb_ha_start_post(support_post_only, key_in_body, has_tb, backend_uri, r);
    }

    hustdb_ha_write_ctx_t * args = __create_write_ctx(r);
    if (!args)
    {
        return NGX_ERROR;
    }
    ngx_http_hustdb_ha_write_ctx_t * ctx = &args->base;
    ctx->base.base.backend_uri = backend_uri;
    write_ctx_t tmp;
    if (!__set_write_context(NULL, has_tb, r, &tmp))
    {
        return NGX_ERROR;
    }
    __copy_data(&tmp, ctx);

    return ngx_http_gen_subrequest(
            ctx->base.base.backend_uri,
            r,
            ctx->base.peer->peer,
            &ctx->base.base,
            hustdb_ha_on_subrequest_complete);
}

static ngx_bool_t __add_sync_head(const ngx_str_t * server, ngx_http_request_t *r)
{
    static ngx_str_t SYNC_KEY = ngx_string("Sync");
    return ngx_http_add_field_to_headers_out(&SYNC_KEY, server, r);
}

static ngx_int_t __write_sync_data(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    do
    {
        time_t now = time(NULL);
        uint32_t ttl = (0 == ctx->ttl) ? 0 : (uint32_t) (now + ctx->ttl);

        ngx_buf_t * value = NULL;
        ngx_buf_t * head = __encode_head(method, has_tb, &ctx->base.version, ctx->base.key, ctx->base.tb, ttl, ctx->base.score, ctx->base.opt, r->pool);
        if (r->request_body)
        {
            if (!ngx_http_insert_head_to_body(head, r))
            {
                break;
            }
            value = ngx_http_get_request_body(r);
        }
        else
        {
            value = head;
        }

        if (!value)
        {
            break;
        }

        ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);
        ngx_str_t * server = &ctx->error_peer->peer->server;
        if (!hustdb_ha_write_log(&mcf->zlog_mdc, server, value, r->pool))
        {
            break;
        }

        if (!__add_sync_head(server, r))
        {
            break;
        }

        return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->base.version, NULL, r);
    } while (0);

    return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
}

static ngx_bool_t __match_method(uint8_t methods[], size_t size, uint8_t method)
{
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        if (method == methods[i])
        {
            return true;
        }
    }
    return false;
}

static ngx_bool_t __skip_not_found_error(uint8_t method, ngx_uint_t status)
{
    if (NGX_HTTP_NOT_FOUND != status)
    {
        return false;
    }
    static uint8_t methods[] = {
        HUSTDB_METHOD_DEL,
        HUSTDB_METHOD_HDEL,
        HUSTDB_METHOD_SREM,
        HUSTDB_METHOD_ZREM,
    };
    static size_t size = sizeof(methods) / sizeof(uint8_t);
    return __match_method(methods, size, method);
}

static ngx_bool_t __skip_precondition_error(uint8_t method, ngx_uint_t status)
{
    if (NGX_HTTP_PRECONDITION_FAILED != status)
    {
        return false;
    }
    static uint8_t methods[] = {
        HUSTDB_METHOD_DEL,
        HUSTDB_METHOD_HDEL,
        HUSTDB_METHOD_SREM,
        HUSTDB_METHOD_ZREM,
        HUSTDB_METHOD_HSET,
        HUSTDB_METHOD_SADD,
        HUSTDB_METHOD_ZADD
    };
    static size_t size = sizeof(methods) / sizeof(uint8_t);
    return __match_method(methods, size, method);
}

static ngx_bool_t __skip_error(uint8_t method, ngx_uint_t status)
{
    return __skip_precondition_error(method, status)
        || __skip_not_found_error(method, status);
}

static void __update_error(uint8_t method, ngx_uint_t status, ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    if (NGX_HTTP_OK == status)
    {
        ctx->health_peer = ctx->base.peer;
        return;
    }
    if (__skip_error(method, status))
    {
        ++ctx->skip_error_count;
        return;
    }
    ++ctx->error_count;
    ctx->error_peer = ctx->base.peer;
}

static ngx_bool_t __should_write_log(ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    return 0 == ctx->skip_error_count && 1 == ctx->error_count;
}

static ngx_str_t __format_binlog_args(uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    static const ngx_str_t HOST = ngx_string("host=");
    static const ngx_str_t METHOD = ngx_string("&method=");
    static const ngx_str_t TB = ngx_string("&tb=");
    static const ngx_str_t KEY = ngx_string("&key=");

    char method_str[4];
    sprintf(method_str, "%d", (uint32_t)method);

    size_t method_len = strlen(method_str);

    ngx_str_t * host = &ctx->error_peer->peer->server;

    typedef struct
    {
        const void * data;
        size_t len;
    } arg_item_t;

    arg_item_t items[] = {
        { HOST.data,    HOST.len   },
        { host->data,   host->len  },
        { METHOD.data,  METHOD.len },
        { method_str,   method_len }
    };
    size_t size = sizeof(items) / sizeof(arg_item_t);

    size_t buf_size = 0;

    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        buf_size += items[i].len;
    }

    size_t tb_len = 0;
    if (has_tb)
    {
        tb_len = strlen(ctx->base.tb);
        buf_size += (TB.len + tb_len);
    }
    size_t key_len = 0;
    if (!ctx->base.key_in_body)
    {
        key_len = strlen(ctx->base.key);
        buf_size += (KEY.len + key_len);
    }

    ngx_str_t args = { 0, 0 };
    args.data = ngx_palloc(r->pool, buf_size);

    for (i = 0; i < size; ++i)
    {
        memcpy(args.data + args.len, items[i].data, items[i].len);
        args.len += items[i].len;
    }

    if (has_tb)
    {
        memcpy(args.data + args.len, TB.data, TB.len);
        args.len += TB.len;
        memcpy(args.data + args.len, ctx->base.tb, tb_len);
        args.len += tb_len;
    }

    if (!ctx->base.key_in_body)
    {
        memcpy(args.data + args.len, KEY.data, KEY.len);
        args.len += KEY.len;
        memcpy(args.data + args.len, ctx->base.key, key_len);
        args.len += key_len;
    }

    return args;
}

static ngx_int_t __write_binlog(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    ngx_bool_t alive = ngx_http_peer_is_alive(ctx->health_peer->peer);
    if (!alive)
    {
        return __write_sync_data(method, has_tb, r, ctx);
    }
    ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);

    ctx->state = STATE_WRITE_BINLOG;
    ctx->base.base.uri = mcf->binlog_uri;
    ctx->base.base.args = __format_binlog_args(method, has_tb, r, ctx);
    return ngx_http_run_subrequest(r, &ctx->base.base, ctx->health_peer->peer);
}

// -------------------------------------------------------------
// |skip   |  0   |  1   |  2   |  0       |  1   |  2  |  0   |
// -------------------------------------------------------------
// |err    |  0   |  0   |  0   |  1       |  1   |  0  |  2   |
// -------------------------------------------------------------
// |action |  200 |  200 |  404 |  200&log |  404 | 404 | 404  |
// -------------------------------------------------------------
static ngx_int_t __post_write_data(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    if (__should_write_log(ctx))
    {
        return __write_binlog(method, has_tb, r, ctx);
    }
    if (0 == ctx->skip_error_count && 0 == ctx->error_count)
    {
        return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->base.version, NULL, r);
    }
    return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
}

static ngx_int_t __on_write_master1_complete(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
	__update_error(method, r->headers_out.status, ctx);
	ctx->base.peer = ctx->base.peer->next;
	ngx_bool_t alive = ngx_http_peer_is_alive(ctx->base.peer->peer);
	ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);
	if (mcf->debug_sync)
	{
	    alive = false;
	}
	if (alive) // master2
	{
		ctx->state = STATE_WRITE_MASTER2;
		return ngx_http_run_subrequest(r, &ctx->base.base, ctx->base.peer->peer);
	}

	// master2 dead
	++ctx->error_count;
	ctx->error_peer = ctx->base.peer;

	return __post_write_data(method, has_tb, r, ctx);
}

static ngx_int_t __on_write_master2_complete(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    __update_error(method, r->headers_out.status, ctx);
    return __post_write_data(method, has_tb, r, ctx);
}

static ngx_int_t __on_write_binlog_complete(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        return __write_sync_data(method, has_tb, r, ctx);
    }
    if (!__add_sync_head(&ctx->error_peer->peer->server, r))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->base.version, NULL, r);
}

static ngx_int_t __switch_state(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    ngx_http_hustdb_ha_write_ctx_t * ctx)
{
    if (STATE_WRITE_MASTER1 == ctx->state)
    {
        return __on_write_master1_complete(method, has_tb, r, ctx);
    }
    else if (STATE_WRITE_MASTER2 == ctx->state)
    {
        return __on_write_master2_complete(method, has_tb, r, ctx);
    }
    else if (STATE_WRITE_BINLOG == ctx->state)
    {
        return __on_write_binlog_complete(method, has_tb, r, ctx);
    }
    return NGX_ERROR;
}

ngx_int_t hustdb_ha_write_handler(
    uint8_t method,
    ngx_bool_t support_post_only,
    ngx_bool_t key_in_body,
    ngx_bool_t has_tb,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r,
    hustdb_ha_start_write_t start_write)
{
    hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return start_write(support_post_only, key_in_body, has_tb, backend_uri, r);
    }
    return __switch_state(method, has_tb, r, &ctx->base);
}

static ngx_http_hustdb_ha_write_ctx_t * __create_zwrite_ctx(ngx_http_request_t *r)
{
    ngx_http_hustdb_ha_write_ctx_t * ctx = ngx_palloc(r->pool, sizeof(ngx_http_hustdb_ha_write_ctx_t));
    if (!ctx)
    {
        return NULL;
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(ngx_http_hustdb_ha_write_ctx_t));
    return ctx;
}

static void __post_body_handler(ngx_http_request_t *r)
{
    --r->main->count;

    ngx_http_hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ctx->base.key = hustdb_ha_get_key(r);
    if (!ctx->base.key)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        return;
    }

    ngx_http_gen_subrequest(
        ctx->base.base.backend_uri,
        r,
        ctx->base.peer->peer,
        &ctx->base.base,
        hustdb_ha_on_subrequest_complete);
}

static ngx_int_t __start_zwrite(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_hustdb_ha_write_ctx_t * ctx = __create_zwrite_ctx(r);
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    ctx->base.base.backend_uri = backend_uri;

    write_ctx_t tmp;
    if (!__parse_args(true, true, true, r, &tmp))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    __copy_data(&tmp, ctx);

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

ngx_int_t hustdb_ha_zwrite_handler(
    uint8_t method,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    ngx_http_hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __start_zwrite(backend_uri, r);
    }
    return __switch_state(method, true, r, ctx);
}

static ngx_int_t __send_write_cache_response(ngx_http_request_t *r, hustdb_ha_write_cache_ctx_t * ctx)
{
    if (0 == ctx->error_count)
    {
        return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
    }
    if (1 == ctx->error_count)
    {
        ngx_str_t * server = &ctx->error_peer->peer->server;
        static ngx_str_t FAIL_KEY = ngx_string("Fail");
        if (ngx_http_add_field_to_headers_out(&FAIL_KEY, server, r))
        {
            return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
        }
    }
    return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
}

static ngx_int_t __post_write_cache(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_write_cache_ctx_t * ctx = data;

    do
    {
        if (!ctx || NGX_HTTP_OK != r->headers_out.status)
        {
            break;
        }

        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;

    } while (0);

    return ngx_http_finish_subrequest(r);
}

static void __post_cache_handler(ngx_http_request_t *r)
{
    --r->main->count;
    hustdb_ha_write_cache_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ngx_http_gen_subrequest(
        ctx->base.backend_uri,
        r,
        ctx->peer->peer,
        &ctx->base,
        __post_write_cache);
}

static void __copy_args(const write_base_ctx_t * src, hustdb_ha_write_cache_ctx_t * dst)
{
    dst->peer = src->peer;
    dst->state = src->state;
    dst->error_count = src->error_count;
    dst->error_peer = src->error_peer;
}

static ngx_bool_t __parse_cache_args(
    hustdb_ha_check_parameter_t check,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r,
    hustdb_ha_write_cache_ctx_t * ctx)
{
    char * key = ngx_http_get_param_val(&r->args, "key", r->pool);
    if (!key)
    {
        return false;
    }

    if (check && !check(backend_uri, r))
    {
        return false;
    }

    ctx->peer = hustdb_ha_get_writelist(key);
    if (!ctx->peer)
    {
        return false;
    }

    write_base_ctx_t tmp;
    if (!__init_write_base_ctx(key, &tmp))
    {
        return false;
    }
    __copy_args(&tmp, ctx);

    return true;
}

static ngx_int_t __start_write_cache(hustdb_ha_check_parameter_t check, ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_write_cache_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_write_cache_ctx_t));
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(hustdb_ha_write_cache_ctx_t));
    ctx->base.backend_uri = backend_uri;

    if (!__parse_cache_args(check, backend_uri, r, ctx))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_cache_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

static void __update_cache_error(ngx_uint_t status, hustdb_ha_write_cache_ctx_t * ctx)
{
    if (NGX_HTTP_OK == status)
    {
        return;
    }
    ++ctx->error_count;
    ctx->error_peer = ctx->peer;
}

static ngx_int_t __on_write_cache_master1_complete(ngx_http_request_t *r, hustdb_ha_write_cache_ctx_t * ctx)
{
    __update_cache_error(r->headers_out.status, ctx);
    ctx->peer = ctx->peer->next;
    ngx_bool_t alive = ngx_http_peer_is_alive(ctx->peer->peer);
    if (alive) // master2
    {
        ctx->state = STATE_WRITE_MASTER2;
        return ngx_http_run_subrequest(r, &ctx->base, ctx->peer->peer);
    }

    // master2 dead
    ++ctx->error_count;
    ctx->error_peer = ctx->peer;

    return __send_write_cache_response(r, ctx);
}

static ngx_int_t __on_write_cache_master2_complete(ngx_http_request_t *r, hustdb_ha_write_cache_ctx_t * ctx)
{
    __update_cache_error(r->headers_out.status, ctx);
    return __send_write_cache_response(r, ctx);
}

ngx_bool_t hustdb_ha_has_tb(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static ngx_str_t arg = ngx_string("tb");
    ngx_str_t rc = ngx_null_string;
    return NGX_OK == ngx_http_arg(r, arg.data, arg.len, &rc);
}

ngx_bool_t hustdb_ha_has_ttl(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static ngx_str_t arg = ngx_string("ttl");
    ngx_str_t rc = ngx_null_string;
    return NGX_OK == ngx_http_arg(r, arg.data, arg.len, &rc);
}

ngx_bool_t hustdb_ha_check_incr(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!hustdb_ha_has_tb(backend_uri, r))
    {
        return false;
    }
    static ngx_str_t arg = ngx_string("val");
    ngx_str_t rc = ngx_null_string;
    return NGX_OK == ngx_http_arg(r, arg.data, arg.len, &rc);
}

ngx_int_t hustdb_ha_write_cache_handler(
    hustdb_ha_check_parameter_t check,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_write_cache_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __start_write_cache(check, backend_uri, r);
    }
    if (STATE_WRITE_MASTER1 == ctx->state)
    {
        return __on_write_cache_master1_complete(r, ctx);
    }
    else if (STATE_WRITE_MASTER2 == ctx->state)
    {
        return __on_write_cache_master2_complete(r, ctx);
    }
    return NGX_ERROR;
}
