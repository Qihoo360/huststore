#ifndef _store_doc_config_h_
#define _store_doc_config_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "i_kv.h"
#include "../../base.h"
#include <vector>
#include <string>

class key_hash_t;
class kv_array_t;

class config_t
{
public:
    config_t ( );
    ~config_t ( );

    bool open (
                const char * config_path,
                int server_id,
                const char * base_dir,
                key_hash_t & hash,
                kv_array_t & files
                );
    bool open_memory (
                       const char * base_dir,
                       const char * content,
                       int server_id,
                       size_t content_bytes,
                       key_hash_t & hash,
                       kv_array_t & files
                       );
    void close ( );

    int get_key_len ( ) const
    {
        return m_key_len;
    }
    int get_max_file_count ( );

    int get_ldb_bloom_filter_bits ( ) const
    {
        return m_ldb_bloom_filter_bits;
    }

    bool is_readonly ( ) const
    {
        return m_readonly;
    }

    bool is_disable_compression ( ) const
    {
        return m_disable_compression;
    }

    int get_md5_bloom_filter_mode ( ) const
    {
        return m_md5_bloom_filter;
    }

    int get_record_bytes ( ) const
    {
        return m_record_bytes;
    }

    void set_record_bytes ( int v )
    {
        m_record_bytes = v;
    }

    int get_write_buffer_m ( ) const
    {
        return m_write_buffer_m;
    }

    int get_cache_size_m ( ) const
    {
        return m_cache_size_m;
    }
    const char * get_file_path ( int i );

    const kv_config_t & get_kv_config ( ) const
    {
        return m_kv_config;
    }

    kv_config_t & get_kv_config ( )
    {
        return m_kv_config;
    }

private:

    bool open (
                ini_t & ini,
                int server_id,
                key_hash_t & hash,
                kv_array_t & files
                );

private:

    typedef std::vector< std::string > path_list_t;

    kv_config_t m_kv_config;
    key_hash_t * m_key_hash;

    bool m_readonly;
    bool m_disable_compression;

    int m_md5_bloom_filter;

    int m_key_len;
    int m_ldb_bloom_filter_bits;

    int m_record_bytes;
    int m_cache_size_m;
    int m_write_buffer_m;

    path_list_t m_files;
    std::string m_base_dir;

private:
    // disable
    config_t ( const config_t & );
    const config_t & operator= ( const config_t & );
};

#endif // #ifndef _store_doc_config_h_
