#include "../lib/item.h"

Item::Item ( uint32_t pos, File *_file )
: bitmap_index ( pos )
, file ( _file )
{
}

Item::~ Item ( )
{
}

void Item::set_query_string ( std::string query )
{
    query_string = query;
}

std::string Item::get_query_string ( )
{
    return query_string;
}

void Item::set_method ( std::string _method )
{
    method = _method;
}

std::string Item::get_method ( )
{
    return method;
}

std::string Item::get_value ( )
{
    return value;
}

void Item::set_value ( std::string _value )
{
    value = _value;
}

File *Item::get_file ( )
{
    return file;
}

unsigned int Item::get_bitmap_index ( )
{
    return bitmap_index;
}

std::string Item::get_path ( )
{
    return path;
}

void Item::set_path ( std::string _path )
{
    path = _path;
}
