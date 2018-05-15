#include "key_hash.h"
#include "kv_config.h"
#include <sstream>
#include <map>

const int hash_plan_t::HASH_COUNT = 10000;

hash_plan_t::hash_plan_t ( )
    : m_data ( )
    , m_data_file_count ( 0 )
    , m_user_file_count ( 0 )
    , m_server_2_user_file ( )
    , m_server_2_inner_file ( )
    , m_file_2_server ( )
    , m_copy_count ( 0 )
    , m_server_id ( - 1 )
{
}

hash_plan_t::hash_plan_t ( const hash_plan_t & rhd )
    : m_data ( rhd.m_data )
    , m_data_file_count ( rhd.m_data_file_count )
    , m_user_file_count ( rhd.m_user_file_count )
    , m_server_2_user_file ( rhd.m_server_2_user_file )
    , m_server_2_inner_file ( rhd.m_server_2_inner_file )
    , m_file_2_server ( rhd.m_file_2_server )
    , m_copy_count ( rhd.m_copy_count )
    , m_server_id ( rhd.m_server_id )
{
}

const hash_plan_t & hash_plan_t::operator = ( const hash_plan_t & rhd )
{
    if ( this != & rhd )
    {
        m_data = rhd.m_data;
        m_data_file_count = rhd.m_data_file_count;
        m_user_file_count = rhd.m_user_file_count;
        m_server_2_user_file = rhd.m_server_2_user_file;
        m_server_2_inner_file = rhd.m_server_2_inner_file;
        m_file_2_server = rhd.m_file_2_server;
        m_copy_count = rhd.m_copy_count;
        m_server_id = rhd.m_server_id;
    }
    return * this;
}

void hash_plan_t::close ( )
{
    m_data.resize ( 0 );
    m_data_file_count = 0;
    m_user_file_count = 0;
    m_server_2_user_file.resize ( 0 );
    m_server_2_inner_file.resize ( 0 );
    m_file_2_server.resize ( 0 );
    m_copy_count = 0;
    m_server_id = - 1;
}

int hash_plan_t::hash_no_md5db (
                                 const char *        key,
                                 int                 key_len
                                 )
{
    unsigned int hash;
    unsigned int seed = 131;
    hash = 0;
    for ( int i = 0; i < key_len; ++ i )
    {
        hash = hash * seed + ( unsigned char ) key[ i ];
    }

    return m_data[ hash % HASH_COUNT ];
}

int hash_plan_t::hash_with_cluster (
                                     const char *            key,
                                     int                     key_len,
                                     std::vector< int > &    other_servers,
                                     int &                   local_file
                                     )
{
    other_servers.resize ( 0 );
    local_file = - 1;

    int v = hash_no_md5db ( key, key_len );
    if ( v < 0 || v >= ( int ) m_file_2_server.size () )
    {
        LOG_ERROR ( "[hash][%d,%d]invalid public_hash", 
                    v, ( int ) m_file_2_server.size () );
        return - 1;
    }
    server_ids_t & servers = m_file_2_server[ v ];

    if ( other_servers.capacity () < servers.size () )
    {
        try
        {
            other_servers.reserve ( servers.size () );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return v;
        }
    }

    for ( size_t i = 0; i != servers.size (); ++ i )
    {
        int sid = servers[ i ];

        if ( m_server_id >= 0 && m_server_id == sid )
        {

            if ( m_server_id < ( int ) m_server_2_user_file.size () )
            {
                file_ids_t & ids = m_server_2_user_file[ m_server_id ];
                if ( v < ( int ) ids.size () )
                {
                    local_file = ids[ v ];
                }
                else
                {
                    LOG_ERROR ( "[hash][hash=%d][ids=%d]", 
                                v, ( int ) ids.size () );
                }
            }
            else
            {
                LOG_ERROR ( "[hash][server_id=%d][server_2_file=%d]", 
                            m_server_id, ( int ) m_server_2_user_file.size () );
            }

        }
        else
        {
            other_servers.push_back ( sid );
        }
    }

    return v;
}

int hash_plan_t::hash_md5db (
                              const char *        key,
                              int                 key_len
                              )
{
    //assert ( key && key_len == 16 );
    unsigned int hash = * ( ( const uint32_t * ) ( & key[ 4 ] ) );
    return m_data[ hash % HASH_COUNT ];
}

