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
        //m_any_visitor.insert(std::make_pair(std::type_index(typeid(ufe::ClassWithMembersAndTypes)),
        //    [this](const std::any& a)
        //    {
        //        return this->class_with_members_and_types(std::any_cast<ufe::ClassWithMembersAndTypes>(a));
        //    })
        //);
        register_any_visitor<ufe::ClassWithMembersAndTypes>(&JsonWriter::class_with_members_and_types);
        register_any_visitor<ufe::ClassWithId>(&JsonWriter::class_with_id);
        register_any_visitor<ufe::MemberReference>(&JsonWriter::member_reference);
        register_any_visitor<ufe::BinaryObjectString>(&JsonWriter::binary_object_string);
        register_any_visitor<ufe::ArraySingleString>(&JsonWriter::array_single_string);
        register_any_visitor<ufe::BinaryArray>(&JsonWriter::array_binary);
        register_any_visitor<ufe::ObjectNull>(&JsonWriter::object_null);
        register_any_visitor<ufe::ObjectNullMultiple256>(&JsonWriter::object_null_256);
        register_any_visitor<IndexedData<bool>>(&JsonWriter::value_bool);
        register_any_visitor<IndexedData<char>>(&JsonWriter::value_char);
        register_any_visitor<IndexedData<unsigned char>>(&JsonWriter::value_uchar);
        register_any_visitor<IndexedData<int16_t>>(&JsonWriter::value_int16);
        register_any_visitor<IndexedData<uint16_t>>(&JsonWriter::value_uint16);
        register_any_visitor<IndexedData<int32_t>>(&JsonWriter::value_int32);
        register_any_visitor<IndexedData<uint32_t>>(&JsonWriter::value_uint32);
        register_any_visitor<IndexedData<int64_t>>(&JsonWriter::value_int64);
        register_any_visitor<IndexedData<uint64_t>>(&JsonWriter::value_uint64);
        register_any_visitor<IndexedData<float>>(&JsonWriter::value_float);
        register_any_visitor<IndexedData<double>>(&JsonWriter::value_double);
        //register_any_visitor<>(&JsonWriter::);
    }
    // init json
    m_json =
    {
        {
            "records", {}
        }
    };
}

bool JsonWriter::save(std::filesystem::path json_path, const std::vector<std::any>& records)
{
    std::ofstream out_json{ json_path };
    if (out_json)
    {
        spdlog::debug("Parsing records into json");
        process_records(records);
        spdlog::info("Exporting data to '{}'", json_path.string());
        out_json << std::setw(4) << m_json;
        return true;
    }
    spdlog::error("Could not save '{}'", json_path.string());
    return false;
}

bool JsonWriter::process_records(const std::vector<std::any>& records)
{
    auto& json_records = m_json["records"];
    for (const auto& rec : records)
    {
        auto json = process(rec);
        if (!json.empty())
        {
            json_records.push_back(json);
        }
    }
    return false;
}

nlohmann::ordered_json JsonWriter::class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt)
{
    nlohmann::ordered_json cls = { {"class", {}} };
    cls["class"]["name"] = cmt.m_ClassInfo.Name.m_data.m_str;
    cls["class"]["id"] = cmt.m_ClassInfo.ObjectId.m_data;
    cls["class"]["members"] = {};
    spdlog::debug("process class {} with id {}", cmt.m_ClassInfo.Name.m_data.m_str, cmt.m_ClassInfo.ObjectId.m_data);
    process_class_members(cls["class"]["members"], cmt.m_ClassInfo, cmt.m_MemberTypeInfo);
    return cls;
}

void JsonWriter::process_class_members(nlohmann::ordered_json& members, const ufe::ClassInfo& ci, const ufe::MemberTypeInfo& mti)
{
    auto it_member_names = ci.MemberNames.cbegin();
    for (const auto& data : mti.Data)
    {
        members[it_member_names->m_data.m_str] = process(data);
        spdlog::debug("'{}' : {}", it_member_names->m_data.m_str, members[it_member_names->m_data.m_str].dump());
        ++it_member_names;
    }
}

ojson JsonWriter::member_reference(const ufe::MemberReference& mref)
{
    return ojson{ {"reference", mref.m_idRef} };
}

ojson JsonWriter::binary_object_string(const ufe::BinaryObjectString& bos)
{
    // return ojson(bos.m_Value.m_data.m_str);
    ojson str = nlohmann::ordered_json::value_t::object;
    str["obj_string_id"] = bos.m_ObjectId;
    str["value"] = bos.m_Value.m_data.m_str;
    return str;
}

ojson JsonWriter::class_with_id(const ufe::ClassWithId& cwi)
{
    nlohmann::ordered_json cls = { {"class_id", {}} };
    cls["class_id"]["name"] = cwi.m_ClassInfo.Name.m_data.m_str;
    cls["class_id"]["id"] = cwi.m_ClassInfo.ObjectId.m_data;
    cls["class_id"]["ref_id"] = cwi.MetadataId.m_data;
    cls["class_id"]["members"] = {};
    spdlog::debug("process class_id {} with id {}", cwi.m_ClassInfo.Name.m_data.m_str, cwi.m_ClassInfo.ObjectId.m_data);
    process_class_members(cls["class_id"]["members"], cwi.m_ClassInfo, cwi.m_MemberTypeInfo);
    return cls;
}

ojson JsonWriter::array_single_string(const ufe::ArraySingleString& arr)
{
    ojson str = nlohmann::ordered_json::value_t::object;
    str["array_id"] = arr.ObjectId;
    str["values"] = {};
    for (const auto& rec : arr.Data)
    {
        str["values"].push_back(process(rec));
    }
    return str;
}

ojson JsonWriter::array_binary(const ufe::BinaryArray& arr)
{
    ojson str = nlohmann::ordered_json::value_t::object;
    str["array_id"] = arr.ObjectId;
    str["values"] = {};
    for (const auto& rec : arr.Data)
    {
        str["values"].push_back(process(rec));
    }
    return str;
}

ojson JsonWriter::object_null_256(ufe::ObjectNullMultiple256 obj)
{
    ojson str = nlohmann::ordered_json::value_t::object;
    str["null_packed"] = obj.NullCount;
    return str;
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
        if (a.type() != std::type_index(typeid(ufe::SerializationHeaderRecord)) && 
            a.type() != std::type_index(typeid(ufe::BinaryLibrary)))
        {
            spdlog::error("JsonWriter unregistered type: {}", a.type().name());
        }
    }
    return nlohmann::ordered_json(nullptr);
}
