#ifndef _task_export_h_
#define _task_export_h_

#include "../hustdb.h"
#include "../slow_task_thread.h"
#include <string>

class task_export_t : public task2_t
{
public:

    static task_export_t * create (
                                    int file_id,
                                    const char * path,
                                    uint32_t offset,
                                    uint32_t size,
                                    uint16_t start = 0,
                                    uint16_t end = MAX_BUCKET_NUM,
                                    bool noval = true,
                                    bool cover = false
                                    );

    virtual void release ( );

    virtual void process ( );

private:

    void process_export_db ( );

private:

    int m_file_id;
    std::string m_path;
    uint32_t m_offset;
    uint32_t m_size;
    uint16_t m_start;
    uint16_t m_end;
    bool m_noval;
    bool m_cover;

public:

    task_export_t (
                    int file_id,
                    const char * path,
                    uint32_t offset,
                    uint32_t size,
                    uint16_t start,
                    uint16_t end,
                    bool noval,
                    bool cover
                    );
    ~task_export_t ( );
};

#endif
