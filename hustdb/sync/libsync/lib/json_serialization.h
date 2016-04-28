#ifndef __json_serialization_20150528111839_h__
#define __json_serialization_20150528111839_h__

#include <typeinfo>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include "../json_src/include/json/value.h"
#include "../json_src/include/json/writer.h"
#include "../json_src/include/json/reader.h"

// interface
namespace json_lib
{
    typedef Json::Int64 int64;
    typedef Json::UInt64 uint64;

    template<typename T>
    std::string GetName ( const T& obj );

    template<typename T>
    std::string ToString ( const T& obj_val );

    template<typename T>
    T ToNumber ( const std::string& str );
}

namespace json_lib
{
    bool Serialize ( const bool& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, bool& obj_val );

    bool Serialize ( const std::string& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, std::string& obj_val );

    bool Serialize ( const int& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, int& obj_val );

    bool Serialize ( const unsigned int& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, unsigned int& obj_val );

    bool Serialize ( const double& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, double & obj_val );

    bool Serialize ( const float& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, float & obj_val );

    bool Serialize ( const json_lib::int64& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, json_lib::int64& obj_val );

    bool Serialize ( const json_lib::uint64& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, json_lib::uint64& obj_val );
}

namespace json_lib
{
    bool Serialize ( const char& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, char& obj_val );

    bool Serialize ( const unsigned char& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, unsigned char& obj_val );

    bool Serialize ( const short& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, short& obj_val );

    bool Serialize ( const unsigned short& obj_val, Json::Value& json_val );
    bool Deserialize ( const Json::Value& json_val, unsigned short& obj_val );

    /**
     * special for enum type
     */
    template<typename T>
    bool Serialize ( const T& obj_val, Json::Value& json_val );
    template<typename T>
    bool Deserialize ( const Json::Value& json_val, T& obj_val );
} // json_lib

namespace json_lib
{
    template<typename T>
    bool Serialize ( const std::vector<T>& obj_val, Json::Value& json_val );
    template<typename T>
    bool Deserialize ( const Json::Value& json_val, std::vector<T>& obj_val );

    template<typename T>
    bool Serialize ( const std::map<std::string, T>& obj_val, Json::Value& json_val );
    template<typename T>
    bool Deserialize ( const Json::Value& json_val, std::map<std::string, T>& obj_val );

    template<typename Number, typename T>
    bool Serialize ( const std::map<Number, T>& obj_val, Json::Value& json_val );
    template<typename Number, typename T>
    bool Deserialize ( const Json::Value& json_val, std::map<Number, T>& obj_val );
} // json_lib

namespace json_lib
{
    template<typename T>
    bool Serialize ( const T& field, const std::string& field_name, Json::Value& json_val );
    template<typename T>
    bool Deserialize ( const Json::Value& json_val, const std::string& field_name, T& field, bool& field_in_json );
} // json_lib

namespace json_lib
{
    template<typename T>
    bool Serialize ( const T& obj_val, std::string& json_val );
    template<typename T>
    bool Deserialize ( const std::string& json_val, T& obj_val );
} // json_lib

namespace json_lib
{
    template<typename T>
    bool Save ( const T& obj_val, const char * path );

    template<typename T>
    bool Load ( const char * path, T& obj_val );
};

// implement
namespace json_lib
{

    template <typename T>
    std::string GetName ( const T& obj )
    {
        try
        {
            return typeid (obj ).name ( );
        }
        catch ( ... )
        {
            return "";
        }
    }

    template <typename T>
    std::string ToString ( const T& obj_val )
    {
        std::stringstream ss;
        ss << obj_val;
        return ss.str ( );
    }

    template <typename T>
    T ToNumber ( const std::string& str )
    {
        std::stringstream ss ( str );
        T result;
        return ss >> result ? result : 0;
    }
}

namespace json_lib
{

    template<typename T>
    bool Serialize ( const T& obj_val, Json::Value& json_val )
    {
        const std::string& obj_type = json_lib::GetName ( obj_val );
        if ( obj_type.find ( "enum" ) < 0 )
        {
            return false;
        };
        int tmp_obj_val = static_cast < int > ( obj_val );
        return Serialize ( tmp_obj_val, json_val );
    }

    template<typename T>
    bool Deserialize ( const Json::Value& json_val, T& obj_val )
    {
        const std::string& obj_type = json_lib::GetName ( obj_val );
        if ( obj_type.find ( "enum" ) < 0 )
        {
            return false;
        };
        int tmp_obj_val = 0;
        if ( !Deserialize ( json_val, tmp_obj_val ) )
        {
            return false;
        }
        T _obj_val = static_cast < T > ( tmp_obj_val );
        obj_val = _obj_val;
        return true;
    }

} // json_lib

namespace json_lib
{

    template<typename T>
    bool Serialize ( const std::map<std::string, T>& obj_val, Json::Value& json_val )
    {
        Json::Value tmp_val ( Json::objectValue ); // initialize tmp_val as objectValue
        typename std::map<std::string, T>::const_iterator it;
        for ( it = obj_val.begin ( ); it != obj_val.end ( ); ++it )
        {
            Json::Value val;
            if ( !Serialize ( it->second, val ) )
            {
                return false;
            }
            tmp_val[it->first] = val;
        }
        json_val = tmp_val;
        return true;
    }

