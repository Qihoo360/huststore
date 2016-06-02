#include "json_serialization.h"

namespace jos_lib
{
    // bool
    bool Serialize(const bool& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetBool(obj_val);
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, bool& obj_val)
    {
        if (json_val.IsBool())
        {
            obj_val = json_val.GetBool();
            return true;
        }
        else if (json_val.IsInt())
        {
            int tmp = json_val.GetInt();
            if (!tmp)
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
    // std::string
    bool Serialize(const std::string& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetString(rapidjson::StringRef(obj_val.c_str()));
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, std::string& obj_val)
    {
        if (json_val.IsString())
        {
            obj_val = json_val.GetString();
            return true;
        }
        return false;
    }
    // int
    bool Serialize(const int& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetInt(obj_val);
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, int& obj_val)
    {
        if (json_val.IsInt())
        {
            obj_val = json_val.GetInt();
            return true;
        }
        return false;
    }
    // unsigned int
    bool Serialize(const unsigned int& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetUint(obj_val);
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, unsigned int& obj_val)
    {
        if (json_val.IsInt())
        {
            obj_val = json_val.GetInt();
            return true;
        }
        else if (json_val.IsUint())
        {
            obj_val = json_val.GetUint();
            return true;
        }
        return false;
    }
    // double
    bool Serialize(const double& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetDouble(obj_val);
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, double & obj_val)
    {
        if (json_val.IsDouble())
        {
            obj_val = json_val.GetDouble();
            return true;
        }
        return false;
    }

    // jos_lib::int64 & jos_lib::uint64
    bool Serialize(const jos_lib::int64& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetInt64(obj_val);
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, jos_lib::int64& obj_val)
    {
        if (json_val.IsInt64())
        {
            obj_val = json_val.GetInt64();
            return true;
        }
        else if (json_val.IsInt())
        {
            obj_val = json_val.GetInt();
            return true;
        }
        else if (json_val.IsUint())
        {
            obj_val = json_val.GetUint();
            return true;
        }
        return false;
    }
    bool Serialize(const jos_lib::uint64& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        json_val.SetUint64(obj_val);
        return true;
    }
    bool Deserialize(const rapidjson::Value& json_val, jos_lib::uint64& obj_val)
    {
        if (json_val.IsUint64())
        {
            obj_val = json_val.GetUint64();
            return true;
        }
        else if (json_val.IsInt())
        {
            obj_val = json_val.GetInt();
            return true;
        }
        else if (json_val.IsUint())
        {
            obj_val = json_val.GetUint();
            return true;
        }
        return false;
    }
}

namespace jos_lib
{
    // char & unsigned char
    bool Serialize(const char& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        int tmp_obj_val = static_cast<int>(obj_val);
        return Serialize(tmp_obj_val, alloc, json_val);
    }
    bool Deserialize(const rapidjson::Value& json_val, char& obj_val)
    {
        int tmp_obj_val = 0;
        if (!Deserialize(json_val, tmp_obj_val))
        {
            return false;
        }
        obj_val = static_cast<char>(tmp_obj_val);
        return true;
    }

    bool Serialize(const unsigned char& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        unsigned int tmp_obj_val = static_cast<unsigned int>(obj_val);
        return Serialize(tmp_obj_val, alloc, json_val);
    }
    bool Deserialize(const rapidjson::Value& json_val, unsigned char& obj_val)
    {
        unsigned int tmp_obj_val = 0;
        if (!Deserialize(json_val, tmp_obj_val))
        {
            return false;
        }
        obj_val = static_cast<unsigned char>(tmp_obj_val);
        return true;
    }

    // short & unsigned short
    bool Serialize(const short& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        int tmp_obj_val = static_cast<int>(obj_val);
        return Serialize(tmp_obj_val, alloc, json_val);
    }
    bool Deserialize(const rapidjson::Value& json_val, short& obj_val)
    {
        int tmp_obj_val = 0;
        if (!Deserialize(json_val, tmp_obj_val))
        {
            return false;
        }
        obj_val = static_cast<short>(tmp_obj_val);
        return true;
    }

    bool Serialize(const unsigned short& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        unsigned int tmp_obj_val = static_cast<unsigned int>(obj_val);
        return Serialize(tmp_obj_val, alloc, json_val);
    }
    bool Deserialize(const rapidjson::Value& json_val, unsigned short& obj_val)
    {
        unsigned int tmp_obj_val = 0;
        if (!Deserialize(json_val, tmp_obj_val))
        {
            return false;
        }
        obj_val = static_cast<unsigned short>(tmp_obj_val);
        return true;
    }

    // float
    bool Serialize(const float& obj_val, jos_lib::Allocator& alloc, rapidjson::Value& json_val)
    {
        double tmp_obj_val = obj_val;
        return Serialize(tmp_obj_val, alloc, json_val);
    }
    bool Deserialize(const rapidjson::Value& json_val, float & obj_val)
    {
        double tmp_obj_val = 0;
        if (!Deserialize(json_val, tmp_obj_val))
        {
            return false;
        }
        obj_val = static_cast<float>(tmp_obj_val);
        return true;
    }

} // jos_lib

namespace jos_lib
{
    template <typename Src, typename Dst>
    bool Convert(const Src& src, Dst& dst)
    {
        if (src.IsBool())
        {
            dst.SetBool(src.GetBool());
        }
        else if (src.IsInt())
        {
            dst.SetInt(src.GetInt());
        }
        else if (src.IsUint())
        {
            dst.SetUint(src.GetUint());
        }
        else if (src.IsInt64())
        {
            dst.SetInt64(src.GetInt64());
        }
        else if (src.IsUint64())
        {
            dst.SetUint64(src.GetUint64());
        }
        else if (src.IsDouble())
        {
            dst.SetDouble(src.GetDouble());
        }
        else if (src.IsString())
        {
            dst.SetString(rapidjson::StringRef(src.GetString()));
        }
        else if (src.IsNull())
        {
            dst.SetNull();
        }
        else
        {
            return false;
        }
        return true;
    }

    bool Encode(rapidjson::Value& src, rapidjson::Document& dst)
    {
        rapidjson::Document::AllocatorType& allocator = dst.GetAllocator();
        if (src.IsArray())
        {
            dst.SetArray();
            rapidjson::SizeType size = src.Size();
            for (rapidjson::SizeType i = 0; i < size; ++i)
            {
                dst.PushBack(src[i].Move(), allocator);
            }
            return true;
        }
        else if (src.IsObject())
        {
            dst.SetObject();
            for (rapidjson::Value::MemberIterator it = src.MemberBegin(); it != src.MemberEnd(); ++it)
            {
                dst.AddMember(rapidjson::StringRef(it->name.GetString()), it->value.Move(), allocator);
            }
            return true;
        }
        return Convert(src, dst);
    }
    bool Decode(rapidjson::Document& src, rapidjson::Value& dst)
    {
        rapidjson::Document::AllocatorType& allocator = src.GetAllocator();
        if (src.IsArray())
        {
            dst.SetArray();
            rapidjson::SizeType size = src.Size();
            for (rapidjson::SizeType i = 0; i < size; ++i)
            {
                dst.PushBack(src[i].Move(), allocator);
            }
            return true;
        }
        else if (src.IsObject())
        {
            dst.SetObject();
            for (rapidjson::Document::MemberIterator it = src.MemberBegin(); it != src.MemberEnd(); ++it)
            {
                dst.AddMember(rapidjson::StringRef(it->name.GetString()), it->value.Move(), allocator);
            }
            return true;
        }
        return Convert(src, dst);
    }

    char * LoadFromFile(const char * path)
    {
        if (!path)
        {
            return NULL;
        }
        FILE * f = fopen(path, "rb");
        if (!f)
        {
            return NULL;
        }
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char * data = (char *)malloc(len + 1);
        fread(data, 1, len, f);
        data[len] = '\0';
        fclose(f);
        return data;
    }

    JsonLoader::JsonLoader(const char * path)
    {
        data_ = LoadFromFile(path);
    }
    JsonLoader::~JsonLoader()
    {
        if (data_)
        {
            free(data_);
            data_ = NULL;
        }
    }
    const char * JsonLoader::c_str()
    {
        return data_;
    }

}
