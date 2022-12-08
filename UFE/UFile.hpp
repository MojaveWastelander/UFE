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

namespace fs = std::filesystem;

class FileParser
{
public:
    bool open(fs::path file_path);
    void close() { return m_file.close(); }
    void write(const char* data, std::streamsize size, int offset);
    void seek(int offset)
    {
        m_file.seekg(offset);
    }
    bool export_json(fs::path json_path);

    const std::any& get_record(int32_t id);

    void read_records();

    std::any read_record(nlohmann::ordered_json& obj);

    void add_record(int32_t id, std::any&& record)
    {
        m_records.emplace_back(std::make_pair(id, record));
    }

    ufe::ERecordType get_record_type()
    {
        return static_cast<ufe::ERecordType>(read());
    }

    unsigned char read()
    {
        char c;
        m_file.read(&c, 1);
        return c;
    }

    template <typename T>
    bool read(IndexedData<T>& data)
    {
        static_assert(std::is_fundamental_v<T>, "Type doesn't have specialization");
        data.m_offset = m_file.tellg();
        m_file.read(reinterpret_cast<char*>(&data.m_data), sizeof(data.m_data));
        return true;
    }

    template<>
    bool read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& lps);

    bool read(ufe::SerializationHeaderRecord& header);
    bool read(ufe::BinaryLibrary& bl);
    bool read(ufe::ClassInfo& ci);
    bool read(ufe::MemberTypeInfo& mti);
    bool read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj, bool system_class = false);
    bool read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj);
    bool read(ufe::LengthPrefixedString& lps);
    bool read(ufe::ClassTypeInfo& cti);
    bool read(ufe::BinaryObjectString& bos);
    bool read(ufe::ObjectNull& obj) {}
    bool read(ufe::MemberReference& ref);

    template <typename T>
    T read()
    {
        static_assert(std::is_fundamental_v<T>, "Only fundamental types allowed");
        T tmp;
        m_file.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
        return tmp;
    }

    void read_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, nlohmann::ordered_json& obj);

    template <typename T>
    void process_member(nlohmann::ordered_json& obj, std::vector<IndexedData<ufe::LengthPrefixedString>>::const_iterator it_member_names, ufe::MemberTypeInfo& mti)
    {
        T tmp = read<T>();
        obj[it_member_names->m_data.m_str] = tmp;
        spdlog::debug("\t{} = {}", it_member_names->m_data.m_str, tmp);
        mti.Data.push_back(std::any{ tmp });
    }
    std::vector<char> raw_data();

private:
    std::vector<std::pair<int32_t, std::any>> m_records;
    std::fstream m_file;
    fs::path m_file_path;
    nlohmann::ordered_json m_export;
};

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
        m_reader.read(tmp);
        try
        {
            T jtmp = obj[name].get<T>();
            spdlog::debug("{} => {}", name, obj.dump());
            if (obj.contains(name))
            {
                spdlog::debug("{} => {}", tmp.m_data, obj[name].dump());
            }
            //m_reader.write(reinterpret_cast<const char*>(&jtmp), sizeof(jtmp), tmp.m_offset);
            //m_reader.seek(tmp.m_offset);
            //m_reader.read(tmp);
            memcpy(reinterpret_cast<void*>(&m_raw_data[tmp.m_offset]), reinterpret_cast<void*>(&jtmp), sizeof(T));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << '\n' << obj.dump() << '\n' << obj[name] << '\n';
        }
    }
    std::fstream m_file;
    fs::path m_file_path;
    FileParser m_reader;
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

