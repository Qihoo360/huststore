#include "../lib/file.h"
#include <sys/mman.h>
#include <pthread.h>
#include <cstdio>

extern std::vector<char *> total_status_addrs;

static pthread_mutex_t bitmap_lock = PTHREAD_MUTEX_INITIALIZER;

File::File ( std::string _path ) : last_read_pos ( 0 ), last_read_count ( 0 ), path ( _path )
{
}

File::File ( std::string _path, std::bitset<FILE_BIT_MAX> &_bitmap )
: last_read_pos ( 0 ), last_read_count ( 0 ), path ( _path )
{
    bitmap |= _bitmap;
}

File::~ File ( )
{
    msync (addr, FILE_BIT_MAX, MS_ASYNC);
    munmap (addr, FILE_BIT_MAX);
    remove (path.c_str ());
    remove (status_path.c_str ());
}

size_t File::get_last_read_pos ( )
{
    return last_read_pos;
}

void File::set_last_read_pos ( size_t size )
{
    last_read_pos = size;
    *( uint32_t * ) ( ( char * ) addr ) = last_read_pos;
}

void File::set_last_read_count ( uint32_t cnt )
{
    last_read_count = cnt;
    *( uint32_t* ) ( ( char * ) addr + 4 ) = last_read_count;
    //int total = *(int *)total_status_addr;
    //*(int *)total_status_addr = total + last_read_count;
}

unsigned int File::get_last_read_count ( )
{
    return last_read_count;
}

std::string File::get_path ( )
{
    return path;
}

std::bitset<FILE_BIT_MAX> File::get_bitmap ( )
{
    return bitmap;
}

void File::set_bitmap ( uint32_t pos )
{
    pthread_mutex_lock (&bitmap_lock);

    bitmap.set (pos);
    uint32_t sync_cnt = * ( uint32_t * ) ( ( char* ) addr + 2 * sizeof (uint32_t ) );
    *( uint32_t * ) ( ( char* ) addr + 2 * sizeof (uint32_t ) ) = sync_cnt + 1;

    char *single_sync_addr = total_status_addrs[index];
    uint64_t single_sync_cnt = * ( uint64_t * ) ( single_sync_addr + sizeof (uint64_t ) );
    *( uint64_t * ) ( single_sync_addr + sizeof (uint64_t ) ) = single_sync_cnt + 1;

    pthread_mutex_unlock (&bitmap_lock);
    *( ( char * ) addr + FILE_BIT_MAX - 1 - pos ) = '1';
}

void File::set_index ( int _index )
{
    index = _index;
}

int File::get_index ( )
{
    return index;
}

void File::set_addr ( char *_addr )
{
    addr = ( void * ) _addr;
}

void *File::get_addr ( )
{
    return addr;
}

void File::set_status_path ( std::string &_status_path )
{
    status_path = _status_path;
}

std::string File::get_status_path ( )
{
    return status_path;
}