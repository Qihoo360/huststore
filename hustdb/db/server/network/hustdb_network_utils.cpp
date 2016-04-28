#include "hustdb_network_utils.h"

namespace hustdb_network {

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

    hustdb_base64_encode(&src, &dst);

    c_str_t head = c_make_str("Basic ");
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

thread_dict_t::thread_dict_t(hustdb_network::mutex_t& m) : mutex(m), id(0)
{
}

thread_dict_t::~thread_dict_t()
{
    for (std::map<evthr_t *, item_t *>::iterator it = dict.begin(); it != dict.end(); ++it)
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
    dict[key] = new item_t(id, max_body_size);
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

bool __check_auth(evhtp_request_t * request, hustdb_network_ctx_t * data)
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

bool check_auth(evhtp_request_t * request, hustdb_network_ctx_t * data)
{
    if (!__check_auth(request, data))
    {
        evhtp_headers_add_header(request->headers_out, evhtp_header_new("Content-Length", "0", 1, 1));
        evhtp_headers_add_header(request->headers_out, evhtp_header_new("WWW-Authenticate", "Basic realm=\"HustDB\"", 1, 1));
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

void add_version(uint32_t ver, evhtp_request_t * request)
{
    add_numeric_kv("Version", ver, request);
}

void add_ver_err(item_ctxt_t * ctxt, evhtp_request_t * request)
{
    hustdb_network::add_kv("VerError", (ctxt && ctxt->is_version_error) ? "true" : "false", request);
}

void add_ver_err(bool is_version_error, evhtp_request_t * request)
{
    hustdb_network::add_kv("VerError", is_version_error ? "true" : "false", request);
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

void send_write_reply(evhtp_res code, uint32_t ver, item_ctxt_t * ctxt, evhtp_request_t * request)
{
    hustdb_network::add_version(ver, request);
    hustdb_network::add_ver_err(ctxt, request);
    hustdb_network::send_nobody_reply(code, request);
}

void send_write_reply(evhtp_res code, uint32_t ver, bool is_version_error, evhtp_request_t * request)
{
    hustdb_network::add_version(ver, request);
    hustdb_network::add_ver_err(is_version_error, request);
    hustdb_network::send_nobody_reply(code, request);
}

void add_uniq_sort (unsigned long start, unsigned long end, ip_allow_t * ip_allow_map)
{
    int i = 0;
    int pos = - 1;

    for ( i = 0; i < ip_allow_map->size; i ++ )
    {
        if ( start == ip_allow_map->ip_map[ i ].start )
        {
            if ( end > ip_allow_map->ip_map[ i ].end )
            {
                ip_allow_map->ip_map[ i ].end = end;
            }

            return;
        }

        if ( start < ip_allow_map->ip_map[ i ].start )
        {
            pos = i;
            break;
        }
    }

    if ( pos < 0 )
    {
        ip_allow_map->ip_map[ ip_allow_map->size ].start = start;
        ip_allow_map->ip_map[ ip_allow_map->size ].end = end;
        ip_allow_map->size ++;
        return;
    }

    for ( i = ip_allow_map->size; i > pos; i -- )
    {
        ip_allow_map->ip_map[ i ] = ip_allow_map->ip_map[ i - 1 ];
    }

    ip_allow_map->ip_map[ pos ].start = start;
    ip_allow_map->ip_map[ pos ].end = end;
    ip_allow_map->size ++;
}

bool get_ip_allow_map(const char * ip_allow_string, unsigned int ip_allow_string_length, ip_allow_t * ip_allow_map)
{
    int i = 0;
    int pos = 0;
    char flag = ',';
    unsigned long prev_ip = 0;
    unsigned long cur_ip = 0;
    char ip_string [16] = { };

    if ( ! ip_allow_string || ip_allow_string_length <= 0 || ip_allow_string_length > 8192 )
    {
        return false;
    }

    for ( i = 0; i < ip_allow_string_length; i ++ )
    {
        if ( ! ( ( ip_allow_string[i] >= '0' ) && ( ip_allow_string[i] <= '9' ) ) && ip_allow_string[i] != '-' && ip_allow_string[i] != '.' && ip_allow_string[i] != ',' )
        {
            return false;
        }
    }

    for ( i = 0; i < ip_allow_string_length && ip_allow_map->size < 1023; i ++ )
    {
        if ( ip_allow_string[ i ] == ',' || ip_allow_string[ i ] == '-' )
        {
            memset (ip_string, 0, sizeof ( ip_string ));
            memcpy (ip_string, ip_allow_string + pos, i - pos);

            cur_ip = ntohl (inet_addr (ip_string));

            if ( flag == ',' && ip_allow_string[ i ] == ',' )
            {
                add_uniq_sort (cur_ip, cur_ip, ip_allow_map);
            }
            else if ( flag == '-' )
            {
                add_uniq_sort (prev_ip, cur_ip, ip_allow_map);
            }

            pos = i + 1;
            flag = ip_allow_string[ i ];
            prev_ip = cur_ip;
        }
    }

    memset (ip_string, 0, sizeof ( ip_string ));
    memcpy (ip_string, ip_allow_string + pos, i - pos);

    cur_ip = ntohl (inet_addr (ip_string));

    if ( flag == ',' )
    {
        add_uniq_sort (cur_ip, cur_ip, ip_allow_map);
    }
    else if ( flag == '-' )
    {
        add_uniq_sort (prev_ip, cur_ip, ip_allow_map);
    }

    return true;
}

bool can_access (struct sockaddr_in * addr, ip_allow_t * ip_allow_map)
{
    unsigned long ip = ntohl (addr->sin_addr.s_addr);

    int low = 0;
    int mid = 0;
    int high = ip_allow_map->size - 1;
    while ( low <= high )
    {
        mid = ( low + high ) / 2;
        if ( ip_allow_map->ip_map[ mid ].start <= ip && ip_allow_map->ip_map[ mid ].end >= ip )
        {
            return true;
        }
        else if ( ip_allow_map->ip_map[ mid ].start > ip )
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }

    return false;
}

} // hustdb_network

hustdb_network_ctx_t::hustdb_network_ctx_t()
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
      ip_allow_map(0),
      db(0),
      dict(mutex)
{
    memset(&recv_timeout, 0, sizeof(struct timeval));
    memset(&send_timeout, 0, sizeof(struct timeval));
}

uint32_t hustdb_network_ctx_t::get_id(evhtp_request_t * request)
{
    if (!request || !request->conn)
    {
        return 0;
    }
    return dict.get_id(request->conn->thread);
}

c_str_t hustdb_network_ctx_t::get_body(evhtp_request_t * request)
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

void hustdb_network_ctx_t::append(evthr_t * thr)
{
    if (!thr)
    {
        return;
    }
    dict.append(thr, max_body_size);
}
