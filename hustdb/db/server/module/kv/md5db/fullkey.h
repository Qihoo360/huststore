#ifndef _md5db_fullkey_h_
#define _md5db_fullkey_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"
#include <sstream>

namespace md5db
{

    struct fullkey_data_t;
    struct fullkey_header_t;
    class block_id_t;

#define FULLKEY_FILE_COUNT      256
#define FILEKEY_BYTES_PER_FILE  (0xFFFFFFFF - 1)

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
                       unsigned int inner_key_len,
                       const char * user_key = NULL,
                       unsigned int user_key_len = 0
                       );

        bool get (
                   const block_id_t & block_id,
                   char * resp_since_4
                   );

        bool del (
                   const block_id_t & block_id
                   );

        void info (
                    std::stringstream & ss
                    );

    private:

        void fill_user_key (
                             const char * user_key,
                             unsigned int user_key_len,
                             fullkey_data_t * item
                             );

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

        int m_file_id;
        char m_path[ 260 ];
        fmap_t m_data;
        FILE * m_file;

    private:
        // disable
        fullkey_t ( const fullkey_t & );
        const fullkey_t & operator= ( const fullkey_t & );
    };

} // namespace md5db

#endif
