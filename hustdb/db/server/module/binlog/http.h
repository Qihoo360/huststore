#ifndef _base_http_h_
#define _base_http_h_
#ifdef __cplusplus
#include <string>
#include <cstdlib>
#endif
#ifdef __cplusplus
extern "C"
{
#endif

#define unlikely

    struct http_t;
    typedef struct http_t http_t;

    typedef enum
    {
        // use Connection: KeepAlive request header
        HTTP_KEEP_ALIVE = 0x00000001,
        // include header in response HTTP packet.
        HTTP_RESP_HEADER = 0x00000002

    } http_flags_t;

    typedef size_t ( * http_data_callback_t ) (
                                                const void * data,
                                                size_t always_1,
                                                size_t data_bytes,
                                                void * param
                                                );

    bool http_global_init ( );
    void http_global_term ( );

    http_t *
    http_create ( );

    // auto member
    bool
    http_set_timeout (
                       http_t * http,
                       int connect_timeout_second,
                       int recv_timeout_second
                       );

    // auto member
    bool
    http_set_callback (
                        http_t * http,
                        http_data_callback_t content_callback,
                        void * content_callback_param,
                        http_data_callback_t header_callback,
                        void * header_callback_param
                        );

    // this info will be lost after http_perform called
    bool
    http_set_base_security (
                             http_t * http,
                             const char * user,
                             const char * passwd
                             );

    // this info will be lost after http_perform called
    bool
    http_set_url (
                   http_t * http,
                   const char * url,
                   const char * refer
                   );

    // this info will be lost after http_perform called
    bool
    http_set_postfields (
                          http_t * http,
                          const char * fields,
                          size_t fields_bytes,
                          const char * content_type
                          );

    // this info will be lost after http_perform called
    bool
    http_add_multi_post (
                          http_t * http,
                          const char * name,
                          const char * file
                          );

    // lost belows data;
    //    http_set_base_security
    //    http_set_url
    //    http_set_postfields
    //    http_add_multi_post
    void http_reset_request (
                              http_t * http
                              );

    bool
    http_perform (
                   http_t * http,
                   unsigned int flags,
                   const char * http_method,
                   int * http_code
                   );

    void
    http_destroy (
                   http_t * http
                   );

#ifdef __cplusplus
}

#endif // #ifdef __cplusplus

#endif // #ifndef _base_http_h_
