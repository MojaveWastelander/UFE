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
#include "BinaryFileParser.hpp"

class JsonReader
{
public:
    JsonReader();
    bool patch(std::filesystem::path json_path, std::filesystem::path binary_path, const BinaryFileParser& parser);
private:

    template<class T, class F>
    inline void register_any_visitor(F const& f)
    {
        //std::cout << "Register visitor for type "
        //    << std::quoted(typeid(T).name()) << '\n';
        //m_any_visitor.insert(std::make_pair(
        //    std::type_index(typeid(T)),
        //    [this, &f](std::any const& a)
        //    {
        //        // return g(std::any_cast<T const&>(a));
        //        return std::bind(f, this, std::any_cast<T const&>(a));
        //    }
        //));
        m_any_visitor[std::type_index(typeid(T))] = [this, f](std::any const& a, const ojson& context) -> void {
            if (std::is_fundamental_v<T>)
            {
                std::bind(f, this, std::any_cast<T>(a), std::cref(context))();
            }
            else
            {
                std::bind(f, this, std::cref(std::any_cast<T const&>(a)), std::cref(context))();
            }
        };
    }
    bool json_elem(const nlohmann::ordered_json& json, const std::string& name)
    {
        if (json.contains(name))
        {
            return true;
        }
        spdlog::warn("current json object doesn't contain element {}", name);
        return false;
    }

    template <typename T>
    void process_member(const IndexedData<T>& data, const ojson& ctx)
    {
        try
        {
            T jtmp = ctx.get<T>();
            if (data.m_data != jtmp)
            {
                spdlog::warn("\t'{}' ==> '{}'", data.m_data, jtmp);
            }
            else
            {
                spdlog::trace("\t'{}' ==> '{}'", data.m_data, jtmp);
            }
            memcpy(reinterpret_cast<void*>(&m_raw_data[data.m_offset]), reinterpret_cast<void*>(&jtmp), sizeof(T));
        }
        catch (std::exception& e)
        {
            spdlog::error("Member update error: {}", e.what());
        }
    }

    bool process_records(const std::vector<std::any>& records);


    const ojson& find_class_by_id(const ojson& ctx, const ufe::ClassInfo& ci, std::string class_type);
    void process_class_members(nlohmann::ordered_json& members, const  ufe::ClassInfo& ci, const  ufe::MemberTypeInfo& mti);

    void member_reference(const ufe::MemberReference& mref, const ojson& ctx) { /* do nothing */ }
    void binary_object_string(const ufe::BinaryObjectString& bos, const ojson& ctx);
    void class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt, const ojson& ctx);
    void class_with_id(const ufe::ClassWithId& cwi, const ojson& ctx);
    //ojson array_single_string(const ufe::ArraySingleString& arr);
    //ojson array_binary(const ufe::BinaryArray& arr);
    void value_bool(const IndexedData<bool>& x, const ojson& context) { process_member<bool>(x, context); }
    void value_char(const IndexedData<char>& x, const ojson& context) { process_member<char>(x, context); }
    void value_uchar(const IndexedData<unsigned char>& x, const ojson& context) { process_member<unsigned char>(x, context); }
    void value_int16(const IndexedData<int16_t>& x, const ojson& context) { process_member<int16_t>(x, context); }
    void value_uint16(const IndexedData<uint16_t>& x, const ojson& context) { process_member<uint16_t>(x, context); }
    void value_int32(const IndexedData<int32_t>& x, const ojson& context) { process_member<int32_t>(x, context); }
    void value_uint32(const IndexedData<uint32_t>& x, const ojson& context) { process_member<uint32_t>(x, context); }
    void value_int64(const IndexedData<int64_t>& x, const ojson& context) { process_member<int64_t>(x, context); }
    void value_uint64(const IndexedData<uint64_t>& x, const ojson& context) { process_member<uint64_t>(x, context); }
    void value_float(const IndexedData<float>& x, const ojson& context) { process_member<float>(x, context); }
    void value_double(const IndexedData<double>& x, const ojson& context) { process_member<double>(x, context); }
    //void value_void(void) { return ojson(nullptr); }
    void object_null(ufe::ObjectNull, const ojson& ctx) { /* do nothing */ }
    //void object_null_256(ufe::ObjectNullMultiple256 obj);
    static std::unordered_map<
        std::type_index, std::function<void(std::any const&, const ojson& context)>>
        m_any_visitor;
    void process(const std::any& a, const ojson& context);
    nlohmann::ordered_json m_json;
    std::vector<char> m_raw_data;
    std::vector <IndexedData<ufe::LengthPrefixedString>> m_updated_strings;
};