int hash_plan_t::hash_with_md5db (
                                   const char *            key,
                                   int                     key_len,
                                   std::vector< int > &    other_servers,
                                   int &                   local_file
                                   )
{
    other_servers.resize ( 0 );
    local_file = - 1;

    int v = hash_md5db ( key, key_len );
    if ( v < 0 || v >= ( int ) m_file_2_server.size () )
    {
        LOG_ERROR ( "[hash][%d,%d]invalid public_hash", 
                    v, ( int ) m_file_2_server.size () );
        return - 1;
    }

    server_ids_t & servers = m_file_2_server[ v ];

    LOG_DEBUG ( "[hash][r=%d][servers=%d][server_id=%d]hash_md5db",
                v, ( int ) servers.size (), m_server_id );

    if ( other_servers.capacity () < servers.size () )
    {
        try
        {
            other_servers.reserve ( servers.size () );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return v;
        }
    }

    for ( size_t i = 0; i != servers.size (); ++ i )
    {
        int sid = servers[ i ];

        if ( m_server_id >= 0 && m_server_id == sid )
        {

            if ( m_server_id < ( int ) m_server_2_user_file.size () )
            {
                file_ids_t & ids = m_server_2_user_file[ m_server_id ];
                if ( v < ( int ) ids.size () )
                {
                    local_file = ids[ v ];
                }
                else
                {
                    LOG_ERROR ( "[hash][hash=%d][ids=%d]", 
                                v, ( int ) ids.size () );
                }
            }
            else
            {
                LOG_ERROR ( "[hash][server_id=%d][server_2_file=%d]", 
                            m_server_id, ( int ) m_server_2_inner_file.size () );
            }

        }
        else
        {
            other_servers.push_back ( sid );
        }
    }

    return v;
}

int hash_plan_t::hash_with_user_file_id (
                                          int                     user_file_id,
                                          std::vector< int > &    other_servers,
                                          int &                   local_file
                                          )
{
    other_servers.resize ( 0 );
    local_file = - 1;

    int v = user_file_id;
    if ( v < 0 || v >= ( int ) m_file_2_server.size () )
    {
        LOG_ERROR ( "[hash][%d,%d]invalid user_file_id", 
                    v, ( int ) m_file_2_server.size () );
        return - 1;
    }
    server_ids_t & servers = m_file_2_server[ v ];

    if ( other_servers.capacity () < servers.size () )
    {
        try
        {
            other_servers.reserve ( servers.size () );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return - 1;
        }
    }

    for ( size_t i = 0; i != servers.size (); ++ i )
    {
        int sid = servers[ i ];

        if ( m_server_id >= 0 && m_server_id == sid )
        {

            if ( m_server_id < ( int ) m_server_2_user_file.size () )
            {
                file_ids_t & ids = m_server_2_user_file[ m_server_id ];
                if ( v < ( int ) ids.size () )
                {
                    local_file = ids[ v ];
                }
                else
                {
                    LOG_ERROR ( "[hash][hash=%d][ids=%d]", 
                                v, ( int ) ids.size () );
                }
            }
            else
            {
                LOG_ERROR ( "[hash][server_id=%d][server_2_file=%d]", 
                            m_server_id, ( int ) m_server_2_user_file.size () );
            }

        }
        else
        {
            other_servers.push_back ( sid );
        }
    }

    return 0;
}

