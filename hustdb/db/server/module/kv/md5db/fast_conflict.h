#ifndef _md5db_fast_conflict_h_
#define _md5db_fast_conflict_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"
#include "bucket.h"
#include <sstream>

namespace md5db
{

#pragma pack( push, 1 )

    struct fast_conflict_header_t
    {
        uint32_t file_version;
        uint32_t count;
        uint32_t free_list_id;
    };

#pragma pack( pop )

#define FAST_CONFLICT_FILE_COUNT      256
#define FAST_CONFLICT_BYTES_PER_FILE  (0xFFFFFFFF - 1)

    class fast_conflict_t
    {
    public:
        fast_conflict_t ( );
        ~fast_conflict_t ( );

        bool open (
                    const char * path,
                    int file_id,
                    int conflict_count
                    );

        void close ( );

        bool get (
                   const block_id_t & addr,
                   bucket_data_item_t * & result,
                   size_t * result_count
                   );

        bool del (
                   const block_id_t & addr
                   );

        bool write (
                     const bucket_data_item_t & bucket_item_1,
                     const bucket_data_item_t & bucket_item_2,
                     block_id_t & addr
                     );

        void info (
                    std::stringstream & ss
                    );

    private:

        bool generate_addr (
                             block_id_t & addr,
                             uint32_t file_id,
                             uint32_t index
                             );

        bool parse_addr (
                          const block_id_t & addr,
                          uint32_t & file_id,
                          uint32_t & index
                          );

        bucket_data_item_t * alloc_item (
                                          fast_conflict_header_t * & header
                                          );

    private:

        int m_conflict_count;
        int m_file_id;
        char m_path[ 260 ];
        fmap_t m_data;
        FILE * m_file;

    private:
        // disable
        fast_conflict_t ( const fast_conflict_t & );
        const fast_conflict_t & operator= ( const fast_conflict_t & );
    };

} // namespace md5db

#endif
