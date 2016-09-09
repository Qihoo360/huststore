#ifndef _husthttp_h_
#define _husthttp_h_

#include "http.h"

#include <string>
#include <sstream>

class husthttp_t
{
public:

    bool process ( int * );
    bool get_response ( std::string & body, std::string & head, int & http_code );
    void set_host ( const char * host, int host_len );
    const char * get_host ( );

public:

    husthttp_t ( );
    husthttp_t ( std::string & host );
    ~husthttp_t ( );

    bool open (
        const char * method,
        const char * path,
        const char * query_string,
        const char * data,
        size_t data_len,
        int conn_timeout,
        int recv_timeout
    );

    bool open2 (
        std::string & method,
        std::string & path,
        std::string & query_string,
        std::string & data,
        int conn_timeout,
        int recv_timeout
    );

private:

    http_t * m_h;

    std::string m_host;
    std::string m_method;
    std::string m_path;
    std::string m_query_string;
    std::string m_data;

    int m_errno;
    int m_http_code;

    std::stringstream * m_body;
    std::stringstream * m_head;

    int m_conn_timeout;
    int m_recv_timeout;

private:
    // disable
    husthttp_t ( const husthttp_t & );
    const husthttp_t & operator= ( const husthttp_t & );
};

#endif // #ifndef _husthttp_h_