bool hash_plan_t::create ( int user_file_count, int server_count, int copy_count )
{
    close ();

    if ( unlikely ( user_file_count <= 0
                   || server_count <= 0
                   || copy_count <= 0
                   || copy_count > 3
                   || copy_count > server_count ) )
    {
        LOG_ERROR ( "[hash]invalid params" );
        return false;
    }

    m_copy_count    = copy_count;
    m_server_2_inner_file.resize ( 0 );
    try
    {
        m_server_2_inner_file.resize ( server_count );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash]bad_alloc" );
        return false;
    }

    size_t server_token = 0;

    // inner file id -> user file id
    for ( int i = 0; i < user_file_count; ++ i )
    {
        for ( int j = 0; j < copy_count; ++ j )
        {
            file_ids_t & ids = m_server_2_inner_file[ server_token % m_server_2_inner_file.size () ];
            ++ server_token;
            try
            {
                ids.push_back ( i );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[hash]bad_alloc" );
                return false;
            }
        }
    }

    // find max size
    int real_data_count = 0;
    for ( int i = 0; i < ( int ) m_server_2_inner_file.size (); ++ i )
    {
        size_t sz = m_server_2_inner_file[ i ].size ();
        if ( real_data_count < ( int ) sz )
        {
            real_data_count = sz;
        }
    }

    m_data_file_count = real_data_count;
    m_user_file_count = user_file_count;

    if ( m_data_file_count <= 0 )
    {
        LOG_ERROR ( "[hash][data_file_count=%d]", 
                    m_data_file_count );
        return false;
    }

    // calc data
    try
    {
        m_data.resize ( HASH_COUNT );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash]bad_alloc" );
        return false;
    }

    int file_hash_count = HASH_COUNT / m_user_file_count;
    int index = 0;
    int i;
    int j;
    for ( i = 0; i < m_user_file_count; ++ i )
    {
        for ( j = 0; j < file_hash_count; ++ j )
        {
            m_data[ index ++ ] = i;
        }
    }
    -- i;
    while ( index < HASH_COUNT )
    {
        m_data[ index ++ ] = i;
    }


    // calc server_2_user_file
    m_server_2_user_file.resize ( 0 );
    try
    {
        m_server_2_user_file.resize ( m_server_2_inner_file.size () );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash]bad_alloc" );
        return false;
    }
    for ( int i = 0; i < ( int ) m_server_2_inner_file.size (); ++ i )
    {

        const file_ids_t & src  = m_server_2_inner_file[ i ];
        file_ids_t & dst        = m_server_2_user_file[ i ];

        // calc max fid
        int max_user_fid = 0;
        for ( int j = 0; j < ( int ) src.size (); ++ j )
        {
            int n = src[ j ];
            if ( n >= 0 && max_user_fid < n )
            {
                max_user_fid = n;
            }
        }
        ++ max_user_fid;

        dst.resize ( 0 );
        try
        {
            dst.resize ( max_user_fid, - 1 );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return false;
        }

        for ( int j = 0; j < ( int ) src.size (); ++ j )
        {
            int n = src[ j ];
            if ( n >= 0 )
            {
                dst[ n ] = j;
            }
        }
    }

    return true;
}

bool hash_plan_t::open_memory ( int server_id, const char * s, size_t s_len )
{
    ini_t * ini;
    bool    b;

    close ();

    ini = G_APPINI->ini_create_memory ( s, s_len );
    if ( NULL == ini )
    {
        LOG_ERROR ( "[hash]error" );
        return false;
    }

    b = open ( server_id, * ini );
    G_APPINI->ini_destroy ( ini );

    if ( ! b )
    {
        LOG_ERROR ( "[hash][content=%s]open failed", 
                    s );
    }

    return b;
}

bool hash_plan_t::open ( int server_id, const char * hash_conf )
{
    ini_t * ini;
    bool    b;

    close ();

    ini = G_APPINI->ini_create ( hash_conf );
    if ( NULL == ini )
    {
        LOG_ERROR ( "[hash]error" );
        return false;
    }

    b = open ( server_id, * ini );
    G_APPINI->ini_destroy ( ini );

    if ( ! b )
    {
        LOG_ERROR ( "[hash][file=%s]open failed", 
                    hash_conf );
    }

    return b;
}

