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
//#include <gzip/compress.hpp>
//#include <gzip/config.hpp>
//#include <gzip/decompress.hpp>
//#include <gzip/utils.hpp>
//#include <gzip/version.hpp>
//#include <zlib.h>
class JsonWriter
{
public:
    JsonWriter();
    bool save(std::filesystem::path json_path, const std::vector<std::any>& records);
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
        m_any_visitor[std::type_index(typeid(T))] = [this, f](std::any const& a) -> nlohmann::ordered_json {
            if (std::is_fundamental_v<T>)
            {
                return std::bind(f, this, std::any_cast<T>(a))();
            }
            else
            {
                return std::bind(f, this, std::cref(std::any_cast<T const&>(a)))();
            }
        };
    }

    bool process_records(const std::vector<std::any>& records);

    nlohmann::ordered_json class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt);

    void process_class_members(nlohmann::ordered_json& members, const  ufe::ClassInfo& ci, const  ufe::MemberTypeInfo& mti);

    ojson member_reference(const ufe::MemberReference& mref);
    ojson binary_object_string(const ufe::BinaryObjectString& bos);
    ojson class_with_id(const ufe::ClassWithId& cwi);
    ojson array_single_string(const ufe::ArraySingleString& arr);
    ojson array_binary(const ufe::BinaryArray& arr);
    ojson value_char(const IndexedData<char>& x) { return ojson(x.value); }
    ojson value_uchar(const IndexedData<unsigned char>& x) { return ojson(x.value); }
    ojson value_bool(const IndexedData<bool>& x) { return ojson(x.value); }
    ojson value_int32(const IndexedData<int32_t>& x) { return ojson(x.value); }
    ojson value_uint32(const IndexedData<uint32_t>& x) { return ojson(x.value); }
    ojson value_int16(const IndexedData<int16_t>& x) { return ojson(x.value); }
    ojson value_uint16(const IndexedData<uint16_t>& x) { return ojson(x.value); }
    ojson value_int64(const IndexedData<int64_t>& x) { return ojson(x.value); }
    ojson value_uint64(const IndexedData<uint64_t>& x) { return ojson(x.value); }
    ojson value_float(const IndexedData<float>& x) { return ojson(float2str2double(x.value)); }
    ojson value_double(const IndexedData<double>& x) { return ojson(x.value); }
    ojson value_void(void) { return ojson(nullptr); }
    ojson object_null(ufe::ObjectNull) { return ojson(nullptr); }
    ojson object_null_256(ufe::ObjectNullMultiple256 obj);
    static std::unordered_map<
        std::type_index, std::function<nlohmann::ordered_json(std::any const&)>>
        m_any_visitor;
    nlohmann::ordered_json process(const std::any& a);
    nlohmann::ordered_json m_json;
};

