#ifndef _md5db_bucket_array_h_
#define _md5db_bucket_array_h_

#include "bucket.h"

namespace md5db
{
    class fullkey_array_t;

    class bucket_array_t
    {
    public:

        bucket_array_t ( );
        ~bucket_array_t ( );

        bool open ( 
                    const char * path 
                    );

        void close ( );

        bucket_t & get_bucket ( 
                                const void * inner_key, 
                                size_t inner_key_len 
                                );
        
        void set_fullkeys ( 
                            fullkey_array_t * p 
                            );

    private:

        bool create_buckets ( 
                                const char * path 
                                );

        bool open_exist_buckets ( 
                                    const char * path, 
                                    bool read_write 
                                    );

    private:

        bucket_t m_buckets[ BLOCK_FILE_COUNT ];

    private:
        // disable
        bucket_array_t ( const bucket_array_t & );
        const bucket_array_t & operator= ( const bucket_array_t & );
    };

} // namespace md5db

#endif