bool hash_plan_t::open ( int server_id, ini_t & ini )
{
    close ();

    if ( server_id < 0 )
    {
        LOG_ERROR ( "[hash]invalid server_id" );
        return false;
    }
    m_server_id = server_id;

    m_data_file_count = G_APPINI->ini_get_int ( & ini, "hash", "data_file_count", 0 );
    if ( m_data_file_count <= 0 )
    {
        LOG_ERROR ( "[hash]data_file_count invalid" );
        return false;
    }

    m_user_file_count = G_APPINI->ini_get_int ( & ini, "hash", "user_file_count", 0 );
    if ( m_user_file_count <= 0 )
    {
        LOG_ERROR ( "[hash]user_file_count invalid" );
        return false;
    }

    m_data.resize ( 0 );
    try
    {
        m_data.resize ( HASH_COUNT );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash]data.resize exception" );
        return false;
    }

    int last_file_id = - 1;
    int index = 0;
    char key[ 32 ];
    while ( index < HASH_COUNT )
    {
        sprintf ( key, "%d", index );
        int file_id = G_APPINI->ini_get_int ( & ini, "hash", key, - 1 );
        if ( file_id < 0 )
        {
            if ( last_file_id < 0 )
            {
                LOG_ERROR ( "[hash]config file error" );
                return false;
            }
        }
        else
        {
            last_file_id = file_id;
        }
        m_data[ index ++ ] = last_file_id;
    }

    //
    m_copy_count = G_APPINI->ini_get_int ( & ini, "server", "copy_count", 0 );
    if ( m_copy_count <= 0 )
    {
        LOG_ERROR ( "[hash][copy_count=%d]invalid copy_count", 
                    m_copy_count );
        return false;
    }

    int server_count = G_APPINI->ini_get_int ( & ini, "server", "server_count", 0 );
    if ( server_count <= 0 )
    {
        LOG_ERROR ( "[hash][server_count=%d]invalid server_count", 
                    server_count );
        return false;
    }
    if ( server_count < m_copy_count )
    {
        LOG_ERROR ( "[hash][server_count=%d < copy_count=%d]", 
                    server_count, m_copy_count );
        return false;
    }
    if ( m_copy_count > 3 )
    {
        LOG_ERROR ( "[hash][copy_count=%d > 3]", 
                    m_copy_count );
        return false;
    }
    if ( m_server_id < 0 || m_server_id >= server_count )
    {
        LOG_ERROR ( "[hash][server_count=%d[server_id=%d]]invalid server_count or server_id", 
                    server_count, m_server_id );
        return false;

    }

    m_server_2_inner_file.resize ( 0 );
    if ( m_server_2_inner_file.capacity () < ( size_t ) server_count )
    {
        try
        {
            m_server_2_inner_file.reserve ( server_count );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return false;
        }
    }

    std::map< int, server_ids_t >   user_file_2_server;

    // load server  =>  inner file
    // inner file is: inner file index = user file id

    // verify step 1: user file id => copy count
    std::map< int, int >            verify;
    std::vector< std::string >      a;
    for ( int i = 0; i < server_count; ++ i )
    {

        sprintf ( key, "server_%d", i );

        // index is inner file id, value is public file id
        const char * s = G_APPINI->ini_get_string ( & ini, "server", key, NULL );
        if ( NULL == s || '\0' == * s )
        {
            LOG_ERROR ( "[hash][key=%s]invalid", 
                        key );
            return false;
        }

        file_ids_t an;
        to_vector ( s, ",", a );
        if ( a.empty () )
        {
            LOG_ERROR ( "[hash][key=%s]invalid", 
                        key );
            return false;
        }
        an.resize ( 0 );
        try
        {
            if ( an.capacity () < ( size_t ) m_data_file_count )
            {
                an.reserve ( m_data_file_count );
            }
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return false;
        }
        for ( int j = 0; j < ( int ) a.size (); ++ j )
        {

            // j is inner file id
            const std::string & item = a[ j ];
            int user_file_id;
            if ( "-1" == item )
            {
                user_file_id = - 1;
            }
            else
            {
                if ( item.empty () || item[ 0 ] <= 0 || ! isdigit ( item[ 0 ] ) )
                {
                    LOG_ERROR ( "[hash][%s,%s]invalid", 
                                key, item.c_str () );
                    return false;
                }
                user_file_id    = atoi ( item.c_str () );
            }
            try
            {
                an.push_back ( user_file_id );
            }
            catch ( ... )
            {
                LOG_ERROR ( "[hash]bad_alloc" );
                return false;
            }

            // member user file => server
            if ( user_file_id >= 0 )
            {
                std::map< int, server_ids_t >::iterator f;
                f = user_file_2_server.find ( user_file_id );
                try
                {
                    if ( f == user_file_2_server.end () )
                    {
                        server_ids_t ids;
                        ids.push_back ( i );
                        user_file_2_server[ user_file_id ] = ids;
                    }
                    else
                    {
                        ( *f ).second.push_back ( i );
                    }
                }
                catch ( ... )
                {
                    LOG_ERROR ( "[hash]bad_alloc" );
                    return false;
                }
            }

            // member user file id => copy count
            if ( user_file_id >= 0 )
            {
                std::map< int, int >::iterator f = verify.find ( user_file_id );
                if ( verify.end () == f )
                {
                    try
                    {
                        verify[ user_file_id ] = 1;
                    }
                    catch ( ... )
                    {
                        LOG_ERROR ( "[hash]bad_alloc" );
                        return false;
                    }
                    LOG_DEBUG ( "[hash][verify][s=%s][item=%s][user_file_id=%d]", 
                                s, item.c_str (), user_file_id );
                }
                else
                {
                    ++ ( *f ).second;
                    LOG_DEBUG ( "[hash][verify][s=%s][item=%s][%d=%d]", 
                                s, item.c_str (), user_file_id, ( *f ).second );
                }
            }
        }

        while ( an.size () < ( size_t ) m_data_file_count )
        {
            an.push_back ( - 1 );
        }
        m_server_2_inner_file.push_back ( an );
    }

    // load server  =>  user file
    // user file is: user file index = inner file id
    m_server_2_user_file.resize ( 0 );
    try
    {
        m_server_2_user_file.resize ( server_count );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash]bad_alloc" );
        return false;
    }
    for ( int i = 0; i < server_count; ++ i )
    {

        const file_ids_t & src  = m_server_2_inner_file[ i ];
        file_ids_t & dst        = m_server_2_user_file[ i ];

        dst.resize ( 0 );
        try
        {
            dst.resize ( m_user_file_count, - 1 );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return false;
        }

        for ( int j = 0; j < ( int ) src.size (); ++ j )
        {
            // n is user file id, j is inner file id
            int n = src[ j ];
            if ( n >= 0 )
            {
                if ( n >= ( int ) dst.size () )
                {
                    LOG_ERROR ( "[hash][user_file_id=%d]user file too large", 
                                n );
                    return false;
                }
                dst[ n ] = j;
            }
        }
    }

