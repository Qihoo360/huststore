#ifndef _md5db_bucket_h_
#define _md5db_bucket_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"

namespace md5db
{

    enum bucket_type_t
    {
        BUCKET_NO_DATA = 0,
        BUCKET_DIRECT_DATA = 1,
        BUCKET_CONFLICT_DATA = 2
    };

#pragma pack( push, 1 )

#define BLOCK_ID_BYTES  5

    class block_id_t
    {
    public:

        block_id_t ( )
        {
            reset ( );
        }

        void reset ( )
        {
            m_bucket_id = 0;
            m_data_id = 0;
        }

        void set ( byte_t bucket_id, uint32_t data_id )
        {
            m_bucket_id = bucket_id;
            m_data_id = htonl ( data_id );
        }

        byte_t bucket_id ( ) const
        {
            return m_bucket_id;
        }

        uint32_t data_id ( ) const
        {
            return ntohl ( m_data_id );
        }

    private:

        byte_t m_bucket_id;
        uint32_t m_data_id;
    };

    inline bool operator== ( const block_id_t & lhd, const block_id_t & rhd )
    {
        return lhd.bucket_id ( ) == rhd.bucket_id ( ) && lhd.data_id ( ) == rhd.data_id ( );
    }

    inline bool operator!= ( const block_id_t & lhd, const block_id_t & rhd )
    {
        return lhd.bucket_id ( ) != rhd.bucket_id ( ) || lhd.data_id ( ) != rhd.data_id ( );
    }

#define BUCKET_DATA_ITEM_BYTES  9

    class bucket_data_item_t
    {
    public:
#define BUCKET_DATA_MAX_VERSION 0x3FFFFFFF

        // block id type
        // BUCKET_DIRECT_DATA, BUCKET_CONFLICT_DATA, BUCKET_NO_DATA
        uint32_t type : 2;

        // user version, if type is BUCKET_DIRECT_DATA
        // conflict count, if type is BUCKET_CONFLICT_DATA
        uint32_t version : 30;

        // block id
        block_id_t block_id;

        // conflict count helper function

        uint32_t get_conflict_count ( ) const
        {
            return version;
        }

        void set_conflict_count ( uint32_t v )
        {
            if ( v <= BUCKET_DATA_MAX_VERSION )
            {
                version = v;
            }
        }

        bool add_conflict_count ( )
        {
            if ( version == BUCKET_DATA_MAX_VERSION )
            {
                return false;
            }
            ++version;
            return true;
        }

        uint32_t dec_conflict_count ( )
        {
            if ( version > 0 )
            {
                --version;
            }
            return version;
        }

    };

#pragma pack( pop )

    class fullkey_t;

    class bucket_t
    {
    public:
        bucket_t ( );
        ~bucket_t ( );

        bool create ( const char * path );
        bool open_exist ( const char * path, bool read_write );
        void close ( );

        int find (
                   const void * inner_key,
                   size_t inner_key_len,
                   bucket_data_item_t * & result
                   );

        static void key_to_bytes3 (
                                    const char * inner_key,
                                    size_t inner_key_len,
                                    unsigned char rsp[ 3 ]
                                    );

        void set_fullkey ( fullkey_t * p )
        {
            m_fullkey = p;
        }

        fullkey_t * get_fullkey ( )
        {
            return m_fullkey;
        }

        lockable_t & get_lock ( )
        {
            return m_lock;
        }

    private:

        size_t bucket_bytes ( ) const;
        size_t bucket_item_count ( ) const;
        size_t bucket_item_bytes ( ) const;

        size_t key_to_item (
                             const void * inner_key,
                             size_t inner_key_len
                             );

    private:

        fmap_t m_data;
        fullkey_t * m_fullkey;
        lockable_t m_lock;

    private:
        // disable
        bucket_t ( const bucket_t & );
        const bucket_t & operator= ( const bucket_t & );
    };

} // namespace md5db

#endif
