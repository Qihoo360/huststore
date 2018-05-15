#ifndef _md5db_bucket_h_
#define _md5db_bucket_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"

namespace md5db
{

    enum bucket_type_t
    {
        BUCKET_NO_DATA       = 0,
        BUCKET_DIRECT_DATA   = 1,
        BUCKET_CONFLICT_DATA = 2
    };

#pragma pack( push, 1 )

#define BLOCK_ID_BYTES     5
#define BLOCK_FILE_COUNT   256

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
            m_data_id   = 0;
        }

        void set ( byte_t bucket_id, uint32_t data_id )
        {
            m_bucket_id = bucket_id;
            m_data_id   = data_id;
        }

        byte_t bucket_id ( ) const
        {
            return m_bucket_id;
        }

        uint32_t data_id ( ) const
        {
            return m_data_id;
        }

    private:

        byte_t   m_bucket_id;
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

#define BUCKET_DATA_ITEM_BYTES  8
#define BUCKET_DATA_MAX_VERSION 4194304

    class bucket_data_item_t
    {
    public:

        bucket_data_item_t ( )
        {
            reset ( );
        }

        void reset ( )
        {
            m_type  = 0;
            m_ver_h = 0;
            m_ver_l = 0;
            m_block_id.reset ();
        }

        void set_type ( byte_t type )
        {
            m_type = type;
        }

        byte_t type ( ) const
        {
            return m_type;
        }

        void set_version ( uint32_t version )
        {
            m_ver_h = version / 64;
            m_ver_l = version % 65536;
        }

        uint32_t version ( ) const
        {
            return m_ver_h * 16 + m_ver_l;
        }

        uint32_t get_conflict_count ( ) const
        {
            return m_ver_h * 16 + m_ver_l;
        }

        void set_conflict_count ( uint32_t v )
        {
            if ( v <= BUCKET_DATA_MAX_VERSION )
            {
                set_version ( v );
            }
        }

        bool add_conflict_count ( )
        {
            uint32_t version = m_ver_h * 16 + m_ver_l;

            if ( version == BUCKET_DATA_MAX_VERSION )
            {
                return false;
            }

            set_version ( version + 1 );

            return true;
        }

        uint32_t dec_conflict_count ( )
        {
            uint32_t version = m_ver_h * 16 + m_ver_l;

            if ( version > 0 )
            {
                version --;
            }

            set_version ( version );

            return version;
        }

    public:
        
        block_id_t m_block_id;
    
    private:

        byte_t m_type  : 2;
        byte_t m_ver_h : 6;
        uint16_t m_ver_l;
    };

    class content_id_t
    {
    public:

        content_id_t ( )
        {
            reset ( );
        }

        void reset ( )
        {
            m_file_id_h = 0;
            m_data_id   = 0;
            m_file_id_l = 0;
            m_data_len  = 0;
        }

        void set ( byte_t file_id, uint32_t data_id, uint32_t data_len )
        {
            m_file_id_h = file_id / 16;
            m_data_id   = data_id;

            m_file_id_l = file_id % 16;
            m_data_len  = data_len;
        }

        byte_t file_id ( ) const
        {
            return m_file_id_h * 16 + m_file_id_l;
        }

        uint32_t data_id ( ) const
        {
            return m_data_id;
        }

        uint32_t data_len ( ) const
        {
            return m_data_len;
        }

    private:

        uint32_t m_file_id_h : 4;
        uint32_t m_data_id   : 28;
        uint32_t m_file_id_l : 4;
        uint32_t m_data_len  : 28;
    };

    inline bool operator== ( const content_id_t & lhd, const content_id_t & rhd )
    {
        return lhd.file_id ( ) == rhd.file_id ( ) && lhd.data_id ( ) == rhd.data_id ( ) && lhd.data_len ( ) == rhd.data_len ( );
    }

    inline bool operator!= ( const content_id_t & lhd, const content_id_t & rhd )
    {
        return lhd.file_id ( ) != rhd.file_id ( ) || lhd.data_id ( ) != rhd.data_id ( ) || lhd.data_len ( ) != rhd.data_len ( );
    }

#pragma pack( pop )

    class fullkey_t;

    class bucket_t
    {
    public:
        bucket_t ( );
        ~bucket_t ( );

        bool create ( 
                        const char * path 
                       );

        bool open_exist ( 
                            const char * path, 
                            bool read_write 
                            );

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

        void set_fullkey ( 
                            fullkey_t * p 
                            )
        {
            m_fullkey = p;
        }

        fullkey_t * get_fullkey ( )
        {
            return m_fullkey;
        }

        rwlockable_t & get_lock ( )
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

        fmap_t       m_data;
        fullkey_t *  m_fullkey;
        rwlockable_t m_lock;

    private:
        // disable
        bucket_t ( const bucket_t & );
        const bucket_t & operator= ( const bucket_t & );
    };

} // namespace md5db

#endif
