#ifndef _store_doc_bloom_filter_h_
#define _store_doc_bloom_filter_h_

#include "db_stdinc.h"
#include "db_lib.h"
#include "../../base.h"

enum md5_bloom_mode_t
{
    MD5_BLOOM_DISABLED  = 0,
    MD5_BLOOM_LARGE     = 1,
    MD5_BLOOM_SMALL     = 2
};

class md5_bloom_filter_t
{
public:

    md5_bloom_filter_t();
    ~md5_bloom_filter_t();

    bool open( const char * path, md5_bloom_mode_t type );
    void close();

    bool not_found( const char * md5 );
    void add_key( const char * md5 );

private:

    fmap_t              m_fmap;
    unsigned char *     m_data;
    md5_bloom_mode_t    m_type;

private:
    // disable
    md5_bloom_filter_t(const md5_bloom_filter_t &);
    const md5_bloom_filter_t & operator = (const md5_bloom_filter_t &);
};

#endif // #ifndef _store_doc_bloom_filter_h_
