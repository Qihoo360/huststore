#include <string>
#include "file.h"

class Item
{
public:
    Item ( uint32_t pos, File *_file );
    ~Item ( );
    void set_query_string ( std::string );
    std::string get_query_string ( );
    void set_method ( std::string );
    std::string get_method ( );
    void set_value ( std::string );
    std::string get_value ( );
    unsigned int get_bitmap_index ( );
    File *get_file ( );
    std::string get_path ( );
    void set_path ( std::string );
private:
    std::string query_string;
    std::string method;
    std::string value;
    uint32_t bitmap_index;
    File *file;
    std::string path;
};
