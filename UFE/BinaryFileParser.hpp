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
#include "Records.hpp"

namespace fs = std::filesystem;

struct AnyJson 
{
    std::any dyn_obj;
    nlohmann::ordered_json json_obj;
};

class BinaryFileParser
{
public:
    bool open(fs::path file_path);
    void close() { return m_file.close(); }
    size_t offset() { return m_file.tellg(); }

    const std::vector<std::pair<int32_t, std::any>>& get_records() const noexcept { return m_records; }

    bool export_json(fs::path json_path);

    const std::any& get_record(int32_t id);

    void read_records();

    AnyJson read_record();

    void read_primitive_element(ufe::EPrimitiveTypeEnumeration type, nlohmann::ordered_json& obj);

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
    bool read(ufe::ArraySingleString& arr_str);
    bool read(ufe::ArraySinglePrimitive& arr_prim);
    bool read(ufe::BinaryArray& arr_bin);

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
    void read_primitive(nlohmann::ordered_json& obj)
    {
        T tmp;
        tmp = read<T>();
        obj.push_back(tmp);
    }

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
    nlohmann::ordered_json m_json;
};