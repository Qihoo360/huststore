#include "libevhtp_utils.h"
#define c_base64_encoded_length(len)  (((len + 2) / 3) * 4)

namespace evhtp {

void c_str_t::assign(char * data, size_t len)
{
    this->data = data;
    this->len = len;
}

void base64_encode_internal(const c_str_t *src, const uint8_t *basis, uintptr_t padding, c_str_t *dst)
{
    uint8_t *d, *s;
    size_t len;

    len = src->len;
    s = (uint8_t *)src->data;
    d = (uint8_t *)dst->data;

    while (len > 2)
    {
        *d++ = basis[(s[0] >> 2) & 0x3f];
        *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len)
    {
        *d++ = basis[(s[0] >> 2) & 0x3f];

        if (len == 1)
        {
            *d++ = basis[(s[0] & 3) << 4];
            if (padding)
            {
                *d++ = '=';
            }

        }
        else
        {
            *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis[(s[1] & 0x0f) << 2];
        }

        if (padding)
        {
            *d++ = '=';
        }
    }

    dst->len = d - (uint8_t *)dst->data;
}

void __base64_encode(const c_str_t *src, c_str_t *dst)
{
    if (!src || !src->data || !dst || !dst->data)
    {
        return;
    }
    static uint8_t basis64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    base64_encode_internal(src, basis64, 1, dst);
}

http_basic_auth_t::http_basic_auth_t(const char * username, const char * passwd)
{
    memset(&src, 0, sizeof(c_str_t));
    memset(&dst, 0, sizeof(c_str_t));
    memset(&auth, 0, sizeof(c_str_t));

    size_t ulen = strlen(username);
    size_t plen = strlen(passwd);
    if (ulen < 1 || plen < 1)
    {
        return;
    }

    src.len = ulen + plen + 1;
    src.data = new char[src.len + 1];
    if (!src.data)
    {
        return;
    }
    memcpy(src.data, username, ulen);
    char dim = ':';
    memcpy(src.data + ulen, &dim, 1);
    memcpy(src.data + ulen + 1, passwd, plen);
    src.data[src.len] = '\0';

    dst.len = c_base64_encoded_length(src.len);
    dst.data = new char[dst.len + 1];
    if (!dst.data)
    {
        return;
    }
    memset(dst.data, 0, dst.len + 1);

    __base64_encode(&src, &dst);

    c_str_t head = evhtp_make_str("Basic ");
    auth.len = head.len + dst.len;
    auth.data = new char[auth.len + 1];
    if (!auth.data)
    {
        return;
    }
    memcpy(auth.data, head.data, head.len);
    memcpy(auth.data + head.len, dst.data, dst.len);
    auth.data[auth.len] = '\0';
}

http_basic_auth_t::~http_basic_auth_t()
{
    if (src.data)
    {
        delete [] src.data;
    }
    if (dst.data)
    {
        delete [] dst.data;
    }
    if (auth.data)
    {
        delete [] auth.data;
    }
}

const char * http_basic_auth_t::c_str()
{
    return auth.data;
}

bool __create_mutex(pthread_mutex_t * mtx)
{
    pthread_mutexattr_t attr;
    int err = pthread_mutexattr_init(&attr);
    if (err != 0)
    {
        return false;
    }

    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0)
    {
        return false;
    }

    err = pthread_mutex_init(mtx, &attr);
    if (err != 0)
    {
        return false;
    }

    pthread_mutexattr_destroy(&attr);
    return true;
}

bool __destroy_mutex(pthread_mutex_t * mtx)
{
    return 0 == pthread_mutex_destroy(mtx);
}

bool __lock(pthread_mutex_t * mtx)
{
    return 0 == pthread_mutex_lock(mtx);
}

bool __unlock(pthread_mutex_t * mtx)
{
    return 0 == pthread_mutex_unlock(mtx);
}

mutex_t::mutex_t()
{
    __create_mutex(&mutex);
}

mutex_t::~mutex_t()
{
    __destroy_mutex(&mutex);
}

bool mutex_t::lock()
{
    return __lock(&mutex);
}

bool mutex_t::unlock()
{
    return __unlock(&mutex);
}

buf_t::buf_t(uint32_t _id, size_t max_body_size) : id(_id), buf(0)
{
    buf = new char[max_body_size];
}

buf_t::~buf_t()
{
    if (buf)
    {
        delete [] buf;
        buf = 0;
    }
}

