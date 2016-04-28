#include <string>

class Message
{
public:
    Message ( std::string, int, void * );
    ~Message ( );
    std::string get_data ( );
    int get_pos ( );
    void *get_file ( );
private:
    std::string data;
    int pos;
    void *file;
};
