#ifndef _i_kv_h_
#define _i_kv_h_

#include <sstream>
#include <map>
#include <string>

struct kv_config_t
{
public:

    kv_config_t ( )
    : cache_size_m ( -1 )
    , write_buffer_m ( -1 )
    , ldb_bloom_filter_bits ( 0 )
    , my_bloom_filter_type ( 0 )
    , is_readonly ( false )
    , disable_compression ( true )
    {
    }

    int cache_size_m;
    int write_buffer_m;
    int ldb_bloom_filter_bits;
    int my_bloom_filter_type;

    bool is_readonly;
    bool disable_compression;
};

class i_iterator_t
{
public:
    virtual void kill_me ( ) = 0;

    virtual bool valid ( ) const = 0;

    virtual void seek_first ( ) = 0;

    virtual void seek_last ( ) = 0;

    virtual void seek (
                        const void * key,
                        unsigned int key_len
                        ) = 0;

    virtual void next ( ) = 0;

    virtual void prev ( ) = 0;

    virtual const char * key (
                               size_t * len = NULL
                               ) const = 0;

    virtual const char * value (
                                 size_t * len = NULL
                                 ) const = 0;

    virtual int status ( ) const = 0;

private:
    // disable
    static void operator delete( void * p );
};

class i_kv_t
{
public:

    virtual void kill_me ( ) = 0;

    virtual bool open (
                        const char * path,
                        const kv_config_t & config,
                        int file_id
                        ) = 0;

    virtual const char * get_path ( ) const = 0;

    virtual int get_id ( ) const = 0;

    virtual void info (
                        std::stringstream & ss
                        ) = 0;

    virtual int flush ( ) = 0;

    virtual int del (
                      const void * key,
                      unsigned int key_len
                      ) = 0;

    virtual int put (
                      const void * key,
                      unsigned int key_len,
                      const void * data,
                      unsigned int data_len
                      ) = 0;

    virtual int get (
                      const void * key,
                      unsigned int key_len,
                      std::string & rsp
                      ) = 0;

    virtual i_iterator_t * iterator ( ) = 0;

private:
    // disable
    static void operator delete( void * p );
};

i_kv_t * create_kv ( );

#endif
