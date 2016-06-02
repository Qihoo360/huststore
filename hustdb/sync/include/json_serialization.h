#ifndef __json_serialization_20160115144052_h__
#define __json_serialization_20160115144052_h__

#include <typeinfo>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <rapidjson.h>
#include <document.h>
#include <stringbuffer.h>
#include <writer.h>
#include <prettywriter.h>

namespace jos_lib
{
    typedef int64_t int64;
    typedef uint64_t uint64;
    typedef rapidjson::Document::AllocatorType Allocator;
}

namespace jos_lib
{
    bool Serialize(const bool& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, bool& obj_val);

    bool Serialize(const std::string& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, std::string& obj_val);

    bool Serialize(const int& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, int& obj_val);

    bool Serialize(const unsigned int& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, unsigned int& obj_val);

    bool Serialize(const double& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, double & obj_val);

    bool Serialize(const jos_lib::int64& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, jos_lib::int64& obj_val);

    bool Serialize(const jos_lib::uint64& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, jos_lib::uint64& obj_val);
}

namespace jos_lib
{
    bool Serialize(const char& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, char& obj_val);

    bool Serialize(const unsigned char& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, unsigned char& obj_val);

    bool Serialize(const short& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, short& obj_val);

    bool Serialize(const unsigned short& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, unsigned short& obj_val);

