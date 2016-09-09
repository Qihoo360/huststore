#include "husthttp.h"
#include <cstdio>

husthttp_t::husthttp_t ( )
: m_h ( NULL )
, m_method ( )
, m_path ( )
, m_query_string ( )
, m_data ( )
, m_errno ( 0 )
, m_http_code ( 0 )
, m_body ( NULL )
, m_head ( NULL )
, m_conn_timeout ( 5 )
, m_recv_timeout ( 60 )
{
}

husthttp_t::husthttp_t ( std::string & host )
: m_h ( NULL )
, m_host ( host )
, m_method ( )
, m_path ( )
, m_query_string ( )
, m_data ( )
, m_errno ( 0 )
, m_http_code ( 0 )
, m_body ( NULL )
, m_head ( NULL )
, m_conn_timeout ( 5 )
, m_recv_timeout ( 60 )
{
}

husthttp_t::~ husthttp_t ( )
{
    if ( m_body )
    {
        delete m_body;
        m_body = NULL;
    }

    if ( m_head )
    {
        delete m_head;
        m_head = NULL;
    }

    if ( m_h )
    {
        http_destroy ( m_h );
    }
}

bool husthttp_t::open (
                        const char * method,
                        const char * path,
                        const char * query_string,
                        const char * data,
                        size_t data_len,
                        int conn_timeout,
                        int recv_timeout
                        )
{
    if ( unlikely ( ! m_h ) )
    {
        m_h = http_create ();
        if ( ! m_h )
        {
            return false;
        }
    }

    if ( unlikely ( ! method || ! path ) )
    {
        return false;
    }

    if ( m_method.compare ( method ) != 0 )
    {
        m_method = method;
    }

    if ( m_path.compare ( path ) != 0 )
    {
        m_path = path;
    }

    if ( ! query_string )
    {
        m_query_string.resize ( 0 );
    }
    else if ( m_query_string.compare ( query_string ) != 0 )
    {
        m_query_string = query_string;
    }

    m_data.resize ( 0 );
    if ( data_len > 0 )
    {
        m_data.assign (( const char * ) data, ( const char * ) ( data + data_len ));
    }

    if ( conn_timeout > 0 )
    {
        m_conn_timeout = conn_timeout;
    }

    if ( recv_timeout > 0 )
    {
        m_recv_timeout = recv_timeout;
    }

    return true;
}

bool husthttp_t::open2 (
                         std::string & method,
                         std::string & path,
                         std::string & query_string,
                         std::string & data,
                         int conn_timeout,
                         int recv_timeout
                         )
{
    if ( unlikely ( ! m_h ) )
    {
        m_h = http_create ();
        if ( ! m_h )
        {
            return false;
        }
    }

    if ( unlikely ( method.empty () || path.empty () ) )
    {
        return false;
    }

    if ( m_method.compare ( method ) != 0 )
    {
        m_method = method;
    }

    if ( m_path.compare ( path ) != 0 )
    {
        m_path = path;
    }

    if ( query_string.size () <= 0 )
    {
        m_query_string.resize ( 0 );
    }
    else if ( m_query_string.compare ( query_string ) != 0 )
    {
        m_query_string = query_string;
    }

    m_data.resize ( 0 );
    if ( data.size () > 0 )
    {
        m_data = data;
    }

    if ( conn_timeout > 0 )
    {
        m_conn_timeout = conn_timeout;
    }

    if ( recv_timeout > 0 )
    {
        m_recv_timeout = recv_timeout;
    }

    return true;
}

struct cb_param_t
{
    http_t * h;
    std::stringstream * data;
} ;

static size_t husthttp_callback (
                                  const void * data,
                                  size_t always_1,
                                  size_t data_bytes,
                                  void * param
                                  )
{
    cb_param_t * cb = ( cb_param_t * ) param;
    cb->data->write (( const char * ) data, ( std::streamsize )data_bytes);
    return data_bytes;
}

bool husthttp_t::process ( int * _errno )
{
    if ( unlikely ( ! m_h ) )
    {
        return false;
    }

    http_t * h = m_h;

    if ( m_body )
    {
        delete m_body;
        m_body = NULL;
    }
    m_body = new std::stringstream;

    if ( m_head )
    {
        delete m_head;
        m_head = NULL;
    }
    m_head = new std::stringstream;

    m_http_code = 0;
    m_errno = 0;
    do
    {
        std::string url;
        url.reserve ( 1024 );
        url += "http://";
        if ( ! m_host.empty () )
        {
            url += m_host;
        }
        url += m_path;
        if ( ! m_query_string.empty () )
        {
            url += "?";
            url += m_query_string;
        }

        http_reset_request (h);

        if ( ! http_set_url (h, url.c_str (), NULL) )
        {
            m_errno = - 5;
            break;
        }

        if ( ! http_set_timeout (h, m_conn_timeout, m_recv_timeout) )
        {
            m_errno = - 25;
            break;
        }

        cb_param_t body_param;
        body_param.h = h;
        body_param.data = m_body;
        cb_param_t head_param;
        head_param.h = h;
        head_param.data = m_head;
        if ( ! http_set_callback (h, husthttp_callback, & body_param, husthttp_callback, & head_param) )
        {
            m_errno = - 3;
            break;
        }

        if ( ! m_data.empty () )
        {
            if ( ! http_set_postfields (h, m_data.c_str (), m_data.size (), "text/plain") )
            {
                m_errno = - 55;
                break;
            }
        }

        int & http_code = m_http_code;
        if ( ! http_perform (h, HTTP_KEEP_ALIVE, m_method.c_str (), & http_code) )
        {
            m_errno = - 6;
            break;
        }
    }
    while ( 0 );
    *_errno = m_errno;
    return 0 == m_errno;
}

bool husthttp_t::get_response ( std::string & body, std::string & head, int & http_code )
{
    http_code = m_http_code;

    if ( 0 != m_errno || NULL == m_body || NULL == m_head )
    {
        body.resize (0);
        head.resize (0);
        return false;
    }

    head = m_head->str ();
    if ( head.empty () )
    {
        return false;
    }

    body = m_body->str ();

    return true;
}

void husthttp_t::set_host ( const char * host, int host_len )
{
    m_host.resize ( 0 );
    m_host.assign ( host, host + host_len );
}

const char * husthttp_t::get_host ( )
{
    return m_host.c_str ();
}
