#include "task_ttl_scan.h"
#include "../base.h"

task_ttl_scan_t * task_ttl_scan_t::create (
                                            uint32_t size,
                                            uint16_t start,
                                            uint16_t end
                                            )
{
    task_ttl_scan_t * p = NULL;

    try
    {
        p = new task_ttl_scan_t ( size, start, end );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[slow_task][ttl_scan]bad_alloc" );
    }

    return p;
}

void task_ttl_scan_t::release ( )
{
    delete this;
}

task_ttl_scan_t::task_ttl_scan_t (
                                   uint32_t size,
                                   uint16_t start,
                                   uint16_t end
                                   )
: m_size ( size )
, m_start ( start )
, m_end ( end )
{
}

task_ttl_scan_t::~ task_ttl_scan_t ( )
{
}

void task_ttl_scan_t::process ( )
{
    process_ttl_scan ();
}

void task_ttl_scan_t::process_ttl_scan ( )
{
    try
    {
        if ( m_size < 0 )
        {
            LOG_ERROR ( "[slow_task][ttl_scan]scan size < 0" );
            return;
        }
        
        i_server_kv_t * db = ( ( hustdb_t * ) G_APPTOOL->get_hustdb () )->get_storage ();
        
        struct export_cb_param_t cb_pm;
        cb_pm.start        = m_start;
        cb_pm.end          = m_end;
        cb_pm.size         = m_size;
        cb_pm.noval        = false;

        int r = db->ttl_scan ( NULL, & cb_pm );
        if ( 0 != r )
        {
            LOG_ERROR ( "[slow_task][ttl_scan][r=%d]task failed", r );
            return;
        }
    }
    catch ( ... )
    {
        LOG_ERROR ( "[slow_task][ttl_scan]task exception" );
    }
}
