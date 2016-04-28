#ifndef _kv_leveldb_h_
#define _kv_leveldb_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "i_kv.h"
#include <exception>
#include "../../perf_target.h"

class kv_leveldb_t;

class kv_iterator_t : public i_iterator_t
{
public:

    virtual void kill_me ( );

    virtual bool valid ( ) const;

    virtual void seek_first ( );

    virtual void seek_last ( );

    virtual void seek (
                        const void * key,
                        unsigned int key_len
                        );

    virtual void next ( );

    virtual void prev ( );

    virtual const char * key (
                               size_t * len
                               ) const;

    virtual const char * value (
                                 size_t * len
                                 ) const;

    virtual int status ( ) const;

public:

    kv_iterator_t ( );
    ~kv_iterator_t ( );

    void destroy ( );

    bool create (
                  kv_leveldb_t & db
                  );

    static void * operator new(
                                size_t size
                                )
    {
        void * p = malloc ( size );
        if ( NULL == p )
        {
            throw std::bad_alloc ( );
        }
        return p;
    }

    static void operator delete(
                                 void * p
                                 )
    {
        free ( p );
    }

private:

    void trans_errno (
                       const char * func_name
                       );

private:

    struct inner_t;

    inner_t * m_inner;
};

class kv_leveldb_t : public i_kv_t
{
public:

    virtual void kill_me ( );

    virtual bool open (
                        const char * path,
                        const kv_config_t & config,
                        int file_id
                        );

    virtual const char * get_path ( ) const;

    virtual int get_id ( ) const
    {
        return m_file_id;
    }

    virtual void info (
                        std::stringstream & ss
                        );

    virtual int flush ( );

    virtual int del (
                      const void * key,
                      unsigned int key_len
                      );

    virtual int put (
                      const void * key,
                      unsigned int key_len,
                      const void * data,
                      unsigned int data_len
                      );

    virtual int get (
                      const void * key,
                      unsigned int key_len,
                      std::string & value
                      );

    virtual i_iterator_t * iterator ( );

    void * get_internal_db ( );

public:

    kv_leveldb_t ( );
    ~kv_leveldb_t ( );

    void destroy ( );

    bool bloomfilter_is_not_found (
                                    const void * key,
                                    unsigned int key_len
                                    );

    void bloomfilter_add (
                           const void * key,
                           unsigned int key_len
                           );

    static void * operator new(
                                size_t size
                                )
    {
        void * p = malloc ( size );
        if ( NULL == p )
        {
            throw std::bad_alloc ( );
        }
        return p;
    }

    static void operator delete(
                                 void * p
                                 )
    {
        free ( p );
    }

private:

    bool check_key (
                     const void * key,
                     unsigned int key_len
                     );

    void calc_my_bloom_filter_path (
                                     const char * parent_dir,
                                     char * bloom_filter_path
                                     );

    bool test ( );

private:

    struct inner;

    int m_file_id;
    std::string m_path;
    kv_config_t m_config;
    inner * m_inner;

    perf_target_t m_perf_bloom_add;
    perf_target_t m_perf_bloom_hit;
    perf_target_t m_perf_bloom_not_hit;
    perf_target_t m_perf_not_found;
    perf_target_t m_perf_found;
    perf_target_t m_perf_put_ok;
    perf_target_t m_perf_put_fail;

private:
    // disable
    kv_leveldb_t ( const kv_leveldb_t & );
    const kv_leveldb_t & operator= ( const kv_leveldb_t & );
};

#endif