    template<typename T>
    bool Deserialize ( const Json::Value& json_val, std::map<std::string, T>& obj_val )
    {
        std::map<std::string, T> tmp_map;
        int size = json_val.size ( );
        if ( size > 0 )
        {
            int count = 0;
            Json::Value::iterator it = json_val.begin ( );
            while ( true )
            {
                T val;
                if ( !Deserialize ( *it, val ) )
                {
                    return false;
                }
                tmp_map[it.key ( ).asString ( )] = val;
                ++it;
                ++count;
                if ( count > size - 1 )
                {
                    break;
                }
            }
        }
        tmp_map.swap ( obj_val );
        return true;
    }

    template<typename Number, typename T>
    bool Serialize ( const std::map<Number, T>& obj_val, Json::Value& json_val )
    {
        Json::Value tmp_val ( Json::objectValue ); // initialize tmp_val as objectValue
        typename std::map<Number, T>::const_iterator it;
        for ( it = obj_val.begin ( ); it != obj_val.end ( ); ++it )
        {
            Json::Value val;
            if ( !Serialize ( it->second, val ) )
            {
                return false;
            }
            tmp_val[json_lib::ToString <Number> ( it->first )] = val;
        }
        json_val = tmp_val;
        return true;
    }

    template<typename Number, typename T>
    bool Deserialize ( const Json::Value& json_val, std::map<Number, T>& obj_val )
    {
        std::map<Number, T> tmp_map;
        int size = json_val.size ( );
        if ( size > 0 )
        {
            int count = 0;
            Json::Value::iterator it = json_val.begin ( );
            while ( true )
            {
                T val;
                if ( !Deserialize ( *it, val ) )
                {
                    return false;
                }
                tmp_map[json_lib::ToNumber <Number> ( it.key ( ).asString ( ) )] = val;
                ++it;
                ++count;
                if ( count > size - 1 )
                {
                    break;
                }
            }
        }
        tmp_map.swap ( obj_val );
        return true;
    }

    template<typename T>
    bool Serialize ( const std::vector<T>& obj_val, Json::Value& json_val )
    {
        Json::Value tmp_val ( Json::arrayValue ); // initialize tmp_val as arrayValue

        typename std::vector<T>::const_iterator it;
        for ( it = obj_val.begin ( ); it != obj_val.end ( ); ++it )
        {
            Json::Value temp;
            if ( !Serialize ( *it, temp ) )
            {
                return false;
            }
            tmp_val.append ( temp );
        }
        json_val = tmp_val;
        return true;
    }

    template<typename T>
    bool Deserialize ( const Json::Value& json_val, std::vector<T>& obj_val )
    {
        std::vector<T> tmp_val;
        tmp_val.reserve ( json_val.size ( ) );
        int size = json_val.size ( );
        if ( size > 0 )
        {

            int count = 0;
            Json::Value::iterator it = json_val.begin ( );
            while ( true )
            {
                T temp;
                if ( !Deserialize ( *it, temp ) )
                {
                    return false;
                }
                tmp_val.push_back ( temp );
                ++it;
                ++count;
                if ( count > size - 1 )
                {
                    break;
                }
            }
        }
        tmp_val.swap ( obj_val );
        return true;
    }

} // json_lib

namespace json_lib
{

    template<typename T>
    bool Serialize ( const T& field, const std::string& field_name, Json::Value& json_val )
    {
        Json::Value tmp_json_val;
        if ( !Serialize ( field, tmp_json_val ) )
        {
            return false;
        }
        json_val[field_name] = tmp_json_val;
        return true;
    }

    template<typename T>
    bool Deserialize ( const Json::Value& json_val, const std::string& field_name, T& field, bool& field_in_json )
    {
        field_in_json = false;
        if ( !json_val.isObject ( ) )
        {
            return false;
        }
        if ( json_val.isMember ( field_name.data ( ) ) )
        {
            field_in_json = true;
            const Json::Value &tmp_json_val = json_val[field_name];
            if ( !Deserialize ( tmp_json_val, field ) )
            {
                return false;
            }
        }
        return true;
    }

} // json_lib

namespace json_lib
{

    template<typename T>
    bool Serialize ( const T& obj_val, std::string& json_val )
    {
        Json::Value value;
        if ( !Serialize ( obj_val, value ) )
        {
            return false;
        }
        Json::StyledWriter writer;
        json_val = writer.write ( value );
        return true;
    }

    template<typename T>
    bool Deserialize ( const std::string& json_val, T& obj_val )
    {
        Json::Value value;
        Json::Reader reader;
        if ( !reader.parse ( json_val, value ) )
        {
            return false;
        }
        return Deserialize ( value, obj_val );
    }

} // json_lib

namespace json_lib
{

    template<typename T>
    bool Save ( const T& obj_val, const char * path )
    {
        std::string json_val;
        if ( !Serialize ( obj_val, json_val ) )
        {
            return false;
        }
        std::ofstream os;
        try
        {
            os.open ( path, std::ios::binary );
        }
        catch ( ... )
        {
            return false;
        }
        os << json_val;
        os.close ( );
        return true;
    }

    template<typename T>
    bool Load ( const char * path, T& obj_val )
    {
        Json::Reader reader;
        Json::Value json_val;
        std::ifstream is;
        try
        {
            is.open ( path, std::ios::binary );
        }
        catch ( ... )
        {
            return false;
        }
        bool ret = reader.parse ( is, json_val );
        is.close ( );
        if ( !ret )
        {
            return false;
        }
        return Deserialize ( json_val, obj_val );
    }
};

#endif // __json_serialization_20150528111839_h__
