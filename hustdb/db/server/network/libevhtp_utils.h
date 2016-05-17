#ifndef __libevhtp_utils_20160510192445_h__
#define __libevhtp_utils_20160510192445_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include <evhtp.h>

#define evhtp_make_str(s) { sizeof(s) - 1, (char *) s }

namespace evhtp {

struct c_str_t
{
    size_t len;
    char * data;
    void assign(char * data, size_t len);
};

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
    thread_dict_t(mutex_t& m);
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
    mutex_t& mutex;
    std::map<evthr_t *, item_t *> dict;
    uint32_t id;
};

struct conf_t
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

    conf_t();
    ~conf_t() {}

    uint32_t get_id(evhtp_request_t * request);
    c_str_t get_body(evhtp_request_t * request);
    void append(evthr_t * thr);
private:
    mutex_t mutex;
    thread_dict_t dict;
};

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

bool check_auth(evhtp_request_t * request, conf_t * data);
void invalid_method(evhtp_request_t * request);
void send_reply(evhtp_res code, evhtp_request_t * request);
void add_numeric_kv(const char * key, uint32_t val, evhtp_request_t * request);
void add_kv(const char * key, const char * val, evhtp_request_t * request);
void send_nobody_reply(evhtp_res code, evhtp_request_t * request);
void send_reply(evhtp_res code, const char * data, size_t len, evhtp_request_t * request);

}

#endif // __libevhtp_utils_20160510192445_h__
