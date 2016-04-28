#ifndef __hustdb_network_utils_20160314135056_h__
#define __hustdb_network_utils_20160314135056_h__

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include <evhtp.h>
#include "hustdb_utils.h"
#include "hustdb.h"

namespace hustdb_network {

struct mutex_t
{
    mutex_t();
    ~mutex_t();

    bool lock();
    bool unlock();
private:
    pthread_mutex_t mutex;
};

struct locker_t
{
    locker_t(mutex_t& obj) : locker(obj)
    {
        locker.lock();
    }
    ~locker_t()
    {
        locker.unlock();
    }
private:
    mutex_t& locker;
};

struct thread_dict_t
{
    thread_dict_t(hustdb_network::mutex_t& m);
    ~thread_dict_t();
    void append(evthr_t * key, size_t max_body_size);
    uint32_t get_id(evthr_t * key);
    char * get_buf(evthr_t * key);
private:
    struct item_t
    {
        uint32_t id;
        char * buf;
        item_t(uint32_t _id, size_t max_body_size) : id(_id), buf(0)
        {
            buf = new char[max_body_size];
        }
        ~item_t()
        {
            if (buf)
            {
                delete [] buf;
                buf = 0;
            }
        }
    };
    hustdb_network::mutex_t& mutex;
    std::map<evthr_t *, item_t *> dict;
    uint32_t id;
};

typedef struct
{
    unsigned long start;
    unsigned long end;
} ip_t;

typedef struct
{
    int size;
    ip_t ip_map [ 1024 ];
} ip_allow_t;

}

struct hustdb_network_ctx_t
{
    // public
    int threads;
    uint16_t port;
    int backlog;
    uint64_t max_keepalive_requests;
    uint64_t max_body_size;
    bool disable_100_cont;
    bool enable_reuseport;
    bool enable_nodelay;
    bool enable_defer_accept;
    struct timeval recv_timeout;
    struct timeval send_timeout;
    const char * auth;
    hustdb_network::ip_allow_t * ip_allow_map;
    hustdb_t * db;

    hustdb_network_ctx_t();
    ~hustdb_network_ctx_t() {}

    uint32_t get_id(evhtp_request_t * request);
    c_str_t get_body(evhtp_request_t * request);
    void append(evthr_t * thr);
private:
    hustdb_network::mutex_t mutex;
    hustdb_network::thread_dict_t dict;
};

namespace hustdb_network {

struct http_basic_auth_t
{
    http_basic_auth_t(const char * username, const char * passwd);
    ~http_basic_auth_t();
    const char * c_str();
private:
    c_str_t src;
    c_str_t dst;
    c_str_t auth;
};

template <typename T>
std::string to_string(const T& obj_val)
{
    std::stringstream ss;
    ss << obj_val;
    return ss.str();
}

template <typename T>
T cast(const std::string& str)
{
    std::stringstream ss(str);
    T result;
    return ss >> result ? result : 0;
}

bool check_auth(evhtp_request_t * request, hustdb_network_ctx_t * data);
void invalid_method(evhtp_request_t * request);
void send_reply(evhtp_res code, evhtp_request_t * request);
void add_numeric_kv(const char * key, uint32_t val, evhtp_request_t * request);
void add_version(uint32_t ver, evhtp_request_t * request);
void add_ver_err(item_ctxt_t * ctxt, evhtp_request_t * request);
void add_ver_err(bool is_version_error, evhtp_request_t * request);
void add_kv(const char * key, const char * val, evhtp_request_t * request);
void send_nobody_reply(evhtp_res code, evhtp_request_t * request);
void send_reply(evhtp_res code, const char * data, size_t len, evhtp_request_t * request);
void send_write_reply(evhtp_res code, uint32_t ver, item_ctxt_t * ctxt, evhtp_request_t * request);
void send_write_reply(evhtp_res code, uint32_t ver, bool is_version_error, evhtp_request_t * request);
bool get_ip_allow_map(const char * ip_allow_string, unsigned int ip_allow_string_length, ip_allow_t * ip_allow_map);
bool can_access (struct sockaddr_in * addr, ip_allow_t * ip_allow_map);

}

#endif // __hustdb_network_utils_20160314135056_h__
