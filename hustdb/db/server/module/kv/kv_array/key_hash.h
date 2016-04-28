#ifndef _store_doc_key_hash_h_
#define _store_doc_key_hash_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"
#include <vector>
#include <sstream>

class config_t;
class kv_array_t;

struct migration_plan_t
{
    int from_server;
    int from_user_file_id;
    int from_inner_file_id;

    int to_inner_file_id;

    migration_plan_t ( )
    : from_server ( -1 )
    , from_user_file_id ( -1 )
    , from_inner_file_id ( -1 )
    , to_inner_file_id ( -1 )
    {
    }
};

class hash_plan_t
{
public:
    hash_plan_t ( );
    hash_plan_t ( const hash_plan_t & );
    const hash_plan_t & operator= ( const hash_plan_t & );
    void close ( );

    bool create ( int user_file_count, int server_count, int copy_count );
    bool open ( int server_id, const char * hash_conf );
    bool open_memory ( int server_id, const char * s, size_t s_len );
    bool open ( int server_id, ini_t & ini );
    bool save ( const char * hash_conf );
    bool save_memory ( std::string & s );

    int get_copy_count ( ) const
    {
        return m_copy_count;
    }

    int get_user_file_count ( ) const
    {
        return m_user_file_count;
    }

    int get_data_file_count ( ) const
    {
        return m_data_file_count;
    }

    // return public_hash value
    int hash_with_cluster (
                            const char * key,
                            int key_len,
                            std::vector< int > & other_servers,
                            int & local_file
                            );

    int hash_with_md5db (
                          const char * key,
                          int key_len,
                          std::vector< int > & other_servers,
                          int & local_file
                          );

    int hash_with_user_file_id (
                                 int user_file_id,
                                 std::vector< int > & other_servers,
                                 int & local_file
                                 );

    bool add_server (
                      const sockaddr_in & addr,
                      std::vector< migration_plan_t > & plan,
                      int & new_server_id
                      );

    bool is_inner_file_valid ( int file_id );
    int inner_file_2_user_file ( int inner_file_id );

    int get_server_count ( )
    {
        return ( int ) m_server_2_user_file.size ( );
    }

private:

    int hash_no_md5db (
                        const char * key,
                        int key_len
                        );

    int hash_md5db (
                     const char * key,
                     int key_len
                     );

    int find_busy_server ( );
    bool find_migration_piece_from_server (
                                            migration_plan_t & item,
                                            const std::vector< migration_plan_t > & plan,
                                            int new_server_id
                                            );

private:
    typedef std::vector< int > data_t;

    typedef std::vector< int > file_ids_t;
    typedef std::vector< file_ids_t > server_2_file_t;

    typedef std::vector< int > server_ids_t;
    typedef std::vector< server_ids_t > file_2_server_t;

    static const int HASH_COUNT;

private:

    // hash => user_file_id
    // hash count = HASH_COUNT
    data_t m_data;
    
    // inner file count. if copy count > 1, this value maybe less than m_user_file_count.
    int m_data_file_count;
    
    // user file count.
    int m_user_file_count;

    // server => user file id => inner file id (-1 if empty)
    server_2_file_t m_server_2_user_file;

    // server => inner file id => user file id (-1 if empty)
    server_2_file_t m_server_2_inner_file;

    // user file => server list
    file_2_server_t m_file_2_server;

    int m_copy_count;
    int m_server_id;
};

class key_hash_t
{
public:
    key_hash_t ( );
    ~key_hash_t ( );

    bool open ( const char * conf_path, int server_id, config_t & config, kv_array_t & files );
    bool create ( const char * conf_path, int max_file_count, int server_count, int copy_count );
    void close ( );

    int hash_with_cluster (
                            const char * key,
                            int key_len,
                            std::vector< int > & other_servers,
                            int & local_file
                            );

    int hash_with_md5db (
                          const char * key,
                          int key_len,
                          std::vector< int > & other_servers,
                          int & local_file
                          );

    int get_data_file_count ( )
    {
        return m_hash.get_data_file_count ( );
    }

    int get_user_file_count ( ) const
    {
        return m_hash.get_user_file_count ( );
    }

    int get_server_count ( )
    {
        return m_hash.get_server_count ( );
    }

    int hash_with_user_file_id (
                                 int user_file_id,
                                 std::vector< int > & other_servers,
                                 int & local_file
                                 );

    bool is_inner_file_valid ( int file_id )
    {
        return m_hash.is_inner_file_valid ( file_id );
    }

    int inner_file_2_user_file ( int inner_file_id )
    {
        return m_hash.inner_file_2_user_file ( inner_file_id );
    }

private:

    config_t * m_config;
    kv_array_t * m_files;
    hash_plan_t m_hash;

private:
    // disable
    key_hash_t ( const key_hash_t & );
    const key_hash_t & operator= ( const key_hash_t & );
};

#endif // #ifndef _store_doc_key_hash_h_
