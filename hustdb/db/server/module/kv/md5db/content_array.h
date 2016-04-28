#ifndef _md5db_content_array_h_
#define _md5db_content_array_h_

#include "content.h"
#include "bucket.h"
#include <vector>
#include <string>
#include <sstream>

namespace md5db
{

    class content_array_t
    {
    public:

        static bool enable ( const char * storage_conf );

        content_array_t ( );
        ~content_array_t ( );

        bool is_open ( )
        {
            return !m_contents.empty ( );
        }

        bool open (
                    const char * path,
                    const char * storage_conf
                    );

        bool open (
                    const char * path,
                    ini_t & ini
                    );

        bool open (
                    const char * path,
                    int count,
                    int cache
                    );

        void close ( );

        bool write (
                     const char * data,
                     uint32_t data_len,
                     uint32_t & file_id,
                     uint32_t & data_id
                     );

        bool update (
                      uint32_t & file_id,
                      uint32_t & data_id,
                      uint32_t old_data_len,
                      const char * data,
                      uint32_t data_len
                      );

        bool get (
                   uint32_t file_id,
                   uint32_t data_id,
                   uint32_t data_len,
                   char * result
                   );

        bool del (
                   uint32_t file_id,
                   uint32_t data_id,
                   uint32_t data_len
                   );

        void info (
                    std::stringstream & ss
                    );

    private:

        typedef std::vector< content_t * > container_t;

        container_t m_contents;
        size_t m_token;

    private:
        // disable
        content_array_t ( const content_array_t & );
        const content_array_t & operator= ( const content_array_t & );
    };

} // namespace md5db

#endif
