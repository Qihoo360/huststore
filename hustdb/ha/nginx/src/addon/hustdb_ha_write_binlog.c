#include "hustdb_ha_write_inner.h"

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

ngx_bool_t hustdb_ha_add_sync_head(const ngx_str_t * server, ngx_http_request_t *r)
{
    static ngx_str_t SYNC_KEY = ngx_string("Sync");
    return ngx_http_add_field_to_headers_out(&SYNC_KEY, server, r);
}


ngx_int_t hustdb_ha_write_sync_data(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
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

        if (!hustdb_ha_add_sync_head(server, r))
        {
            break;
        }

        return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->base.version, NULL, r);
    } while (0);

    return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
}

static ngx_str_t __format_binlog_args(uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
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

ngx_int_t hustdb_ha_write_binlog(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
{
    ngx_bool_t alive = ngx_http_peer_is_alive(ctx->health_peer->peer);
    if (!alive)
    {
        return hustdb_ha_write_sync_data(method, has_tb, r, ctx);
    }
    ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);

    ctx->state = STATE_WRITE_BINLOG;
    ctx->base.base.uri = mcf->binlog_uri;
    ctx->base.base.args = __format_binlog_args(method, has_tb, r, ctx);
    return ngx_http_run_subrequest(r, &ctx->base.base, ctx->health_peer->peer);
}