thread_dict_t::thread_dict_t(mutex_t& m) : mutex(m), id(0)
{
}

thread_dict_t::~thread_dict_t()
{
    for (std::map<evthr_t *, buf_t *>::iterator it = dict.begin(); it != dict.end(); ++it)
    {
        if (it->second)
        {
            delete it->second;
            it->second = NULL;
        }
    }
}

void thread_dict_t::append(evthr_t * key, size_t max_body_size)
{
    if (!key)
    {
        return;
    }
    locker_t locker(mutex);
    dict[key] = new buf_t(id, max_body_size);
    ++id;
}

uint32_t thread_dict_t::get_id(evthr_t * key)
{
    return dict[key]->id;
}

char * thread_dict_t::get_buf(evthr_t * key)
{
    return dict[key]->buf;
}

bool __check_auth(evhtp_request_t * request, conf_t * data)
{
    if (!request || !data)
    {
        return false;
    }
    if (!data->auth)
    {
        return true;
    }
    const char * auth = evhtp_header_find(request->headers_in, "Authorization");
    if (!auth)
    {
        return false;
    }
    if (0 != strcmp(auth, data->auth))
    {
        return false;
    }
    return true;
}

bool check_auth(evhtp_request_t * request, conf_t * data)
{
    if (!__check_auth(request, data))
    {
        evhtp_headers_add_header(request->headers_out, evhtp_header_new("Content-Length", "0", 1, 1));
        evhtp_headers_add_header(request->headers_out, evhtp_header_new("WWW-Authenticate", "Basic realm=\"libevhtp\"", 1, 1));
        evhtp_send_reply(request, EVHTP_RES_UNAUTH);
        return false;
    }
    return true;
}

void send_reply(evhtp_res code, evhtp_request_t * request)
{
    evhtp_headers_add_header(request->headers_out, evhtp_header_new("Content-Length", "0", 1, 1));
    evhtp_send_reply(request, code);
}

void invalid_method(evhtp_request_t * request)
{
    send_reply(EVHTP_RES_METHNALLOWED, request);
}

void add_numeric_kv(const char * key, uint32_t val, evhtp_request_t * request)
{
    char tmp[11] = { 0 };
    sprintf(tmp, "%u", val);
    evhtp_headers_add_header(request->headers_out, evhtp_header_new(key, tmp, 1, 1));
}

void add_kv(const char * key, const char * val, evhtp_request_t * request)
{
    evhtp_headers_add_header(request->headers_out, evhtp_header_new(key, val, 1, 1));
}

void send_nobody_reply(evhtp_res code, evhtp_request_t * request)
{
    evhtp_headers_add_header(request->headers_out, evhtp_header_new("Content-Length", "0", 1, 1));
    evhtp_send_reply(request, code);
}

void send_reply(evhtp_res code, const char * data, size_t len, evhtp_request_t * request)
{
    if (!data || len < 1)
    {
        send_nobody_reply(code, request);
    }
    else
    {
        evbuffer_add(request->buffer_out, data, len);
        evhtp_send_reply(request, code);
    }
}

conf_t::conf_t()
    : threads(0),
      port(0),
      backlog(0),
      max_keepalive_requests(0),
      max_body_size(0),
      disable_100_cont(0),
      enable_reuseport(0),
      enable_nodelay(0),
      enable_defer_accept(0),
      auth(0),
      dict(mutex)
{
    memset(&recv_timeout, 0, sizeof(struct timeval));
    memset(&send_timeout, 0, sizeof(struct timeval));
}

uint32_t conf_t::get_id(evhtp_request_t * request)
{
    if (!request || !request->conn)
    {
        return 0;
    }
    return dict.get_id(request->conn->thread);
}

c_str_t conf_t::get_body(evhtp_request_t * request)
{
    c_str_t body = { 0, 0 };
    if (!request || !request->buffer_in)
    {
        return body;
    }
    size_t len = evbuffer_get_length(request->buffer_in);
    if (len < 1 || len > max_body_size)
    {
        return body;
    }
    body.data = dict.get_buf(request->conn->thread);
    if (!body.data)
    {
        return body;
    }
    body.len = len;
    evbuffer_copyout(request->buffer_in, body.data, body.len);
    return body;
}

void conf_t::append(evthr_t * thr)
{
    if (!thr)
    {
        return;
    }
    dict.append(thr, max_body_size);
}

}
