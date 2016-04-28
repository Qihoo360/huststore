#ifndef _md5db_fast_conflict_array_h_
#define _md5db_fast_conflict_array_h_

#include "fast_conflict.h"

namespace md5db
{

    class bucket_array_t;

    class fast_conflict_array_t
    {
    public:
        fast_conflict_array_t ( );
        ~fast_conflict_array_t ( );

        bool open (
                    const char * path,
                    bucket_array_t * buckets,
                    const char * storage_conf
                    );

        bool open (
                    const char * path,
                    bucket_array_t * buckets,
                    int conflict_count
                    );

        void close ( );

        size_t size ( ) const
        {
            return COUNT_OF ( m_fast_conflicts );
        }

        fast_conflict_t & item (
                                 size_t index
                                 )
        {
            return m_fast_conflicts[ index ];
        }

        void info (
                    std::stringstream & ss
                    );

        int get (
                  const void * inner_key,
                  size_t inner_key_len,
                  md5db::block_id_t & block_id,
                  uint32_t & version
                  );

        bool write (
                     const void * first_inner_key,
                     size_t first_inner_key_len,
                     const block_id_t & first_block_id,
                     uint32_t first_version,
                     const void * second_inner_key,
                     size_t second_inner_key_len,
                     const block_id_t & second_block_id,
                     uint32_t second_version,
                     block_id_t & addr
                     );

        int add (
                  const void * inner_key,
                  size_t inner_key_len,
                  const md5db::block_id_t & block_id,
                  uint32_t version
                  );

        int update (
                     const void * inner_key,
                     size_t inner_key_len,
                     const md5db::block_id_t & block_id,
                     uint32_t version
                     );

        int del (
                  const void * inner_key,
                  size_t inner_key_len
                  );

    private:

        fast_conflict_t & get_fast_conflict ( const void * inner_key, size_t inner_key_len );

    private:

        fast_conflict_t m_fast_conflicts[ FAST_CONFLICT_FILE_COUNT ];
        bucket_array_t * m_bucket_array;

    private:
        // disable
        fast_conflict_array_t ( const fast_conflict_array_t & );
        const fast_conflict_array_t & operator= ( const fast_conflict_array_t & );
    };

} // namespace md5db

#endif