#if ENABLE_DEBUG
    {
        std::stringstream ss;
        ss << S_CRLF "verify: ";
        for ( std::map< int, int >::iterator i = verify.begin (); i != verify.end (); ++ i )
        {
            if ( i != verify.begin () )
            {
                ss << "; ";
            }
            ss << ( *i ).first << "=" << ( *i ).second;
        }
        ss << S_CRLF "server_2_file: ";
        for ( size_t i = 0; i < m_server_2_user_file.size (); ++ i )
        {
            file_ids_t & ids = m_server_2_user_file[ i ];
            ss << "    " << ( int ) i << ":" S_CRLF;
            for ( size_t j = 0; j < ids.size (); ++ j )
            {
                ss << "        " << ( int ) j << "=" << ids[ j ] << S_CRLF;
            }
        }
    }
#endif

    // verify step 2
    if ( m_server_2_user_file.size () != server_count )
    {
        LOG_ERROR ( "[hash][%d,%d,%d,%d]invalid section, size not match",
                   ( int ) verify.size (), m_data_file_count, server_count, ( int ) m_server_2_user_file.size () );
        return false;
    }
    for ( int i = 0; i < m_data_file_count; ++ i )
    {
        std::map< int, int >::iterator f = verify.find ( i );
        if ( f == verify.end () )
        {
            LOG_ERROR ( "[hash][file_id=%d]invalid section, file not found", 
                        i );
            return false;
        }
        if ( m_copy_count != ( *f ).second )
        {
            LOG_ERROR ( "[hash][file_id=%d][copy_count=%d][need=%d]invalid section",
                        i, ( *f ).second, m_copy_count );
            return false;
        }
    }
    // verify step 2 OK

    m_file_2_server.resize ( 0 );
    if ( m_file_2_server.capacity () < ( size_t ) m_user_file_count )
    {
        try
        {
            m_file_2_server.reserve ( m_user_file_count );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return false;
        }
    }
    if ( user_file_2_server.size () != m_user_file_count )
    {
        LOG_ERROR ( "[hash][%d != %d]user_file_2_server", 
                    ( int ) user_file_2_server.size (), m_user_file_count );
        return false;
    }
    for ( int i = 0; i < m_user_file_count; ++ i )
    {

        std::map< int, server_ids_t >::iterator f;
        f = user_file_2_server.find ( i );
        if ( f == user_file_2_server.end () )
        {
            LOG_ERROR ( "[hash][file_id=%d]not found in user_file_2_server", 
                        i );
            return false;
        }
        if ( ( int ) m_file_2_server.size () != i )
        {
            LOG_ERROR ( "[hash][%d != %d]", 
                        ( int ) m_file_2_server.size (), i );
            return false;
        }
        if ( ( *f ).second.size () != m_copy_count )
        {
            LOG_ERROR ( "[hash][%d != %d]", 
                        ( int ) ( *f ).second.size (), m_copy_count );
            return false;
        }
        try
        {
            m_file_2_server.push_back ( ( *f ).second );
        }
        catch ( ... )
        {
            LOG_ERROR ( "[hash]bad_alloc" );
            return false;
        }
    }

    // verify step 3
    for ( int i = 0; i < m_user_file_count; ++ i )
    {
        // 对每个 user_file_id，找到所有的 server_id
        if ( i >= ( int ) m_file_2_server.size () )
        {
            LOG_ERROR ( "[hash][size=%d]invalid file_2_server.size", 
                        ( int ) m_file_2_server.size () );
            return false;
        }
        server_ids_t & ids = m_file_2_server[ i ];
        if ( ids.size () != m_copy_count )
        {
            LOG_ERROR ( "[hash][size=%d]invalid server ids", 
                        ( int ) ids.size () );
            return false;
        }

        for ( size_t j = 0; j < ids.size (); ++ j )
        {

            // 对每个 server_id
            int server_id = ids[ j ];

            if (   server_id < 0
                 || server_id >= ( int ) m_server_2_user_file.size ()
                 || server_id >= ( int ) m_server_2_inner_file.size ()
                 )
            {
                LOG_ERROR ( "[hash][server_id=%d]invalid server id", 
                            server_id );
                return false;
            }

            // 从对应的 user_file 中取得 inner_file
            file_ids_t & pfids = m_server_2_user_file[ server_id ];
            if ( i >= ( int ) pfids.size () )
            {
                LOG_ERROR ( "[hash][server_id=%d]invalid server_2_user_file for server id", 
                            server_id );
                return false;
            }

            int inner_file = pfids[ i ];
            if ( inner_file < 0 )
            {
                LOG_ERROR ( "[hash][server_id=%d][public_file=%d]invalid server_2_user_file", 
                            server_id, i );
                return false;
            }

            // 再从对应的 inner_file 中取得 user_file
            file_ids_t & ifids = m_server_2_inner_file[ server_id ];
            if ( inner_file >= ( int ) ifids.size () )
            {
                LOG_ERROR ( "[hash][server_id=%d]invalid server_2_inner_file", 
                            server_id );
                return false;
            }

            if ( ifids[ inner_file ] != i )
            {
                LOG_ERROR ( "[hash][server_id=%d][inner_file=%d]invalid server_2_inner_file", 
                            server_id, inner_file );
                return false;
            }
        }
    }
    // verify step 3 OK

    return true;
}

