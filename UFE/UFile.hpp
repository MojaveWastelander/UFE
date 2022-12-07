#pragma once
#include <filesystem>
#include <vector>
#include <fstream>
#include <map>
#include <type_traits>
#include "IndexedData.hpp"
#include <any>
#include <nlohmann/json.hpp>
//#include <gzip/compress.hpp>
//#include <gzip/config.hpp>
//#include <gzip/decompress.hpp>
//#include <gzip/utils.hpp>
//#include <gzip/version.hpp>
//#include <zlib.h>

#include "Records.hpp"

namespace fs = std::filesystem;

class FileWriter
{
    bool update_file(fs::path& binary_file, fs::path& json_file);
private:
    std::ifstream m_file;
    fs::path m_file_path;
    nlohmann::ordered_json m_export;
};

class FileReader
{
public:
    bool open(fs::path file_path)
    {
        if (fs::exists(file_path) && fs::is_regular_file(file_path))
        {
            m_file.open(file_path, std::ios::binary);

            // init json
            m_export = 
            {
                {
                    "records", {}
                }
            };

            return true;
        }
        return false;
    }

    bool export_json()
    {
        std::ofstream out{ "__psibeetlecarapace.json" };
        out << std::setw(4) << m_export;
        return true;
    }

    const std::any& get_record(int32_t id)
    {
        static std::any dummy;
        for (auto& rec : m_records)
        {
            if (rec.first == id)
            {
                return rec.second;
            }
        }
        return dummy;
    }

    void read_records()
    {
        for (std::any a = read_record(m_export["records"]); a.has_value(); a = read_record(m_export["records"]))
        {
        }
    }

    std::any read_record(nlohmann::ordered_json& obj)
    {
        ufe::ERecordType rec = get_record_type();
        ufe::log_line(std::format("Parsing record: {}", ufe::ERecordType2str(rec)));
        switch (rec)
        {
            case ufe::ERecordType::SerializedStreamHeader:
            {
                ufe::SerializationHeaderRecord rec;
                read(rec);
                return std::any{std::move(rec)};
            } break;
            case ufe::ERecordType::ClassWithId:
            {
                ufe::ClassWithId cwi;
                read(cwi, obj);
                return std::any{ std::move(cwi) };
            } break;
            case ufe::ERecordType::SystemClassWithMembers:
                break;
            case ufe::ERecordType::ClassWithMembers:
                break;
            case ufe::ERecordType::SystemClassWithMembersAndTypes:
                break;
            case ufe::ERecordType::ClassWithMembersAndTypes:
            {
                ufe::ClassWithMembersAndTypes cmt;
                read(cmt, obj);
                add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
                return std::any{std::move(cmt)};
            } break;
            case ufe::ERecordType::BinaryObjectString:
            {
                ufe::BinaryObjectString bos;
                read(bos);
                add_record(bos.m_ObjectId, std::any{ bos });
                return std::any{ std::move(bos) };
            } break;
            case ufe::ERecordType::BinaryArray:
                break;
            case ufe::ERecordType::MemberPrimitiveTyped:
                break;
            case ufe::ERecordType::MemberReference:
            {
                ufe::MemberReference ref;
                read(ref);
                obj["reference"] = ref.m_idRef;
                return std::any{ std::move(ref) };
            } break;
            case ufe::ERecordType::ObjectNull:
            {
                obj = {};
                return std::any{ ufe::ObjectNull{} };
            } break;
            case ufe::ERecordType::MessageEnd:
                break;
            case ufe::ERecordType::BinaryLibrary:
            {
                ufe::BinaryLibrary bl;
                read(bl);
                add_record(bl.LibraryId.m_data, std::any{ bl });
                return std::any{ std::move(bl) };
            } break;
            case ufe::ERecordType::ObjectNullMultiple256:
                break;
            case ufe::ERecordType::ObjectNullMultiple:
                break;
            case ufe::ERecordType::ArraySinglePrimitive:
                break;
            case ufe::ERecordType::ArraySingleObject:
                break;
            case ufe::ERecordType::ArraySingleString:
                break;
            case ufe::ERecordType::MethodCall:
                break;
            case ufe::ERecordType::MethodReturn:
                break;
            case ufe::ERecordType::InvalidType:
                break;
            default:
                break;
        }
        return std::any{};
    }

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
        auto cnt = m_file.readsome(reinterpret_cast<char*>(&data.m_data), sizeof(data.m_data));
        return cnt == sizeof(data.m_data);
    }

    template<>
    bool read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& lps);

    bool read(ufe::SerializationHeaderRecord& header)
    {
        read(header.RootId);
        read(header.HeaderId);
        read(header.MajorVersion);
        read(header.MinorVersion);
        return true;
    }
    bool read(ufe::BinaryLibrary& bl);
    bool read(ufe::ClassInfo& ci);
    bool read(ufe::MemberTypeInfo& mti);
    bool read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj);
    bool read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj);
    bool read(ufe::LengthPrefixedString& lps);
    bool read(ufe::ClassTypeInfo& cti);
    bool read(ufe::BinaryObjectString& bos);
    bool read(ufe::ObjectNull& obj) {}
    bool read(ufe::MemberReference& ref)
    {
        ref.m_idRef = read<int32_t>();
        return true;
    }

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
        mti.Data.push_back(std::any{ tmp });
    }


private:
    std::vector<std::pair<int32_t, std::any>> m_records;
    std::ifstream m_file;
    fs::path m_file_path;
    nlohmann::ordered_json m_export;
};


class UFile
{
public:
private:
    fs::path m_file_path;

};

