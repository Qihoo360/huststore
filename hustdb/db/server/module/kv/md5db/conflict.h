#ifndef _md5db_conflict_h_
#define _md5db_conflict_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "i_kv.h"
#include "../../base.h"

namespace md5db
{

    class block_id_t;

    class conflict_t
    {
    public:
        conflict_t ( );
        ~conflict_t ( );

        bool open (
                    const char * path,
                    const kv_config_t & config,
                    int file_id );

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

        i_kv_t * get_kv ( )
        {
            return m_db;
        }

    private:

        i_kv_t * m_db;

    private:
        // disable
        conflict_t ( const conflict_t & );
        const conflict_t & operator= ( const conflict_t & );
    };

} // namespace md5db

#endif