bool hash_plan_t::save_memory ( std::string & s )
{
    s.resize ( 0 );

    if ( m_data_file_count <= 0 || 
         m_user_file_count <= 0 || 
         m_data.empty () || 
         m_server_2_user_file.empty () || 
         m_copy_count <= 0 || 
         m_copy_count > 3 )
    {
        LOG_ERROR ( "[hash][%d,%d,%d,%d,%d]error",
                   m_data_file_count, m_user_file_count, ( int ) m_data.size (), ( int ) m_server_2_user_file.size (), m_copy_count );
        return false;
    }

    try
    {
        std::stringstream ss;
        ss << "[hash]" S_CRLF;
        ss << "data_file_count = " << m_data_file_count << S_CRLF;
        ss << "user_file_count = " << m_user_file_count << S_CRLF;

        int last_file_id = - 1;
        int file_id;
        for ( int i = 0; i < ( int ) m_data.size (); ++ i )
        {
            file_id = m_data[ i ];
            if ( file_id != last_file_id )
            {
                last_file_id = file_id;
                ss << i << " = " << file_id << S_CRLF;
            }
        }

        ss << "[server]" S_CRLF;
        ss << "server_count = " << ( int ) m_server_2_user_file.size () << S_CRLF;
        ss << "copy_count   = " << m_copy_count << S_CRLF;
        for ( size_t i = 0; i < m_server_2_inner_file.size (); ++ i )
        {
            ss << "server_" << ( int ) i << " = ";
            file_ids_t & ids = m_server_2_inner_file[ i ];
            for ( size_t j = 0; j < ids.size (); ++ j )
            {
                char buf[ 32 ];
                if ( 0 != j )
                {
                    ss << ", ";
                }
                // n is public file, j is inner file
                int n = ids[ j ];
                if ( n >= 0 )
                {
                    // write public file
                    sprintf ( buf, "%3d", n );
                    ss << buf;
                }
                else
                {
                    ss << "-1";
                }
            }
            ss << S_CRLF;
        }

        s = ss.str ();

    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash]error" );
        return false;
    }

    if ( s.empty () )
    {
        LOG_ERROR ( "[hash]error" );
        return false;
    }

    return true;
}

bool hash_plan_t::save ( const char * hash_conf )
{
    std::string s;
    if ( ! save_memory ( s ) )
    {
        LOG_ERROR ( "[hash]save_memory failed" );
        return false;
    }

    FILE * fp = fopen ( hash_conf, "wb" );
    if ( NULL == fp )
    {
        LOG_ERROR ( "[hash]error" );
        return false;
    }

    if ( s.size () != fwrite ( s.c_str (), 1, s.size (), fp ) )
    {
        LOG_ERROR ( "[hash]error" );
        fclose ( fp );
        return false;
    }

    fclose ( fp );
    return true;
}

int hash_plan_t::find_busy_server ( )
{
    int server_id   = - 1;
    int piece_count = 0;

    int server_count = ( int ) m_server_2_user_file.size ();
    for ( int i = 0; i < server_count; ++ i )
    {

        file_ids_t & ids = m_server_2_user_file[ i ];

        int    count = 0;
        int    total = ( int ) ids.size ();
        for ( int j = 0; j < total; ++ j )
        {
            if ( ids[ j ] >= 0 )
            {
                ++ count;
            }
        }
        if ( count > piece_count )
        {
            server_id = i;
            piece_count = count;
        }
    }

    return server_id;
}

