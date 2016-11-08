#ifndef _md5db_content_h_
#define _md5db_content_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"
#include <sstream>
#include <map>
#include <iostream>

#define PAGE                 1024
#define BITMAP               262144
#define BITMAP_BIT           2097152
#define CONTENT_FILE_LEN     2147483648

#define BITMAP_MAX           1073741824
#define PAGE512              524288

#define MICRO_IN_SEC         1000000.00

#define BITSET0(X)           m_bitmap.ptr[ X / 8 ] &= ~( 128 >> X % 8 );
#define BITSET1(X)           m_bitmap.ptr[ X / 8 ] |= ( 128 >> X % 8 );
#define BITGET(X)            m_bitmap.ptr[ X / 8 ] & ( 128 >> X % 8 )

#define CACHE_BITSET0(X)     m_cache_bitmap.ptr[ X / 8 ] &= ~( 128 >> X % 8 );
#define CACHE_BITSET1(X)     m_cache_bitmap.ptr[ X / 8 ] |= ( 128 >> X % 8 );
#define CACHE_BITGET(X)      m_cache_bitmap.ptr[ X / 8 ] & ( 128 >> X % 8 )

namespace md5db
{

    struct data_index_t
    {
        uint32_t id : 1;
        uint32_t offset : 31;

        data_index_t ( )
        : id ( 0 )
        , offset ( 0 )
        {
        }
    };

    class content_t
    {
    public:
        content_t ( );
        ~content_t ( );

        bool open (
                    const char * path,
                    int file_id,
                    int cache
                    );

        void close ( );

        void parse_free_list ( );

        void parse_cache_free_list ( );

        bool find_free_block (
                               uint32_t block,
                               uint32_t & offset
                               );

        bool find_cache_free_block (
                                     uint32_t block,
                                     uint32_t & offset
                                     );

        bool merge_free_block (
                                uint32_t block,
                                uint32_t offset
                                );

        bool merge_cache_free_block (
                                      uint32_t block,
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
        FILE * m_data_file;
        uint32_t m_file_size;

        char m_tail[ PAGE ];

        typedef std::multimap < uint32_t, uint32_t > free_list_t;

        fmap_t m_bitmap;
        uint32_t m_free_size;
        free_list_t m_free_list;
        double m_parse_time;
        uint32_t m_merge_valve;

        fmap_t m_cache_file;
        fmap_t m_cache_bitmap;
        uint32_t m_cache_free_size;
        free_list_t m_cache_free_list;
        double m_cache_parse_time;
        uint32_t m_cache_merge_valve;

        uint32_t m_cache_file_length;
        uint32_t m_cache_file_bitmap_length;
        uint32_t m_cache_file_bitmap_bit_length;

        rwlockable_t m_lock;

    private:
        // disable
        content_t ( const content_t & );
        const content_t & operator= ( const content_t & );
    };

} // namespace md5db

#endif
