#include "http.h"
#include <unistd.h>
#include <cstring>

#define CURL_STATICLIB
#include <curl/curl.h>

#define HEADER_MAX  100
#define QUERY_MAX   100
#define FORM_MAX    100

#define gr_url_decode   url_decode
#define gr_memrchr      memrchr
#define mem5cmp         fast_mem5cmp
#define mem2cmp         fast_mem2cmp
#define mem6cmp         fast_mem6cmp


#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct curl_httppost curl_httppost;
    typedef struct curl_slist curl_slist;

    struct http_t
    {
        int connect_timeout_second;
        int recv_timeout_second;
        http_data_callback_t content_callback;
        void * content_callback_param;
        http_data_callback_t header_callback;
        void * header_callback_param;

        char content_type[ 128 ];
        bool keep_alive;

        CURL * curl;
        curl_slist * header;
        curl_httppost * formpost;
        curl_httppost * formpost_lastptr;
    };

#define IS_PARSE_HTTP_POST_DATA     1

#define MULTIPART_FORMDATA_LEAD     "multipart/form-data; boundary="
#define MULTIPART_FORMDATA_LEAD_LEN (sizeof(MULTIPART_FORMDATA_LEAD) - 1)

    bool http_global_init ( )
    {
        CURLcode r;
        r = curl_global_init (CURL_GLOBAL_ALL);
        if ( CURLE_OK == r )
        {
            return true;
        }

        return false;
    }

    void http_global_term ( )
    {
        curl_global_cleanup ();
    }

    http_t *
    http_create ( )
    {
        CURL * c;
        http_t * h;

        c = curl_easy_init ();
        if ( unlikely (! c) )
        {
            return NULL;
        }

        h = ( http_t * ) calloc (1, sizeof ( http_t ));
        if ( unlikely (! h) )
        {
            curl_easy_cleanup (c);
            return NULL;
        }

        h->keep_alive = true; ///////////////////////////////keep_alive
        h->connect_timeout_second = - 1;
        h->recv_timeout_second = - 1;

        h->curl = c;

        h->header = curl_slist_append (h->header, "Connection: close");
        h->header = curl_slist_append (h->header, "Expect:");
        if ( unlikely (! h->header) )
        {
            curl_easy_cleanup (c);
            return NULL;
        }

        return h;
    }

    static inline
    void http_clear_form (
                           http_t * http
                           )
    {
        if ( http )
        {
            if ( http->formpost )
            {
                curl_formfree (http->formpost);
            }
            http->formpost = NULL;
            http->formpost_lastptr = NULL;
        }
    }

    void
    http_destroy (
                   http_t * http
                   )
    {
        if ( http )
        {
            http_clear_form (http);
            curl_slist_free_all (http->header);
            curl_easy_cleanup (http->curl);
            free (http);
        }
    }

    bool
    http_header_reset (
                        http_t * http
                        )
    {
        if ( http )
        {
            if ( http->header )
            {
                curl_slist_free_all (http->header);
                http->header = NULL;
            }

            http->header = curl_slist_append (http->header, "Expect:");

            if ( '\0' != http->content_type[ 0 ] )
            {
                char buf[ 256 ] = "";
                sprintf (buf, "Content-Type: %s", http->content_type);
                http->header = curl_slist_append (http->header, buf);
                if ( unlikely (! http->header) )
                {
                    return false;
                }
            }

            if ( http->keep_alive )
            {
                http->header = curl_slist_append (http->header, "Connection: Keep-Alive");
                if ( unlikely (! http->header) )
                {
                    return false;
                }
            }
            else
            {
                http->header = curl_slist_append (http->header, "Connection: close");
                if ( unlikely (! http->header) )
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool
    http_set_timeout (
                       http_t * http,
                       int connect_timeout_second,
                       int recv_timeout_second
                       )
    {
        if ( unlikely (! http || connect_timeout_second <= 0 || recv_timeout_second <= 0) )
        {
            return false;
        }

        http->connect_timeout_second = connect_timeout_second;
        http->recv_timeout_second = recv_timeout_second;
        return true;
    }

    bool
    http_set_callback (
                        http_t * http,
                        http_data_callback_t content_callback,
                        void * content_callback_param,
                        http_data_callback_t header_callback,
                        void * header_callback_param
                        )
    {
        if ( unlikely (! http) )
        {
            return false;
        }

        http->content_callback = content_callback;
        http->content_callback_param = content_callback_param;
        http->header_callback = header_callback;
        http->header_callback_param = header_callback_param;
        return true;
    }

    bool
    http_set_url (
                   http_t * http,
                   const char * url,
                   const char * refer
                   )
    {
        CURLcode r;

        if ( unlikely (NULL == http || NULL == url || '\0' == * url) )
        {
            return false;
        }

        curl_easy_setopt (http->curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt (http->curl, CURLOPT_AUTOREFERER, 1);
        curl_easy_setopt (http->curl, CURLOPT_MAXREDIRS, 11);
        curl_easy_setopt (http->curl, CURLOPT_MAXCONNECTS, 11);
        curl_easy_setopt (http->curl, CURLOPT_CLOSEPOLICY, CURLCLOSEPOLICY_LEAST_RECENTLY_USED);

        curl_easy_setopt (http->curl, CURLOPT_FILETIME, 1);
        curl_easy_setopt (http->curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt (http->curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt (http->curl, CURLOPT_SSL_VERIFYPEER, 0);
        //curl_easy_setopt (http->curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 1);
        curl_easy_setopt (http->curl, CURLOPT_DNS_CACHE_TIMEOUT, 39600);
        curl_easy_setopt (http->curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt (http->curl, CURLOPT_ENCODING, "gzip,deflate");

        if ( refer && * refer )
        {
            r = curl_easy_setopt (http->curl, CURLOPT_REFERER, refer);
            if ( unlikely (CURLE_OK != r) )
            {
                return false;
            }
        }

        r = curl_easy_setopt (http->curl, CURLOPT_URL, url);
        if ( unlikely (CURLE_OK != r) )
        {
            return false;
        }

        return true;
    }

    bool
    http_set_postfields (
                          http_t * http,
                          const char * fields,
                          size_t fields_bytes,
                          const char * content_type
                          )
    {
        CURLcode r;
        size_t content_type_len;

        if ( unlikely (NULL == http) )
        {
            return false;
        }
        if ( NULL == fields )
        {
            fields = "";
            fields_bytes = 0;
        }
        else
        {
            if ( ( size_t ) - 1 == fields_bytes )
            {
                fields_bytes = strlen (fields);
            }
        }

        if ( ( ( NULL == content_type || '\0' == * content_type ) && '\0' != http->content_type[ 0 ] )
             || ( content_type && * content_type && 0 != strcmp (http->content_type, content_type) ) )
        {
            content_type_len = content_type ? strlen (content_type) : 0;
            if ( content_type_len > 0 )
            {
                if ( content_type_len >= sizeof ( http->content_type ) )
                {
                    return false;
                }
                memcpy (http->content_type, content_type, content_type_len + 1);
            }
            else
            {
                http->content_type[ 0 ] = '\0';
            }

            if ( unlikely (! http_header_reset (http)) )
            {
                return false;
            }
        }

        r = curl_easy_setopt (http->curl, CURLOPT_POSTFIELDS, fields);
        if ( unlikely (CURLE_OK != r) )
        {
            return false;
        }

        r = curl_easy_setopt (http->curl, CURLOPT_POSTFIELDSIZE, ( long ) fields_bytes);
        if ( unlikely (CURLE_OK != r) )
        {
            return false;
        }

        return true;
    }

    bool
    http_add_multi_post (
                          http_t * http,
                          const char * name,
                          const char * file
                          )
    {
#if defined( __ANDROID__ )
        return false;
#else
        CURLFORMcode r;

        if ( unlikely (NULL == http || NULL == name || '\0' == * name || NULL == file || '\0' == * file) )
        {
            return false;
        }

        r = curl_formadd (
                          & http->formpost,
                          & http->formpost_lastptr,
                          CURLFORM_COPYNAME, name,
                          CURLFORM_FILE, file,
                          CURLFORM_END
                          );
        if ( unlikely (CURL_FORMADD_OK != r) )
        {
            const char * s;
            char buf[ 32 ];
            switch ( r )
            {
                case CURL_FORMADD_MEMORY:
                    s = "CURL_FORMADD_MEMORY";
                    break;
                case CURL_FORMADD_OPTION_TWICE:
                    s = "CURL_FORMADD_OPTION_TWICE";
                    break;
                case CURL_FORMADD_NULL:
                    s = "CURL_FORMADD_NULL";
                    break;
                case CURL_FORMADD_UNKNOWN_OPTION:
                    s = "CURL_FORMADD_UNKNOWN_OPTION";
                    break;
                case CURL_FORMADD_INCOMPLETE:
                    s = "CURL_FORMADD_INCOMPLETE";
                    break;
                case CURL_FORMADD_ILLEGAL_ARRAY:
                    s = "CURL_FORMADD_ILLEGAL_ARRAY";
                    break;
                case CURL_FORMADD_DISABLED:
                    s = "CURL_FORMADD_DISABLED";
                    break;
                default:
                    sprintf (buf, "%d", r);
                    s = buf;
                    break;
            }
            return false;
        }

        return true;
#endif
    }

    bool
    http_set_base_security (
                             http_t * http,
                             const char * user,
                             const char * passwd
                             )
    {
#if defined( __ANDROID__ )
        return false;
#else
        CURLcode r;
        char userpwd[ 64 ];
        size_t user_len;
        size_t passwd_len;

        if ( unlikely (NULL == http) )
        {
            return false;
        }

        if ( NULL == user )
        {
            user = "";
        }
        user_len = strlen (user);
        if ( NULL == passwd )
        {
            passwd = "";
        }
        passwd_len = strlen (passwd);
        if ( 0 == user_len )
        {
            userpwd[ 0 ] = '\0';
        }
        else
        {
            if ( user_len + 1 + passwd_len >= sizeof ( userpwd ) )
            {
                return false;
            }

            if ( strchr (user, ':') )
            {
                return false;
            }

            strcpy (userpwd, user);
            strcat (userpwd, ":");
            strcat (userpwd, passwd);
        }

        if ( userpwd[ 0 ] )
        {
            r = curl_easy_setopt (http->curl, CURLOPT_USERPWD, userpwd);
            if ( unlikely (CURLE_OK != r) )
            {
                return false;
            }
        }

        return true;
#endif
    }

    bool
    http_perform (
                   http_t * http,
                   unsigned int flags,
                   const char * http_method,
                   int * http_code
                   )
    {
#if defined( __ANDROID__ )
        return false;
#else
        CURLcode r;
        bool b = false;
        bool is_keep_alive = HTTP_KEEP_ALIVE & flags;
        bool is_resp_header = HTTP_RESP_HEADER & flags;
        long rcode = 0;

        if ( unlikely (! http) )
        {
            if ( http_code )
            {
                * http_code = 0;
            }
            return false;
        }

        do
        {

            if ( http->connect_timeout_second > 0 )
            {
                r = curl_easy_setopt (http->curl, CURLOPT_CONNECTTIMEOUT, http->connect_timeout_second);
                if ( unlikely (CURLE_OK != r) )
                {
                    if ( http_code )
                    {
                        * http_code = 0;
                    }
                    break;
                }
            }

            if ( http->recv_timeout_second > 0 )
            {
                r = curl_easy_setopt (http->curl, CURLOPT_TIMEOUT, http->recv_timeout_second);
                if ( unlikely (CURLE_OK != r) )
                {
                    if ( http_code )
                    {
                        * http_code = 0;
                    }
                    break;
                }
            }

            r = curl_easy_setopt (http->curl, CURLOPT_WRITEDATA, http->content_callback_param);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            r = curl_easy_setopt (http->curl, CURLOPT_WRITEFUNCTION, http->content_callback);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            r = curl_easy_setopt (http->curl, CURLOPT_HEADERDATA, http->header_callback_param);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            r = curl_easy_setopt (http->curl, CURLOPT_HEADERFUNCTION, http->header_callback);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            if ( http_method && * http_method )
            {
                r = curl_easy_setopt (http->curl, CURLOPT_CUSTOMREQUEST, http_method);
                if ( unlikely (CURLE_OK != r) )
                {
                    if ( http_code )
                    {
                        * http_code = 0;
                    }
                    break;
                }
            }

            if ( is_resp_header )
            {
                r = curl_easy_setopt (http->curl, CURLOPT_HEADER, 1);
            }
            else
            {
                r = curl_easy_setopt (http->curl, CURLOPT_HEADER, 0);
            }
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            if ( http->keep_alive != is_keep_alive )
            {
                http->keep_alive = is_keep_alive;

                if ( unlikely (! http_header_reset (http)) )
                {
                    if ( http_code )
                    {
                        * http_code = 0;
                    }
                    break;
                }
            }

            r = curl_easy_setopt (http->curl, CURLOPT_HTTPHEADER, http->header);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            if ( http->formpost )
            {
                r = curl_easy_setopt (http->curl, CURLOPT_HTTPPOST, http->formpost);
                if ( unlikely (CURLE_OK != r) )
                {
                    if ( http_code )
                    {
                        * http_code = 0;
                    }
                    break;
                }
            }

            r = curl_easy_perform (http->curl);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            r = curl_easy_getinfo (http->curl, CURLINFO_RESPONSE_CODE, & rcode);
            if ( unlikely (CURLE_OK != r) )
            {
                if ( http_code )
                {
                    * http_code = 0;
                }
                break;
            }

            if ( http_code )
            {
                * http_code = ( int ) rcode;
            }

            b = true;
        }
        while ( 0 );

        http_reset_request (http);

        return b;
#endif
    }

    void http_reset_request (
                              http_t * http
                              )
    {
        if ( unlikely (! http) )
        {
            return;
        }

        curl_easy_reset (http->curl);
        http_clear_form (http);
    }

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus

