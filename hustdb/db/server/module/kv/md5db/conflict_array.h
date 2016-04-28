#ifndef _md5db_conflict_array_h_
#define _md5db_conflict_array_h_

#include "conflict.h"
#include <vector>
#include <string>
#include <sstream>

namespace md5db
{

    class block_id_t;

    class conflict_array_t
    {
    public:
        conflict_array_t ( );
        ~conflict_array_t ( );

        bool open (
                    const char * path,
                    const char * storage_conf
                    );

        void close ( );

        int del (
                  const void * inner_key,
                  unsigned int inner_key_len
                  );

        int put (
                  const void * inner_key,
                  unsigned int inner_key_len,
                  const block_id_t & block_id,
                  uint32_t version
                  );

        int get (
                  const void * inner_key,
                  unsigned int inner_key_len,
                  block_id_t & block_id,
                  uint32_t & version
                  );

        void info (
                    std::stringstream & ss
                    );

        int size ( ) const
        {
            return ( int ) m_conflicts.size ( );
        }

        conflict_t * item ( int index );

    private:

        bool open (
                    const char * path,
                    ini_t & ini
                    );

        bool open (
                    const char * path,
                    int count,
                    int cache_m,
                    int write_buffer_m,
                    int bloom_filter_bits,
                    bool disable_compression,
                    int md5_bloom_filter_type
                    );

        conflict_t & get_conflict (
                                    const void * inner_key,
                                    size_t inner_key_len
                                    );

    private:

        typedef std::vector< conflict_t * > container_t;

        container_t m_conflicts;

    private:
        // disable
        conflict_array_t ( const conflict_array_t & );
        const conflict_array_t & operator= ( const conflict_array_t & );
    };

} // namespace md5db

#endif
