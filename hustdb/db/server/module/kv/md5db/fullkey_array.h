#ifndef _md5db_fullkey_array_h_
#define _md5db_fullkey_array_h_

#include "fullkey.h"

namespace md5db
{

    class fullkey_array_t
    {
    public:
        fullkey_array_t ( );
        ~fullkey_array_t ( );

        bool open (
                    const char * path
                    );

        void close ( );

        size_t size ( ) const
        {
            return COUNT_OF ( m_fullkeys );
        }

        fullkey_t & item (
                           size_t index
                           )
        {
            return m_fullkeys[ index ];
        }

        void info (
                    std::stringstream & ss
                    );

    private:

        fullkey_t & get_fullkey (
                                  const void * inner_key,
                                  size_t inner_key_len
                                  );

    private:

        fullkey_t m_fullkeys[ FULLKEY_FILE_COUNT ];

    private:
        // disable
        fullkey_array_t ( const fullkey_array_t & );
        const fullkey_array_t & operator= ( const fullkey_array_t & );
    };

} // namespace md5db

#endif
