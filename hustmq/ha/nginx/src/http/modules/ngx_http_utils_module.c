#include "ngx_http_utils_module.h"
#include <ngx_http_addon_def.h>

#if (NGX_HTTP_UPSTREAM_CHECK)
    #include "ngx_http_upstream_check_module.h"
#endif

ngx_http_request_item_t * ngx_http_get_request_item(
    ngx_http_request_item_t dict[],
    size_t size,
    const ngx_str_t * uri)
{
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        if (ngx_http_str_eq(uri, &dict[i].uri))
        {
            return dict + i;
        }
    }
    return NULL;
}

ngx_str_t ngx_http_get_conf_path(ngx_cycle_t * cycle, ngx_str_t * name)
{
    ngx_str_t path = { 0, 0 };
    if (!cycle || !name || !name->data)
    {
        return path;
    }
    ngx_str_t prefix = cycle->conf_prefix;
    size_t size = prefix.len + name->len;
    path.data = ngx_palloc(cycle->pool, size + 1);
    memcpy(path.data, prefix.data, prefix.len);
    memcpy(path.data + prefix.len, name->data, name->len);
    path.data[size] = '\0';
    path.len = size;
    return path;
}

int ngx_http_get_flag_slot(ngx_conf_t * cf)
{
    ngx_str_t * value = cf->args->elts;
    if (0 == ngx_strncmp(value[1].data, "on", value[1].len))
    {
        return 1;
    }
    else if (0 == ngx_strncmp(value[1].data, "off", value[1].len))
    {
        return 0;
    }
    return ngx_atoi(value[1].data, value[1].len);
}

ngx_bool_t ngx_http_str_eq(const ngx_str_t * src, const ngx_str_t * dst)
{
    if (dst->len != src->len)
    {
        return false;
    }
    return 0 == ngx_strncmp(src->data, dst->data, src->len);
}

ngx_str_t ngx_http_make_str(const ngx_str_t * str, ngx_pool_t * pool)
{
    ngx_str_t val = { 0, 0 };
    if (!str || !str->data || !pool)
    {
        return val;
    }
    val.data = ngx_palloc(pool, str->len + 1);
    if (!val.data)
    {
        return val;
    }
    val.len = str->len;
    memcpy(val.data, str->data, str->len);
    val.data[str->len] = '\0';

    return val;
}

char * ngx_http_execute(const char * cmd, ngx_pool_t * pool)
{
    FILE * f = popen(cmd, "r");
    if (!f)
    {
        return NULL;
    }
    enum { BUF_SIZE = 256 };
    char buf[BUF_SIZE + 1] = { 0 };
    char * output = NULL;
    size_t size = 0;
    while (!feof(f))
    {
        if (fgets(buf, BUF_SIZE, f))
        {
            size_t old_size = size;
            size_t len = strlen(buf);
            size = old_size + len;
            if (!output)
            {
                output = ngx_palloc(pool, size + 1);
                memcpy(output, buf, size);
                output[size] = '\0';
            }
            else
            {
                void * tmp = ngx_palloc(pool, old_size);
                memcpy(tmp, output, old_size);

                output = ngx_palloc(pool, size + 1);
                memcpy(output, tmp, old_size);
                memcpy(output + old_size, buf, len);
                output[size] = '\0';
            }
        }
    }
    pclose(f);
    return output;
}

