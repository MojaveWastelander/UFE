#include "JsonWriter.hpp"

nlohmann::ordered_json test(int x)
{
    return {};
}

JsonWriter::JsonWriter()
{
    // assign all callbacks
    if (m_any_visitor.empty())
    {
       //register_any_visitor<ufe::ClassWithMembersAndTypes>( &JsonWriter::class_with_members_and_types);
        m_any_visitor.insert(std::make_pair(std::type_index(typeid(ufe::ClassWithMembersAndTypes)),
            [this](const std::any& a)
            {
                return this->class_with_members_and_types(std::any_cast<ufe::ClassWithMembersAndTypes>(a));
            })
        );
    }
}

bool JsonWriter::save(std::filesystem::path json_path, const std::vector<std::pair<int32_t, std::any>>& records)
{
    process_records(records);
    return false;
}

bool JsonWriter::process_records(const std::vector<std::pair<int32_t, std::any>>& records)
{
    for (const auto& [id , rec] : records)
    {
        m_json.push_back(process(rec));
    }
    return false;
}

std::unordered_map<
    std::type_index, std::function<nlohmann::ordered_json(std::any const&)>> JsonWriter::m_any_visitor;

nlohmann::ordered_json JsonWriter::process(const std::any& a)
{
    if (const auto it = m_any_visitor.find(std::type_index(a.type()));
        it != m_any_visitor.cend()) {
        return it->second(a);
    }
    else {
        spdlog::error("Unregistered type: {}", a.type().name());
    }
    return nlohmann::ordered_json{};
}
