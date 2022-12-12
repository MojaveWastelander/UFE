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
    bool save(std::filesystem::path json_path, const std::vector<std::pair<int32_t, std::any>>& records);
private:

    template<class T, class F>
    inline void register_any_visitor(F const& f)
    {
        std::cout << "Register visitor for type "
            << std::quoted(typeid(T).name()) << '\n';
        m_any_visitor.insert(std::make_pair(
            std::type_index(typeid(T)),
            [this, g = f](std::any const& a)
            {
                return g(std::any_cast<T const&>(a));
            }
        ));
    }

    bool process_records(const std::vector<std::pair<int32_t, std::any>>& records);

    nlohmann::ordered_json class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt)
    {
        nlohmann::ordered_json cls = { {"class", {}} };

        cls["class"]["name"] = cmt.m_ClassInfo.Name.m_data.m_str;
        cls["class"]["id"] = cmt.m_ClassInfo.ObjectId.m_data;
        cls["class"]["members"] = {};
        auto& members = cls["class"]["members"];
        spdlog::debug("process class {} with id {}", cmt.m_ClassInfo.Name.m_data.m_str, cmt.m_ClassInfo.ObjectId.m_data);
        auto it_member_names = cmt.m_ClassInfo.MemberNames.cbegin();
        for (const auto& data : cmt.m_MemberTypeInfo.Data)
        {
            members[it_member_names->m_data.m_str] = process(data);
            spdlog::trace("'{}' : {}", it_member_names->m_data.m_str, members[it_member_names->m_data.m_str].dump());
        }
        return cls;
    }
    static std::unordered_map<
        std::type_index, std::function<nlohmann::ordered_json(std::any const&)>>
        m_any_visitor;
    nlohmann::ordered_json process(const std::any& a);
    nlohmann::ordered_json m_json;
};

