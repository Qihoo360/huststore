#ifndef _md5db_fullkey_h_
#define _md5db_fullkey_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "bucket.h"
#include "../../base.h"
#include <sstream>

namespace md5db
{

    class  block_id_t;
    class  content_id_t;

#define FULLKEY_FILE_COUNT      256
#define FILEKEY_BYTES_PER_FILE  (0xFFFFFFFF - 1)
#define DEFAULT_ITEM_COUNT      400000
#define EXPAND_ITEM_COUNT       400000

#pragma pack( push, 1 )

    struct fullkey_header_t
    {
        fullkey_header_t ( )
        {
            reset ( );
        }

        void reset ( )
        {
            file_version = 0;
            count        = 0;
            free_list_id = 0;
        }

        uint32_t    file_version;
        uint32_t    count;
        uint32_t    free_list_id;
        char        data[ 9 ];
    };

    struct fullkey_data_t
    {
        fullkey_data_t ( )
        {
            reset ( );
        }

        void reset ( )
        {
            memset ( m_md5_suffix, 0, 13 );
            m_content_id.reset ();
        }

        char           m_md5_suffix[ 13 ];
        content_id_t   m_content_id;
    };

    typedef fullkey_data_t      data_t;
    typedef fullkey_header_t    header_t;

#pragma pack( pop )

    class fullkey_t
    {
    public:
        fullkey_t ( );
        ~fullkey_t ( );

        bool open (
                    const char * path,
                    int file_id
                    );

        void close ( );

        bool write (
                     const char * inner_key,
                     unsigned int inner_key_len,
                     const char * user_key,
                     unsigned int user_key_len,
                     block_id_t & block_id
                     );

        bool compare (
                       const block_id_t & block_id,
                       const char * inner_key,
                       unsigned int inner_key_len
                       );

        bool get (
                   const block_id_t & block_id,
                   char * resp_since_4
                   );

        bool del (
                   const block_id_t & block_id
                   );

        bool set_content_id (
                              const block_id_t & block_id,
                              content_id_t & content_id
                             );

        bool get_content_id (
                                const block_id_t & block_id,
                                content_id_t & content_id
                                );

        void info (
                    std::stringstream & ss
                    );

    private:

        bool generate_block_id (
                                 block_id_t & block_id,
                                 uint32_t file_id,
                                 uint32_t index
                                 );

        bool parse_block_id (
                              const block_id_t & block_id,
                              uint32_t & file_id,
                              uint32_t & index
                              );

        fullkey_data_t * alloc_item (
                                      fullkey_header_t * & header
                                      );

    private:

        int    m_file_id;
        char   m_path[ 260 ];
        fmap_t m_data;
        FILE * m_file;

    private:
        // disable
        fullkey_t ( const fullkey_t & );
        const fullkey_t & operator= ( const fullkey_t & );
    };

} // namespace md5db

#endif