ngx_str_t ngx_http_load_from_file(const char * path, ngx_pool_t * pool)
{
    ngx_str_t result = { 0, 0 };
    if (!path)
    {
        return result;
    }
    FILE * f = fopen(path, "rb");
    if (!f)
    {
        return result;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * data = ngx_palloc(pool, len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);

    result.data = (u_char *) data;
    result.len = len;
    return result;
}

ngx_bool_t ngx_http_save_to_file(const char * data, const char * path)
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

size_t ngx_http_get_buf_size(const ngx_buf_t * buf)
{
    if (!buf)
    {
        return 0;
    }
    return buf->last - buf->pos;
}

static char * __match_key(const ngx_str_t * str, const char * key, size_t key_size)
{
    if (!str || !str->data || !key)
    {
        return NULL;
    }
    char * pos = (char *)str->data;
    while (true)
    {
        pos = ngx_strstr(pos, key);
        if (!pos)
        {
            break;
        }
        size_t offset = (size_t)pos - (size_t)str->data;
        if (offset + key_size >= str->len)
        {
            break;
        }
        char * end = pos + key_size;
        if ('=' == *end)
        {
            return pos;
        }
        pos = pos + key_size;
    }
    return NULL;
}

ngx_http_str_pos_t ngx_http_match_key(const ngx_str_t * args, const char * key, size_t key_size)
{
    ngx_http_str_pos_t pos = { 0, 0 };
    if (!args || !args->data || !key)
    {
        return pos;
    }
    char * start = __match_key(args, key, key_size);
    if (!start)
    {
        return pos;
    }

    char * end = start + key_size;
    char * end_pos = (char *) (args->data + args->len);

    if (end_pos == end)
    {
        return pos;
    }

    while (end < end_pos)
    {
        ++end;
        if ('&' == *end)
        {
            break;
        }
    }

    pos.start = start;
    pos.end = end;

    return pos;
}

char * ngx_http_get_param_val(const ngx_str_t * args, const char * key, ngx_pool_t * pool)
{
    if (!key || !pool)
    {
        return NULL;
    }
    size_t key_size = strlen(key);
    ngx_http_str_pos_t pos = ngx_http_match_key(args, key, key_size);
    if (!pos.start || !pos.end)
    {
        return NULL;
    }

    pos.start += key_size + 1; // skip '='
    size_t len = pos.end - pos.start;
    char * val = ngx_palloc(pool, len + 1);
    memcpy(val, pos.start, len);
    val[len] = '\0';
    return val;
}

ngx_str_t ngx_http_get_empty_str(ngx_pool_t * pool)
{
    ngx_str_t str = ngx_null_string;
    if (!pool)
    {
        return str;
    }
    str.data = ngx_palloc(pool, 1);
    str.data[0] = '\0';
    return str;
}

static ngx_str_t __remove_first_param(const ngx_str_t * args, ngx_http_str_pos_t * pos, ngx_pool_t * pool)
{
    char * end_pos = (char *) (args->data + args->len);
    pos->end = pos->end + 1; // skip '&'
    size_t size = end_pos - pos->end;
    if (size < 1)
    {
        return *args;
    }
    ngx_str_t result;
    result.data = ngx_palloc(pool, size + 1);
    memcpy(result.data, pos->end, size);
    result.data[size] = '\0';
    result.len = size;
    return result;
}

static ngx_str_t __remove_last_param(const ngx_str_t * args, ngx_http_str_pos_t * pos, ngx_pool_t * pool)
{
    pos->start = pos->start - 1; // skip '&'
    size_t size = pos->start - (char *)args->data;
    if (size < 1)
    {
        return *args;
    }
    ngx_str_t result;
    result.data = ngx_palloc(pool, size + 1);
    memcpy(result.data, args->data, size);
    result.data[size] = '\0';
    result.len = size;
    return result;
}

static ngx_str_t __remove_mid_param(const ngx_str_t * args, ngx_http_str_pos_t * pos, ngx_pool_t * pool)
{
    char * end_pos = (char *) (args->data + args->len);
    pos->start = pos->start - 1; // skip '&'
    size_t left = (size_t)(pos->start - (char *)args->data);
    size_t right = (size_t)(end_pos - pos->end);
    size_t size = left + right;
    ngx_str_t result;
    result.data = ngx_palloc(pool, size + 1);
    if (left > 0)
    {
        memcpy(result.data, args->data, left);
    }
    if (right > 0)
    {
        memcpy(result.data + left, pos->end, right);
    }
    result.data[size] = '\0';
    result.len = size;
    return result;
}

ngx_str_t ngx_http_remove_param(const ngx_str_t * args, const ngx_str_t * key, ngx_pool_t * pool)
{
    ngx_str_t result = ngx_null_string;
    if (!pool)
    {
        return result;
    }

    ngx_http_str_pos_t pos = ngx_http_match_key(args, (const char *)key->data, key->len);
    if (!pos.start || !pos.end)
    {
        return *args;
    }

    char * end_pos = (char *) (args->data + args->len);
    if (pos.start == (char *)args->data && end_pos == pos.end) // key=val
    {
        return ngx_http_get_empty_str(pool);
    }
    else if (pos.start == (char *)args->data) // key=val&xxx=yyy
    {
        return __remove_first_param(args, &pos, pool);
    }
    else if (end_pos == pos.end) // xxx=yyy&key=val
    {
        return __remove_last_param(args, &pos, pool);
    }
    // xxx=yyy&key=val&mmm=nnn
    return __remove_mid_param(args, &pos, pool);
}

ngx_bool_t ngx_http_check_key(ngx_http_request_t *r)
{
    char * key = ngx_http_get_param_val(&r->args, "key", r->pool);
    if (!key)
    {
        return false;
    }

    long long unsigned int now = (long long unsigned int) time(NULL);
    now = now - now % 10;
    now = now << 3;

    char buf[128];
    sprintf(buf, "%llu", now);

    if (0 != strcmp(key, buf))
    {
        return false;
    }

    return true;
}

ngx_int_t __send_header(ngx_http_request_t *r, size_t len)
{
    static ngx_str_t type = ngx_string("text/plain");
    r->headers_out.content_type = type;
    r->headers_out.content_length_n = len;

    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;
    return ngx_http_send_header(r);
}

ngx_int_t __send_chain(ngx_http_request_t *r, size_t len, const char * response)
{
    ngx_buf_t * buf = ngx_create_temp_buf(r->pool, len);
    memcpy(buf->pos, response, len);
    buf->last = buf->pos + len;
    buf->last_buf = 1;

    ngx_chain_t out;
    out.buf = buf;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

ngx_int_t ngx_http_send_response_imp(ngx_uint_t status, const ngx_str_t * response, ngx_http_request_t *r)
{
    ngx_bool_t nobody = (!response || !response->data || response->len < 1);

    size_t len = nobody ? 0 : response->len;

    r->headers_out.status = status;
    ngx_int_t rc = __send_header(r, len);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        return rc;
    }

    return nobody ? ngx_http_output_filter(
            r, NULL) : __send_chain(
            r, len, (const char *)response->data);
}

ngx_buf_t * ngx_http_get_request_body(ngx_http_request_t * r)
{
    ngx_chain_t * chain = r->request_body->bufs;
    if (!chain)
    {
        return NULL;
    }
    ngx_buf_t * buf = ngx_create_temp_buf (r->pool, r->headers_in.content_length_n + 1);
    if (!buf)
    {
        return NULL;
    }
    ngx_memset(buf->start, '\0', r->headers_in.content_length_n + 1);
    while (chain && chain->buf)
    {
        off_t len = ngx_buf_size(chain->buf);
        if (len >= r->headers_in.content_length_n)
        {
            buf->start = buf->pos;
            buf->last = buf->pos;
            len = r->headers_in.content_length_n;
        }
        if (chain->buf->in_file)
        {
            ssize_t n = ngx_read_file(chain->buf->file, buf->start, len, 0);
            if (NGX_FILE_ERROR == n)
            {
                return NULL;
            }
            buf->last = buf->last + len;
            ngx_delete_file(chain->buf->file->name.data);
            chain->buf->file->fd = NGX_INVALID_FILE;
        }
        else
        {
            buf->last = ngx_copy(buf->start, chain->buf->pos, len);
        }
        chain = chain->next;
        buf->start = buf->last;
    }
    return buf;
}

ngx_str_t * ngx_http_find_head_value(const ngx_list_t * headers, const ngx_str_t * key)
{
    const ngx_list_part_t * part = &headers->part;
    ngx_table_elt_t * header = part->elts;
    size_t i;
    for (i = 0; /* void */; i++)
    {

        if (i >= part->nelts)
        {
            if (!part->next)
            {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0)
        {
            continue;
        }

        ngx_str_t * tmpkey = &header[i].key;
        ngx_str_t * value = &header[i].value;

        if (0 == ngx_strncmp(tmpkey->data, key->data, key->len))
        {
            return value;
        }
    }
    return NULL;
}

ngx_bool_t ngx_http_add_field_to_headers_out(const ngx_str_t * key, const ngx_str_t * val, ngx_http_request_t * r)
{
    if (!key || !val || !r)
    {
        return false;
    }
    ngx_table_elt_t * elem = ngx_list_push(&r->headers_out.headers);
    if (elem)
    {
        elem->hash = ngx_hash_key(key->data, key->len);

        elem->key.data = key->data;
        elem->key.len = key->len;

        elem->value.data = val->data;
        elem->value.len = val->len;
    }
    return true;
}

ngx_bool_t ngx_http_insert_head_to_body(const ngx_buf_t * head, ngx_http_request_t *r)
{
    if (!head || !r || !r->request_body)
    {
        return false;
    }
    size_t head_len = ngx_http_get_buf_size(head);

    off_t old_size = r->request_body->bufs ? r->headers_in.content_length_n : 0;

    ngx_chain_t * head_chain = ngx_alloc_chain_link(r->pool);
    if (!head_chain)
    {
        return false;
    }
    head_chain->buf = (ngx_buf_t *) head;
    head_chain->next = r->request_body->bufs;

    r->request_body->bufs = head_chain;
    r->request_body->buf = (ngx_buf_t *) head;

    r->headers_in.content_length_n = old_size + head_len;

    return true;
}

static void __ngx_strcat(const ngx_str_t * src, ngx_str_t * dst)
{
    memcpy(dst->data + dst->len, src->data, src->len);
    dst->len += src->len;
}

static ngx_bool_t __make_location(const ngx_str_t * head, ngx_http_request_t * r, size_t size, ngx_str_t * location)
{
    location->data = ngx_palloc(r->pool, size);
    location->len = 0;
    if (!location->data)
    {
        return false;
    }

    __ngx_strcat(head, location);
    __ngx_strcat(&r->args, location);
    char * buf = (char *) location->data;
    *(buf + location->len) = '\0';

    return true;
}

ngx_bool_t ngx_http_make_redirect(size_t location_size, const ngx_str_t * head, ngx_http_request_t * r)
{
    if (!r)
    {
        return false;
    }

    ngx_str_t location;
    if (!__make_location(head, r, location_size, &location))
    {
        return false;
    }

    ngx_http_clear_location(r);

    r->headers_out.location = ngx_list_push(&r->headers_out.headers);
    if (!r->headers_out.location)
    {
        return false;
    }

    r->headers_out.location->value.len = location.len;
    r->headers_out.location->value.data = location.data;
    r->headers_out.status = NGX_HTTP_TEMPORARY_REDIRECT;

    return true;
}

void ngx_http_post_body_handler(ngx_http_request_t * r, ngx_http_post_body_cb cb)
{
    int status = NGX_HTTP_BAD_REQUEST;
    do
    {
        ngx_buf_t * buf = ngx_http_get_request_body(r);
        if (!buf)
        {
            break;
        }

        size_t buf_size = ngx_http_get_buf_size(buf);
        if (buf_size < 1)
        {
            break;
        }

        u_char * end = buf->pos + buf_size;
        if (!end)
        {
            break;
        }
        u_char end_val = *end;
        *end = '\0';

        ngx_bool_t rc = cb ? cb(r, buf, buf_size) : false;

        *end = end_val;

        if (rc)
        {
            status = NGX_HTTP_OK;
        }
    } while (0);

    ngx_int_t rc = ngx_http_send_response_imp(status, NULL, r);
    ngx_http_finalize_request(r, rc);
}

ngx_int_t ngx_http_run_subrequest(
        ngx_http_request_t *r,
        ngx_http_subrequest_ctx_t * ctx,
        ngx_http_upstream_rr_peer_t * peer)
{
    ngx_http_request_t * psr;
    ngx_str_t * args = ctx->args.data ? &ctx->args : (r->args.len > 0 ? &r->args : NULL);
    ngx_int_t ret = ngx_http_subrequest(r, &ctx->uri, args, &psr, ctx->subrequest, NGX_HTTP_SUBREQUEST_IN_MEMORY);

    if (!psr)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_http_set_addon_module_ctx(psr, peer);

    if (NGX_OK != ret)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->main->count++;
    return NGX_DONE;
}

ngx_int_t ngx_http_gen_subrequest(
        ngx_str_t * backend_uri,
        ngx_http_request_t *r,
        ngx_http_upstream_rr_peer_t * peer,
        ngx_http_subrequest_ctx_t * ctx,
        ngx_http_post_subrequest_pt handler)
{
    ctx->backend_uri = backend_uri;
    ctx->subrequest = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (!ctx->subrequest)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ctx->subrequest->handler = handler;
    ctx->subrequest->data = ctx;

    ctx->uri.data = backend_uri->data;
    ctx->uri.len = backend_uri->len;
    ctx->response.data = NULL;
    ctx->response.len = 0;

    return ngx_http_run_subrequest(r, ctx, peer);
}

ngx_int_t ngx_http_finish_subrequest(ngx_http_request_t * r)
{
    if (r && r->parent)
    {
        r->parent->headers_out.status = r->headers_out.status;
        r->parent->write_event_handler = ngx_http_core_run_phases;
    }
    return NGX_OK;
}

ngx_int_t ngx_http_post_subrequest_handler(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    ngx_http_subrequest_ctx_t * ctx = data;
    if (ctx && NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->response.data = r->upstream->buffer.pos;
    }
    return ngx_http_finish_subrequest(r);
}

ngx_bool_t ngx_http_peer_is_alive(ngx_http_upstream_rr_peer_t * peer)
{
    if (!peer)
    {
        return false;
    }
#if (NGX_HTTP_UPSTREAM_CHECK)
    return !ngx_http_upstream_check_peer_down(peer->check_index);
#endif
    return true;
}

ngx_http_upstream_rr_peer_t * ngx_http_next_peer(ngx_http_upstream_rr_peer_t * peer)
{
    if (!peer || !peer->next)
    {
        return NULL;
    }
    do
    {
        peer = peer->next;
        if (ngx_http_peer_is_alive(peer))
        {
            return peer;
        }
    } while (peer->next);
    return NULL;
}

ngx_http_upstream_rr_peer_t * ngx_http_first_peer(ngx_http_upstream_rr_peer_t * peer)
{
    if (!peer)
    {
        return NULL;
    }
    if (ngx_http_peer_is_alive(peer))
    {
        return peer;
    }
    return ngx_http_next_peer(peer);
}

ngx_http_subrequest_peer_t * ngx_http_get_next_peer(ngx_http_subrequest_peer_t * peer)
{
    if (!peer || !peer->next)
    {
        return NULL;
    }
    do
    {
        peer = peer->next;
        if (ngx_http_peer_is_alive(peer->peer))
        {
            return peer;
        }
    } while(peer->next);

    return NULL;
}

ngx_http_subrequest_peer_t * ngx_http_get_first_peer(ngx_http_subrequest_peer_t * peer)
{
    if (!peer)
    {
        return NULL;
    }
    if (ngx_http_peer_is_alive(peer->peer))
    {
        return peer;
    }
    return ngx_http_get_next_peer(peer);
}

ngx_http_subrequest_peer_t * ngx_http_append_peer(
    ngx_pool_t * pool,
    ngx_http_upstream_rr_peer_t * peer,
    ngx_http_subrequest_peer_t * node)
{
    if (!pool || !peer || !node)
    {
        return NULL;
    }
    ngx_http_subrequest_peer_t * tmp = node->peer ? ngx_palloc(pool, sizeof(ngx_http_subrequest_peer_t)) : node;
    if (!tmp)
    {
        return NULL;
    }
    if (node->peer) // not head
    {
        node->next = tmp;
    }

    memset(tmp, 0, sizeof(ngx_http_subrequest_peer_t));

    tmp->peer = peer;
    tmp->data = NULL;
    tmp->next = NULL;
    return tmp;
}

ngx_http_upstream_rr_peer_t * ngx_http_peer_dict_get(ngx_http_peer_dict_t * dict, const char * key)
{
    ngx_http_upstream_rr_peer_t ** val = c_dict_get(dict, key);
    return val ? *val : 0;
}

ngx_bool_t ngx_http_init_peer_dict(ngx_http_upstream_rr_peers_t * peers, ngx_http_peer_dict_t * peer_dict)
{
    if (!peers || !peers->peer || !peer_dict)
    {
        return false;
    }
    c_dict_init(peer_dict);
    ngx_http_upstream_rr_peer_t * peer = peers->peer;
    while (peer)
    {
        c_dict_set(peer_dict, (const char *)peer->server.data, peer);
        peer = peer->next;
    }
    return true;
}

ngx_http_subrequest_peer_t * ngx_http_init_peer_list(ngx_pool_t * pool, ngx_http_upstream_rr_peers_t * peers)
{
    if (!pool || !peers || !peers->peer)
    {
        return NULL;
    }
    ngx_http_subrequest_peer_t * head = ngx_palloc(pool, sizeof(ngx_http_subrequest_peer_t));
    if (!head)
    {
        return NULL;
    }
    memset(head, 0, sizeof(ngx_http_subrequest_peer_t));

    ngx_http_subrequest_peer_t * node = head;

    ngx_http_upstream_rr_peer_t * peer = peers->peer;
    while (peer)
    {
        node = ngx_http_append_peer(pool, peer, node);
        if (!node)
        {
            return NULL;
        }
        peer = peer->next;
    }
    return head;
}

ngx_bool_t ngx_http_init_peer_array(
    ngx_pool_t * pool,
    ngx_http_upstream_rr_peers_t * peers,
    size_t size,
    ngx_http_peer_array_t * peer_array)
{
    if (!pool || !peers || !peers->peer || !peer_array || size < 1)
    {
        return false;
    }
    peer_array->size = 0;
    peer_array->arr = ngx_palloc(pool, size * sizeof(ngx_http_upstream_rr_peer_t *));
    ngx_http_upstream_rr_peer_t * peer = peers->peer;
    while (peer)
    {
        peer_array->arr[peer_array->size++] = peer;
        peer = peer->next;
    }
    return peer_array->size == size;
}

ngx_http_upstream_rr_peer_t * ngx_http_get_peer_by_index(ngx_http_peer_array_t * peer_array, size_t index)
{
    return index < peer_array->size ? peer_array->arr[index] : NULL;
}

static ngx_int_t ngx_http_addon_init_zone(ngx_shm_zone_t * shm_zone, void * data)
{
    ngx_http_addon_shm_ctx_t * octx = data;
    ngx_http_addon_shm_ctx_t * ctx = shm_zone->data;

    if (octx)
    {
        ctx->sh = octx->sh;
        ctx->shpool = octx->shpool;
        return NGX_OK;
    }

    ctx->shpool = (ngx_slab_pool_t *) shm_zone->shm.addr;
    if (shm_zone->shm.exists)
    {
        ctx->sh = ctx->shpool->data;
        return NGX_OK;
    }

    ctx->sh = ngx_slab_alloc(ctx->shpool, ctx->sizeof_sh);
    if (!ctx->sh)
    {
        return NGX_ERROR;
    }

    ctx->shpool->data = ctx->sh;

    if (NGX_OK != ctx->init_sh(ctx->shpool, ctx->sh))
    {
        return NGX_ERROR;
    }

    size_t len = sizeof(" in ngx_http_addon zone \"\"") + shm_zone->shm.name.len;
    ctx->shpool->log_ctx = ngx_slab_alloc(ctx->shpool, len);
    if (!ctx->shpool->log_ctx)
    {
        return NGX_ERROR;
    }
    return NGX_OK;
}

ngx_shm_zone_t * ngx_http_addon_init_shm(
    ngx_conf_t * cf,
    ngx_str_t * name,
    size_t shm_size,
    size_t sizeof_sh,
    ngx_http_addon_init_shm_ctx_t init_sh,
    void * module)
{
    ngx_http_addon_shm_ctx_t * ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_addon_shm_ctx_t));
    if (!ctx)
    {
        return NULL;
    }
    ctx->init_sh = init_sh;
    ctx->sizeof_sh = sizeof_sh;

    ngx_shm_zone_t * zone = ngx_shared_memory_add(cf, name, shm_size, module);
    if (!zone)
    {
        ngx_pfree(cf->pool, ctx);
        ctx = NULL;
        return NULL;
    }

    if (zone->data)
    {
        ctx = zone->data;
        return NULL;
    }

    zone->init = ngx_http_addon_init_zone;
    zone->data = ctx;

    return zone;
}