bool hash_plan_t::find_migration_piece_from_server (
                                                     migration_plan_t &                          item,
                                                     const std::vector< migration_plan_t > &     plan,
                                                     int                                         new_server_id
                                                     )
{
    assert ( item.from_server < ( int ) m_server_2_user_file.size () );

    file_ids_t & ids = m_server_2_user_file[ item.from_server ];

    int    count = 0;
    int    total = ( int ) ids.size ();
    for ( int j = 0; j < total; ++ j )
    {
        if ( ids[ j ] >= 0 )
        {
            ++ count;
        }
    }
    if ( count <= 1 )
    {
        // no piece to migration
        return false;
    }

    // 
    for ( int j = 0; j < total; ++ j )
    {
        int user_file = total - 1 - j;

        if ( ids[ user_file ] < 0 )
        {
            // empty slot
            continue;
        }

        // avoid multiple copy of same piece to new server
        bool found = false;
        for ( size_t i = 0; i < plan.size (); ++ i )
        {
            const migration_plan_t & item = plan[ i ];
            if ( item.from_user_file_id == user_file )
            {
                found = true;
            }
        }
        if ( found )
        {
            continue;
        }

        item.from_user_file_id  = user_file;
        item.from_inner_file_id = ids[ user_file ];

        if ( m_server_2_inner_file[ item.from_server ][ item.from_inner_file_id ] != user_file )
        {
            LOG_ERROR ( "[hash][add_server]verify failed" );
            return false;
        }

        // update file_2_server
        server_ids_t & svrs = m_file_2_server[ user_file ];
        for ( size_t i = 0; i < svrs.size (); ++ i )
        {
            if ( item.from_server == svrs[ i ] )
            {
                svrs[ i ] = new_server_id;
            }
        }

        // delete from server_2_user_file
        ids[ user_file ] = - 1;
        // delete from server_2_inner_file
        m_server_2_inner_file[ item.from_server ][ item.from_inner_file_id ] = - 1;
        return true;
    }

    return false;
}

bool hash_plan_t::add_server (
                               const sockaddr_in &                 addr,
                               std::vector< migration_plan_t > &   plan,
                               int &                               new_server_id
                               )
{
    plan.resize ( 0 );
    new_server_id = - 1;

    if ( m_server_2_inner_file.size () != m_server_2_user_file.size () )
    {
        LOG_ERROR ( "[hash][add_server]error" );
        return false;
    }

    new_server_id = ( int ) m_server_2_user_file.size ();

    int migration_count = m_user_file_count * m_copy_count / ( int ) ( m_server_2_inner_file.size () + 1 );
    LOG_DEBUG ( "[hash][add_server][migration_count=%d]", 
                migration_count );

    try
    {
        plan.reserve ( migration_count );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash][add_server][migration_count=%d]bad_alloc",
                    migration_count );
        return false;
    }

    int new_inner_file_id = - 1;
    for ( int i = 0; i < migration_count; ++ i )
    {
        int busy_server_id = find_busy_server ();
        if ( busy_server_id < 0 )
        {
            break;
        }

        migration_plan_t item;
        item.from_server        = busy_server_id;
        if ( ! find_migration_piece_from_server ( item, plan, new_server_id ) )
        {
            continue;
        }

        item.to_inner_file_id = ++ new_inner_file_id;
        plan.push_back ( item );
    }
    if ( plan.empty () )
    {
        LOG_ERROR ( "[hash][add_server][migration_count=%d]find migration piece failed", 
                    migration_count );
        return false;
    }

    // add to m_server_2_user_file
    file_ids_t public_2_inner;
    try
    {
        public_2_inner.resize ( m_user_file_count, - 1 );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash][add_server][migration_count=%d]bad_alloc", 
                    migration_count );
        return false;
    }
    for ( int i = 0; i < ( int ) plan.size (); ++ i )
    {
        const migration_plan_t & item = plan[ i ];
        public_2_inner[ item.from_user_file_id ] = item.to_inner_file_id;
    }
    try
    {
        m_server_2_user_file.push_back ( public_2_inner );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash][add_server][migration_count=%d]bad_alloc", 
                    migration_count );
        return false;
    }

    // add to m_server_2_inner_file
    int inner_file_count = new_inner_file_id + 1;
    file_ids_t inner_2_public;
    try
    {
        inner_2_public.resize ( inner_file_count, - 1 );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash][add_server][migration_count=%d]bad_alloc", 
                    migration_count );
        return false;
    }
    for ( int i = 0; i < ( int ) plan.size (); ++ i )
    {
        const migration_plan_t & item = plan[ i ];
        inner_2_public[ item.to_inner_file_id ] = item.from_user_file_id;
    }
    try
    {
        m_server_2_inner_file.push_back ( inner_2_public );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[hash][add_server][migration_count=%d]bad_alloc", 
                    migration_count );
        return false;
    }

    return true;
}

