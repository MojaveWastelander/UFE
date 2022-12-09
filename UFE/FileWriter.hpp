#pragma once
#include <filesystem>
#include <vector>
#include <fstream>
#include <map>
#include <type_traits>
#include "IndexedData.hpp"
#include <any>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
//#include <gzip/compress.hpp>
//#include <gzip/config.hpp>
//#include <gzip/decompress.hpp>
//#include <gzip/utils.hpp>
//#include <gzip/version.hpp>
//#include <zlib.h>

#include "Records.hpp"
#include "BinaryFileParser.hpp"

namespace fs = std::filesystem;



class FileWriter
{
public:
    bool update_file(fs::path& binary_file, fs::path& json_file);
private:
    bool read_record(nlohmann::ordered_json& obj);
    void get_raw_data();
    bool read(ufe::MemberTypeInfo& mti);
    bool read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj);
    bool read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj);
    bool read(ufe::ClassTypeInfo& cti);
    bool read(ufe::BinaryObjectString& bos);

    void update_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, nlohmann::ordered_json& obj);
    nlohmann::ordered_json& find_class_members(std::string name, nlohmann::ordered_json& obj, int ref_id = 0);
    template <typename T>
    void process_member(nlohmann::ordered_json& obj, const std::string& name)
    {
        IndexedData<T> tmp;
        m_parser.read(tmp);
        try
        {
            if (!obj.contains(name))
            {
                spdlog::error("Member '{}' doesn't exists in current context:  {}", name, obj.dump());
            }
            else
            {
                T jtmp = obj[name].get<T>();
                memcpy(reinterpret_cast<void*>(&m_raw_data[tmp.m_offset]), reinterpret_cast<void*>(&jtmp), sizeof(T));
            }
            //m_reader.write(reinterpret_cast<const char*>(&jtmp), sizeof(jtmp), tmp.m_offset);
            //m_reader.seek(tmp.m_offset);
            //m_reader.read(tmp);
        }
        catch (std::exception& e)
        {
            spdlog::error("Member '{}' update error: {}", name, e.what());
            //std::cerr << e.what() << '\n' << obj.dump() << '\n' << obj[name] << '\n';
        }
    }
    std::fstream m_file;
    fs::path m_file_path;
    BinaryFileParser m_parser;
    std::vector <IndexedData<ufe::LengthPrefixedString>> m_updated_strings;
    std::vector<char> m_raw_data;
    nlohmann::ordered_json m_export;
};



class UFile
{
public:
private:
    fs::path m_file_path;

};

