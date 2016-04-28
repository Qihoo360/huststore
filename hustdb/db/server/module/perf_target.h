#ifndef _store_doc_perf_target_h_
#define _store_doc_perf_target_h_

#include "db_stdinc.h"
#include "base.h"
#include <sstream>

class perf_target_t
{
public:

    perf_target_t ( )
    : total_ms ( 0 )
    , times ( 0 )
    , slowest ( 0 )
    {
    }

    size_t total_ms;
    size_t times;
    size_t slowest;

private:
    //disable
    perf_target_t ( const perf_target_t & );
    const perf_target_t & operator= ( const perf_target_t & );
};

class scope_perf_target_t
{
public:

    scope_perf_target_t ( perf_target_t & perf )
#if ENABLE_PERF_INFO
    : m_perf ( perf )
    , m_cancel ( false )
#endif
    {
#if ENABLE_PERF_INFO
        m_start = G_APPTOOL->get_tick_count ( );
#endif
    }

    ~scope_perf_target_t ( )
    {
#if ENABLE_PERF_INFO
        if ( !m_cancel )
        {
            uint32_t stop = G_APPTOOL->get_tick_count ( );
            m_perf.total_ms += ( stop - m_start );
            ++m_perf.times;
            if ( stop - m_start > m_perf.slowest )
            {
                m_perf.slowest = stop - m_start;
            }
        }
#endif
    }

    void cancel ( )
    {
#if ENABLE_PERF_INFO
        m_cancel = true;
#endif
    }

#if ENABLE_PERF_INFO
    uint32_t m_start;
    perf_target_t & m_perf;
    bool m_cancel;
#endif

private:
    // disable
    scope_perf_target_t ( );
    scope_perf_target_t ( const scope_perf_target_t & );
    const scope_perf_target_t & operator= ( const scope_perf_target_t & );
};

inline void write_perf_info (
                              std::stringstream & ss,
                              int file_id,
                              const char * name,
                              perf_target_t & perf,
                              bool footer = false
                              )
{
    try
    {
        int per_ms;
        
        if ( 0 == perf.times )
        {
            per_ms = 0;
        }
        else if ( 0 == perf.total_ms )
        {
            per_ms = ( int ) perf.times;
        }
        else
        {
            per_ms = ( int ) ( perf.times / perf.total_ms );
        }
        
        ss << "\"" << file_id << "|" << name << "\":{"
            "\"speed\":" << per_ms << ","
            "\"slowest\":" << ( int ) perf.slowest << ","
            "\"elapsed\":" << ( int ) perf.total_ms << ","
            "\"total\":" << ( int ) perf.times << "}";
        
        if ( ! footer )
        {
            ss << ",";
        }

        perf.slowest = 0;
        perf.total_ms = 0;
        perf.times = 0;
    }
    catch ( ... )
    {
        LOG_ERROR ( "bad_alloc" );
    }
}

inline void write_single_count (
                                 std::stringstream & ss,
                                 int file_id,
                                 const char * name,
                                 size_t & count,
                                 bool footer = false
                                 )
{
    try
    {
        ss << "\"" << file_id << "|" << name << "\":{"
            "\"total\":" << count << "}";
        
        if ( ! footer )
        {
            ss << ",";
        }

        count = 0;
    }
    catch ( ... )
    {
        LOG_ERROR ( "bad_alloc" );
    }
}

#endif
