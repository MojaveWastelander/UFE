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

class BinaryFileParser
{
public:
    enum class EFileStatus
    {
        Empty,
        PartialRead,
        FullRead,
        Invalid
    };
    enum class EFileType
    {
        Compressed,
        Uncompressed
    };
    bool open(fs::path file_path);
    EFileStatus status() const noexcept { return m_status; }
    EFileType file_type() const noexcept { return m_file_type; }

    const std::vector<std::any>& get_records() const noexcept { return m_root_records; }
    std::string header() const noexcept { return m_header; }

    void read_records();

    std::vector<char> raw_data() const;

private:
    unsigned char read()
    {
        char c;
        m_file.read(&c, 1);
        return c;
    }

    template <typename T>
    T read()
    {
        static_assert(std::is_fundamental_v<T>, "Only fundamental types allowed");
        T tmp;
        m_file.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
        return tmp;
    }

    template <typename T>
    IndexedData<T> read_primitive()
    {
        IndexedData<T> tmp;
        read(tmp);
        return tmp;
    }

    template <typename T>
    void process_member(std::vector<IndexedData<ufe::LengthPrefixedString>>::const_iterator it_member_names, ufe::MemberTypeInfo& mti)
    {
        IndexedData<T> tmp;
        read(tmp);
        spdlog::debug("\t{} = {}", it_member_names->value.string, tmp.value);
        mti.Data.push_back(std::any{ tmp });
    }
    
    template <typename T>
    bool read(IndexedData<T>& data)
    {
        static_assert(std::is_fundamental_v<T>, "Type doesn't have specialization");
        data.offset = m_file.tellg();
        m_file.read(reinterpret_cast<char*>(&data.value), sizeof(data.value));
        return true;
    }

    template<>
    bool read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& lps);
    bool read(ufe::SerializationHeaderRecord& header);
    bool read(ufe::BinaryLibrary& bl);
    bool read(ufe::ClassInfo& ci);
    bool read(ufe::MemberTypeInfo& mti);
    bool read(ufe::ClassWithMembersAndTypes& cmt, bool system_class = false);
    bool read(ufe::ClassWithId& cmt);
    bool read(ufe::LengthPrefixedString& lps);
    bool read(ufe::ClassTypeInfo& cti);
    bool read(ufe::BinaryObjectString& bos);
    bool read(ufe::ObjectNull& obj) {}
    bool read(ufe::MemberReference& ref);
    bool read(ufe::ArraySingleString& arr_str);
    bool read(ufe::ArraySinglePrimitive& arr_prim);
    bool read(ufe::BinaryArray& arr_bin);

    std::any read_primitive_element(ufe::EPrimitiveTypeEnumeration type);
    const std::any& get_record(int32_t id);
    std::any read_record();

    std::any get_ArraySingleString();

    std::any get_ArraySinglePrimitive();

    std::any get_ObjectNullMultiple256();

    std::any get_BinaryLibrary();

    std::any get_MemberReference();

    std::any get_BinaryArray();

    std::any get_BinaryObjectString();

    std::any get_ClassWithMembersAndTypes();

    std::any get_SystemClassWithMembersAndTypes();

    std::any get_ClassWithId();

    std::any get_SerializedStreamHeader();

    void add_record(int32_t id, std::any&& record)
    {
        m_records.emplace_back(std::make_pair(id, record));
    }
    ufe::ERecordType get_record_type()
    {
        return static_cast<ufe::ERecordType>(read());
    }
    void read_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci);
    bool check_header();
    std::vector<std::pair<int32_t, std::any>> m_records;
    std::vector<std::any> m_root_records;
    fs::path m_file_path;
    EFileStatus m_status = EFileStatus::Empty;
    EFileType m_file_type = EFileType::Uncompressed;
    std::string m_header;
    std::string m_raw_data;
    mutable std::istringstream m_file;
};