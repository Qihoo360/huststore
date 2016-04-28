#include <string>
#include <stdint.h>
#include <bitset>
#include <vector>

#define FILE_BIT_MAX 10000000

class File
{
public:
    File ( std::string );
    File ( std::string, std::bitset<FILE_BIT_MAX> & );
    ~File ( );

    void set_last_read_pos ( size_t );
    size_t get_last_read_pos ( );

    void set_last_read_count ( uint32_t );
    unsigned int get_last_read_count ( );

    std::string get_path ( );

    void set_bitmap ( uint32_t );
    std::bitset<FILE_BIT_MAX> get_bitmap ( );

    void set_index ( int );
    int get_index ( );

    void set_addr ( char * );
    void *get_addr ( );

    void set_status_path ( std::string & );
    std::string get_status_path ( );
private:
    size_t last_read_pos;
    uint32_t last_read_count;
    std::string path;
    std::string status_path;
    std::bitset<FILE_BIT_MAX> bitmap;
    int index;
    void *addr;
};
