#include "task_export.h"
#include "../base.h"

task_export_t * task_export_t::create (
                                        int file_id,
                                        const char * path,
                                        uint32_t offset,
                                        uint32_t size,
                                        uint16_t start,
                                        uint16_t end,
                                        bool noval,
                                        bool cover
                                        )
{
    task_export_t * p = NULL;
    
    try
    {
        p = new task_export_t ( file_id, path, offset, size, start, end, noval, cover );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[slow_task][export]bad_alloc" );
    }
    
    return p;
}

void task_export_t::release ( )
{
    delete this;
}

task_export_t::task_export_t (
                               int file_id,
                               const char * path,
                               uint32_t offset,
                               uint32_t size,
                               uint16_t start,
                               uint16_t end,
                               bool noval,
                               bool cover
                               )
: m_file_id ( file_id )
, m_path ( )
, m_offset ( offset )
, m_size ( size )
, m_start ( start )
, m_end ( end )
, m_noval ( noval )
, m_cover ( cover )
{
    try
    {
        m_path = path ? path : "";
    }
    catch ( ... )
    {
        LOG_ERROR ( "[slow_task][export]bad_alloc" );
    }
}

task_export_t::~ task_export_t ( )
{
}

void task_export_t::process ( )
{
    process_export_db ();
}

void task_export_t::process_export_db ( )
{
    try
    {
        if ( m_file_id < 0 || m_path.empty () )
        {
            LOG_ERROR ( "[slow_task][export]m_file_id < 0 or m_path empty" );
            return;
        }

        char            i_ph[ 256 ]     = { };
        char            d_ph[ 256 ]     = { };
        const char      flag_k[]        = "k";
        const char      flag_kv[]       = "kv";
        const char *    noval_flag      = NULL;

        noval_flag = m_noval ? flag_k : flag_kv;

        i_server_kv_t * db = ( ( hustdb_t * ) G_APPTOOL->get_hustdb () )->get_storage ();
        if ( ! db )
        {
            LOG_ERROR ( "[slow_task][export]get_storage() return NULL" );
            return;
        }
        
        if ( strcmp ( m_path.c_str (), EXPORT_DB_ALL ) == 0 )
        {
            int file_count = db->get_user_file_count ();
            
            for ( int i = 0; i < file_count; i ++ )
            {
                if ( i == m_file_id )
                {
                    continue;
                }
                
                sprintf ( i_ph, "./EXPORT/%s%d[%d-%d].%s", m_path.c_str (), i, m_start, m_end, noval_flag );
                G_APPTOOL->path_to_os ( i_ph );
                
                if ( G_APPTOOL->is_file ( i_ph ) )
                {
                    unlink ( i_ph );
                }
                
                sprintf ( d_ph, "./EXPORT/%s%d[%d-%d].%s.data", m_path.c_str (), i, m_start, m_end, noval_flag );
                G_APPTOOL->path_to_os ( d_ph );
                
                if ( G_APPTOOL->is_file ( d_ph ) )
                {
                    unlink ( d_ph );
                }
            }
        }

        sprintf ( i_ph, "./EXPORT/%s%d[%d-%d].%s", m_path.c_str (), m_file_id, m_start, m_end, noval_flag );
        G_APPTOOL->path_to_os ( i_ph );

        sprintf ( d_ph, "./EXPORT/%s%d[%d-%d].%s.data", m_path.c_str (), m_file_id, m_start, m_end, noval_flag );
        G_APPTOOL->path_to_os ( d_ph );

        if ( m_cover && ( G_APPTOOL->is_file ( i_ph ) || G_APPTOOL->is_file ( d_ph ) ) )
        {
            unlink ( i_ph );
            unlink ( d_ph );
        }

        struct export_cb_param_t cb_pm;
        cb_pm.start        = m_start;
        cb_pm.end          = m_end;
        cb_pm.offset       = m_offset;
        cb_pm.size         = m_size;
        cb_pm.noval        = m_noval;

        int r = db->export_db ( m_file_id, m_path.c_str (), NULL, & cb_pm );
        if ( 0 != r )
        {
            LOG_ERROR ( "[slow_task][export]export( %d, %s ) failed: %d", m_file_id, m_path.c_str (), r );
            return;
        }

        LOG_INFO ( "[slow_task][export]export( %d, %s ) OK", m_file_id, m_path.c_str () );
    }
    catch ( ... )
    {
        LOG_ERROR ( "[slow_task][export]export( %d, %s ) exception", m_file_id, m_path.c_str () );
    }
}