bool hash_plan_t::is_inner_file_valid ( int file_id )
{
    if ( m_server_id < 0 || m_server_id >= ( int ) m_server_2_inner_file.size () )
    {
        LOG_ERROR ( "[hash]invalid server_id" );
        return false;
    }

    file_ids_t & ids = m_server_2_inner_file[ m_server_id ];
    if ( file_id < 0 || file_id >= ( int ) ids.size () )
    {
        LOG_ERROR ( "[hash]invalid file_id" );
        return false;
    }

    return ids[ file_id ] >= 0;
}

int hash_plan_t::inner_file_2_user_file ( int inner_file_id )
{
    if ( m_server_id < 0 || m_server_id >= ( int ) m_server_2_inner_file.size () )
    {
        LOG_ERROR ( "[hash]invalid server_id" );
        return - 1;
    }

    file_ids_t & ids = m_server_2_inner_file[ m_server_id ];
    if ( inner_file_id < 0 || inner_file_id >= ( int ) ids.size () )
    {
        LOG_ERROR ( "[hash]invalid file_id" );
        return - 1;
    }

    return ids[ inner_file_id ];
}

key_hash_t::key_hash_t ( )
: m_config ( NULL )
, m_files ( NULL )
, m_hash ( )
{
}

key_hash_t::~ key_hash_t ( )
{
    close ();
}

bool key_hash_t::create ( const char * conf_path, int max_file_count, int server_count, int copy_count )
{
    hash_plan_t hp;
    if ( ! hp.create ( max_file_count, server_count, copy_count ) )
    {
        LOG_ERROR ( "[hash]create failed" );
        return false;
    }
    if ( ! hp.save ( conf_path ) )
    {
        LOG_ERROR ( "[hash][file=%s]generate failed", conf_path );
        return false;
    }

    LOG_INFO ( "[hash][file=%s]generated", conf_path );
    return true;
}

bool key_hash_t::open ( const char * conf_path, int server_id, config_t & config, kv_array_t & files )
{
    hash_plan_t     hp;

    if ( ! G_APPTOOL->is_file ( conf_path ) )
    {
        LOG_ERROR ( "[hash][file=%s]not exist", conf_path );
        return false;
    }

    if ( ! hp.open ( server_id, conf_path ) )
    {
        LOG_ERROR ( "[hash][file=%s]open failed", conf_path );
        return false;
    }

    m_hash      = hp;
    m_config    = & config;
    m_files     = & files;

    return true;
}

void key_hash_t::close ( )
{
}

int key_hash_t::hash_with_cluster (
                                    const char *            key,
                                    int                     key_len,
                                    std::vector< int > &    other_servers,
                                    int &                   local_file
                                    )
{
    int max_key_len = m_config->get_key_len ();
    if ( unlikely ( NULL == key || key_len <= 0 || ( max_key_len > 0 && key_len > max_key_len ) ) )
    {
        LOG_ERROR ( "[hash]invalid key or key_len" );
        local_file = - 1;
        other_servers.resize ( 0 );
        return - 1;
    }

    return m_hash.hash_with_cluster ( key, key_len, other_servers, local_file );
}

int key_hash_t::hash_with_md5db (
                                  const char *            key,
                                  int                     key_len,
                                  std::vector< int > &    other_servers,
                                  int &                   local_file
                                  )
{
    int max_key_len = m_config->get_key_len ();
    if ( unlikely ( NULL == key || key_len <= 0 || ( max_key_len > 0 && key_len > max_key_len ) ) )
    {
        LOG_ERROR ( "[hash]invalid key or key_len" );
        local_file = - 1;
        other_servers.resize ( 0 );
        return - 1;
    }

    return m_hash.hash_with_md5db ( key, key_len, other_servers, local_file );
}

int key_hash_t::hash_with_user_file_id (
                                         int                     user_file_id,
                                         std::vector< int > &    other_servers,
                                         int &                   local_file
                                         )
{
    return m_hash.hash_with_user_file_id ( user_file_id, other_servers, local_file );
}
