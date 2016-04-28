#include "../lib/json_serialization.h"

namespace json_lib
{

    /**
     * bool
     */
    bool Serialize ( const bool& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, bool& obj_val )
    {
        /**
         * Warning: the default type may be int, even you Serialize a bool value
         */
        if ( json_val.isBool () )
        {
            obj_val = json_val.asBool ();
            return true;
        }
        else if ( json_val.isInt () )
        {
            int tmp = json_val.asInt ();
            if ( ! tmp )
            {
                obj_val = false;
            }
            else
            {
                obj_val = true;
            }
            return true;
        }
        return false;
    }

    /**
     * std::string
     */
    bool Serialize ( const std::string& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, std::string& obj_val )
    {
        if ( json_val.isString () )
        {
            obj_val = json_val.asString ();
            return true;
        }
        return false;
    }

    /**
     * int
     */
    bool Serialize ( const int& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, int& obj_val )
    {
        if ( json_val.isInt () )
        {
            obj_val = json_val.asInt ();
            return true;
        }
        return false;
    }

    /**
     * unsigned int
     */
    bool Serialize ( const unsigned int& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, unsigned int& obj_val )
    {
        /**
         * Warning: the default type may be int, even you Serialize a uint value
         */
        if ( json_val.isIntegral () )
        {
            obj_val = json_val.asUInt ();
            return true;
        }
        return false;
    }

    /**
     * double
     */
    bool Serialize ( const double& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, double & obj_val )
    {
        if ( json_val.isDouble () )
        {
            obj_val = json_val.asDouble ();
            return true;
        }
        return false;
    }

    /**
     * float
     */
    bool Serialize ( const float& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, float & obj_val )
    {
        if ( json_val.isDouble () )
        {
            obj_val = json_val.asFloat ();
            return true;
        }
        return false;
    }

    /**
     * json_lib::int64 & json_lib::uint64
     */
    bool Serialize ( const json_lib::int64& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, json_lib::int64& obj_val )
    {
        if ( json_val.isIntegral () )
        {
            obj_val = json_val.asInt64 ();
            return true;
        }
        return false;
    }

    bool Serialize ( const json_lib::uint64& obj_val, Json::Value& json_val )
    {
        json_val = obj_val;
        return true;
    }

    bool Deserialize ( const Json::Value& json_val, json_lib::uint64& obj_val )
    {
        if ( json_val.isIntegral () )
        {
            obj_val = json_val.asUInt64 ();
            return true;
        }
        return false;
    }

} // json_lib

namespace json_lib
{

    /**
     * char & unsigned char
     */
    bool Serialize ( const char& obj_val, Json::Value& json_val )
    {
        int tmp_obj_val = static_cast < int > ( obj_val );
        return Serialize (tmp_obj_val, json_val);
    }

    bool Deserialize ( const Json::Value& json_val, char& obj_val )
    {
        int tmp_obj_val = 0;
        if ( ! Deserialize (json_val, tmp_obj_val) )
        {
            return false;
        }
        obj_val = static_cast < char > ( tmp_obj_val );
        return true;
    }

    bool Serialize ( const unsigned char& obj_val, Json::Value& json_val )
    {
        unsigned int tmp_obj_val = static_cast < unsigned int > ( obj_val );
        return Serialize (tmp_obj_val, json_val);
    }

    bool Deserialize ( const Json::Value& json_val, unsigned char& obj_val )
    {
        unsigned int tmp_obj_val = 0;
        if ( ! Deserialize (json_val, tmp_obj_val) )
        {
            return false;
        }
        obj_val = static_cast < unsigned char > ( tmp_obj_val );
        return true;
    }

    /**
     * short & unsigned short
     */
    bool Serialize ( const short& obj_val, Json::Value& json_val )
    {
        int tmp_obj_val = static_cast < int > ( obj_val );
        return Serialize (tmp_obj_val, json_val);
    }

    bool Deserialize ( const Json::Value& json_val, short& obj_val )
    {
        int tmp_obj_val = 0;
        if ( ! Deserialize (json_val, tmp_obj_val) )
        {
            return false;
        }
        obj_val = static_cast < short > ( tmp_obj_val );
        return true;
    }

    bool Serialize ( const unsigned short& obj_val, Json::Value& json_val )
    {
        unsigned int tmp_obj_val = static_cast < unsigned int > ( obj_val );
        return Serialize (tmp_obj_val, json_val);
    }

    bool Deserialize ( const Json::Value& json_val, unsigned short& obj_val )
    {
        unsigned int tmp_obj_val = 0;
        if ( ! Deserialize (json_val, tmp_obj_val) )
        {
            return false;
        }
        obj_val = static_cast < unsigned short > ( tmp_obj_val );
        return true;
    }

} // json_lib