    bool Serialize(const float& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    bool Deserialize(const rapidjson::Value& json_val, float & obj_val);

    // special for enum type
    template <typename T>
    bool Serialize(T obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    template <typename T>
    bool Deserialize(const rapidjson::Value& json_val, T& obj_val);
} // jos_lib

namespace jos_lib
{
    template<typename T>
    bool Serialize(const std::vector<T>& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    template<typename T>
    bool Deserialize(const rapidjson::Value& json_val, std::vector<T>& obj_val);

    template<typename T>
    bool Serialize(const std::map<std::string, T>& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    template<typename T>
    bool Deserialize(const rapidjson::Value& json_val, std::map<std::string, T>& obj_val);

    template<typename Number, typename T>
    bool Serialize(const std::map<Number, T>& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    template<typename Number, typename T>
    bool Deserialize(const rapidjson::Value& json_val, std::map<Number, T>& obj_val);
} // jos_lib

namespace jos_lib
{
    template<typename T>
    bool Serialize(const T& field, const std::string& field_name, jos_lib::Allocator& alloc, rapidjson::Value& json_val);
    template<typename T>
    bool Deserialize(const rapidjson::Value& json_val, const std::string& field_name, T& field, bool& field_in_json);
} // jos_lib

namespace jos_lib
{
    template<typename T, bool pretty>
    bool Serialize(const T& obj_val, std::string& json_val);
    template<typename T>
    bool Deserialize(const std::string& json_val, T& obj_val);
} // jos_lib

namespace jos_lib
{
    template<typename T, bool pretty>
    bool Save(const T& obj_val, const char * path);

    template<typename T>
    bool Load(const char * path, T& obj_val);
};

// implement
namespace jos_lib
{
    template <typename T>
    std::string GetName(const T& obj)
    {
        try
        {
            return typeid(obj).name();
        }
        catch (...)
        {
            return "";
        }
    }

    template <typename T>
    bool Serialize(T obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        const std::string& obj_type = jos_lib::GetName(obj_val);
        if (obj_type.find("enum") < 0)
        {
            return false;
        };
        int tmp_obj_val = static_cast<int>(obj_val);
        return Serialize(tmp_obj_val, alloc, json_val);
    }
    template <typename T>
    bool Deserialize(const rapidjson::Value& json_val, T& obj_val)
    {
        const std::string& obj_type = jos_lib::GetName(obj_val);
        if (obj_type.find("enum") < 0)
        {
            return false;
        };
        int tmp_obj_val = 0;
        if (!Deserialize(json_val, tmp_obj_val))
        {
            return false;
        }
        T _obj_val = static_cast<T>(tmp_obj_val);
        obj_val = _obj_val;
        return true;
    }
}

namespace jos_lib
{
    template<typename T>
    bool Serialize(const std::map<std::string, T>& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetObject();
        typename std::map<std::string, T>::const_iterator it;
        for (it = obj_val.begin(); it != obj_val.end(); ++it)
        {
            rapidjson::Value val;
            if (!Serialize(it->second, alloc, val))
            {
                return false;
            }
            json_val.AddMember(rapidjson::StringRef(it->first.c_str()), val.Move(), alloc);
        }
        return true;
    }
    template<typename T>
    bool Deserialize(const rapidjson::Value& json_val, std::map<std::string, T>& obj_val)
    {
        if (!json_val.IsObject())
        {
            return false;
        }
        for (rapidjson::Value::ConstMemberIterator it = json_val.MemberBegin(); it != json_val.MemberEnd(); ++it)
        {
            T val;
            if (!Deserialize(it->value, val))
            {
                return false;
            }
            obj_val[it->name.GetString()] = val;
        }
        return true;
    }

    template<typename Number, typename T>
    bool Serialize(const std::map<Number, T>& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetObject();
        typename std::map<Number, T>::const_iterator it;
        for (it = obj_val.begin(); it != obj_val.end(); ++it)
        {
            rapidjson::Value val;
            if (!Serialize(it->second, alloc, val))
            {
                return false;
            }
            // number to string
            std::stringstream ss;
            ss << it->first;
            const std::string & tmp = ss.str();
            rapidjson::Value key(tmp.c_str(), alloc);
            json_val.AddMember(key, val.Move(), alloc);
        }
        return true;
    }
    template<typename Number, typename T>
    bool Deserialize(const rapidjson::Value& json_val, std::map<Number, T>& obj_val)
    {
        if (!json_val.IsObject())
        {
            return false;
        }
        for (rapidjson::Value::ConstMemberIterator it = json_val.MemberBegin(); it != json_val.MemberEnd(); ++it)
        {
            T val;
            if (!Deserialize(it->value, val))
            {
                return false;
            }
            // string to number
            std::stringstream ss(it->name.GetString());
            Number key;
            ss >> key;
            obj_val[key] = val;
        }
        return true;
    }

    template<typename T>
    bool Serialize(const std::vector<T>& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetArray();
        typename std::vector<T>::const_iterator it;
        for (it = obj_val.begin(); it != obj_val.end(); ++it)
        {
            rapidjson::Value temp;
            if (!Serialize(*it, alloc, temp))
            {
                return false;
            }
            json_val.PushBack(temp.Move(), alloc);
        }
        return true;
    }
    template<typename T>
    bool Deserialize(const rapidjson::Value& json_val, std::vector<T>& obj_val)
    {
        if (!json_val.IsArray())
        {
            return false;
        }
        rapidjson::SizeType size = json_val.Size();
        obj_val.reserve(size);

        for (rapidjson::SizeType i = 0; i < size; ++i)
        {
            T tmp_val;
            if (!Deserialize(json_val[i], tmp_val))
            {
                return false;
            }
            obj_val.push_back(tmp_val);
        }
        return true;
    }

} // jos_lib

namespace jos_lib
{
    template<typename T>
    bool Serialize(const T& field, const char * field_name, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        if (!field_name)
        {
            return false;
        }
        rapidjson::Value tmp_json_val;
        if (!Serialize(field, alloc, tmp_json_val))
        {
            return false;
        }
        rapidjson::Value key(field_name, alloc);
        json_val.AddMember(key, tmp_json_val.Move(), alloc);
        return true;
    }

    template<typename T>
    bool Deserialize(const rapidjson::Value& json_val, const char * field_name, T& field, bool& field_in_json)
    {
        field_in_json = false;
        if (!json_val.IsObject() || !field_name)
        {
            return false;
        }
        if (json_val.HasMember(field_name))
        {
            field_in_json = true;
            const rapidjson::Value &tmp_json_val = json_val[field_name];
            if (!Deserialize(tmp_json_val, field))
            {
                return false;
            }
        }
        return true;
    }

} // jos_lib

namespace jos_lib
{
    template <bool pretty> struct WriterSelector;

    template <>
    struct WriterSelector <true>
    {
        typedef rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer;
    };

    template <>
    struct WriterSelector <false>
    {
        typedef rapidjson::Writer<rapidjson::StringBuffer> Writer;
    };

    template<bool pretty>
    bool Serialize(const rapidjson::Document& doc, std::string& json_val)
    {
        rapidjson::StringBuffer buffer;
        typename WriterSelector<pretty>::Writer writer(buffer);
        doc.Accept(writer);
        json_val = buffer.GetString();
        return true;
    }

    bool Encode(rapidjson::Value& dst, rapidjson::Document& src);
    bool Decode(rapidjson::Document& src, rapidjson::Value& dst);

    template<typename T, bool pretty>
    bool Serialize(const T& obj_val, std::string& json_val)
    {
        rapidjson::Document doc;
        rapidjson::Value value;
        if (!Serialize(obj_val, doc.GetAllocator(), value))
        {
            return false;
        }
        if (!Encode(value, doc))
        {
            return false;
        }
        return Serialize <pretty> (doc, json_val);
    }
    template<typename T>
    bool Deserialize(const char * json_val, T& obj_val)
    {
        rapidjson::Document doc;
        doc.Parse(json_val);
        rapidjson::Value value;
        if (!jos_lib::Decode(doc, value))
        {
            return false;
        }
        return Deserialize(value, obj_val);
    }
    template<typename T>
    bool Deserialize(const std::string& json_val, T& obj_val)
    {
        return Deserialize(json_val.c_str(), obj_val);
    }

} // jos_lib

namespace jos_lib
{
    template<typename T, bool pretty>
    bool Save(const T& obj_val, const char * path)
    {
        std::string json_val;
        if (!Serialize <T, pretty> (obj_val, json_val))
        {
            return false;
        }
        std::ofstream os;
        try
        {
            os.open(path, std::ios::binary);
        }
        catch (...)
        {
            return false;
        }
        os << json_val;
        os.close();
        return true;
    }

    struct JsonLoader
    {
        JsonLoader(const char * path);
        ~JsonLoader();
        const char * c_str();
    private:
        char * data_;
    };

    template<typename T>
    bool Load(const char * path, T& obj_val)
    {
        JsonLoader loader(path);
        if (!loader.c_str())
        {
            return false;
        }
        return Deserialize(loader.c_str(), obj_val);
    }
};

#endif // __json_serialization_20160115144052_h__
