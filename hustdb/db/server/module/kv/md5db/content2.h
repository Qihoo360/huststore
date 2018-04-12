#ifndef _md5db_content2_h_
#define _md5db_content2_h_

#include "db_lib.h"
#include "db_stdinc.h"
#include "../../base.h"
#include <map>
#include <sstream>
#include <iostream>

#define PAGE                     1024
#define FREE_BLOCK_INCR          16777216

namespace md5db
{

    struct free_block_t
    {
        uint32_t offset;
        uint32_t size;

        free_block_t ( )
        : offset ( 0 )
        , size ( 0 )
        {
        }
    }

    struct block_cmp_t
    {
        bool operator () ( const struct free_block_t * p, const struct free_block_t * q )
        {
            return p->size < q->size ? true : false;
        }
    };

    struct block_merge_t
    {
        bool operator () ( const struct free_block_t * p, const struct free_block_t * q )
        {
            return p->offset < q->offset ? true : false;
        }
    };

    class content2_t
    {
    public:
        content2_t ( );
        ~content2_t ( );

        bool open (
                    const char * path,
                    int file_id
                    );

        void close ( );

        void parse_free_block ( );

        bool incr_free_block_file ( );

        bool use_free_block (
                               uint32_t size,
                               uint32_t & offset
                               );

        bool add_free_block (
                                uint32_t size,
                                uint32_t offset
                                );

        bool write (
                     const char * data,
                     uint32_t data_len,
                     uint32_t & offset
                     );

        bool get (
                   uint32_t offset,
                   uint32_t data_len,
                   char * result
                   );

        bool update (
                      uint32_t & offset,
                      uint32_t old_data_len,
                      const char * data,
                      uint32_t data_len
                      );

        bool del (
                   uint32_t offset,
                   uint32_t data_len
                   );

        void info (
                    std::stringstream & ss
                    );

    private:

        bool write_inner (
                           const char * data,
                           uint32_t data_len,
                           uint32_t & offset,
                           uint32_t block,
                           uint32_t tail
                           );

    private:

        int m_file_id;
        char m_free_block_path[ 260 ];

        FILE * m_data_file;
        int m_data_fd;
        uint64_t m_data_file_size;

        char m_tail[ PAGE ];

        typedef std::multimap< struct free_block_t *, uint32_t, block_cmp_t > free_block_list_t;
        typedef std::multimap< struct free_block_t *, uint32_t, block_merge_t > free_block_merge_t;

        FILE * m_free_block_file;
        uint64_t m_free_block_file_size;

        fmap_t m_free_block_cache;
        uint32_t m_free_block_size;

        free_block_list_t m_free_block_list;
        free_block_merge_t m_free_block_merge;

        rwlockable_t m_lock;

    private:
        // disable
        content2_t ( const content2_t & );
        const content2_t & operator= ( const content2_t & );
    };

} // namespace md5db

#endif
