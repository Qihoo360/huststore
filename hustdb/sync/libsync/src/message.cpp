#include "../lib/message.h"

Message::Message ( std::string _data, int _pos, void *_file )
: data ( _data )
, pos ( _pos )
, file ( _file )
{
}

Message::~ Message ( )
{
}

std::string Message::get_data ( )
{
    return data;
}

int Message::get_pos ( )
{
    return pos;
}

void *Message::get_file ( )
{
    return file;
}
