#include "JsonReader.hpp"
JsonReader::JsonReader()
{
    // assign all callbacks once per run
    if (m_any_visitor.empty())
    {
        //register_any_visitor<ufe::ClassWithMembersAndTypes>( &JsonWriter::class_with_members_and_types);
         //m_any_visitor.insert(std::make_pair(std::type_index(typeid(ufe::ClassWithMembersAndTypes)),
         //    [this](const std::any& a)
         //    {
         //        return this->class_with_members_and_types(std::any_cast<ufe::ClassWithMembersAndTypes>(a));
         //    })
         //);
        register_any_visitor<ufe::ClassWithMembersAndTypes>(&JsonReader::class_with_members_and_types);
        register_any_visitor<ufe::ClassWithId>(&JsonReader::class_with_id);
        register_any_visitor<ufe::MemberReference>(&JsonReader::member_reference);
        register_any_visitor<ufe::BinaryObjectString>(&JsonReader::binary_object_string);
        register_any_visitor<ufe::ArraySingleString>(&JsonReader::array_single_string);
        register_any_visitor<ufe::ArraySinglePrimitive>(&JsonReader::array_single_primitive);
        register_any_visitor<ufe::BinaryArray>(&JsonReader::array_binary);
        register_any_visitor<ufe::ObjectNull>(&JsonReader::object_null);
        register_any_visitor<ufe::ObjectNullMultiple256>(&JsonReader::object_null_256);
        register_any_visitor<IndexedData<bool>>(&JsonReader::value_bool);
        register_any_visitor<IndexedData<char>>(&JsonReader::value_char);
        register_any_visitor<IndexedData<unsigned char>>(&JsonReader::value_uchar);
        register_any_visitor<IndexedData<int16_t>>(&JsonReader::value_int16);
        register_any_visitor<IndexedData<uint16_t>>(&JsonReader::value_uint16);
        register_any_visitor<IndexedData<int32_t>>(&JsonReader::value_int32);
        register_any_visitor<IndexedData<uint32_t>>(&JsonReader::value_uint32);
        register_any_visitor<IndexedData<int64_t>>(&JsonReader::value_int64);
        register_any_visitor<IndexedData<uint64_t>>(&JsonReader::value_uint64);
        register_any_visitor<IndexedData<float>>(&JsonReader::value_float);
        register_any_visitor<IndexedData<double>>(&JsonReader::value_double);
        //register_any_visitor<>(&JsonReader::);
    }
    // init json
    m_json =
    {
        {
            "records", {}
        }
    };
}

bool JsonReader::patch(std::filesystem::path json_path, std::filesystem::path binary_path, const BinaryFileParser& parser)
{
    if (std::filesystem::exists(json_path) && std::filesystem::is_regular_file(json_path))
    {
        if (std::filesystem::exists(binary_path) && std::filesystem::is_regular_file(binary_path))
        {
            m_raw_data = parser.raw_data();
            std::ifstream json{ json_path };
            spdlog::info("Patching file '{}'", binary_path.string());
            spdlog::info("with json file '{}'", json_path.string());
            try
            {
                json >> m_json;
                process_records(parser.get_records());
                update_strings();

                parser.close();
                std::ofstream bin{ binary_path, std::ios::binary };
                if (bin)
                {
                    bin.write(m_raw_data.data(), m_raw_data.size());
                }
            }
            catch (std::exception& e)
            {
                spdlog::critical("Failed to parse json file: {}", e.what());
            }
        }

    }
    return false;
}

std::unordered_map<
    std::type_index, std::function<void(std::any const&, const ojson&)>> JsonReader::m_any_visitor;

bool JsonReader::process_records(const std::vector<std::any>& records)
{
    const auto& json_records = m_json["records"];
    for (const auto& rec : records)
    {
        process(rec, json_records);
    }
    return false;
}

void JsonReader::update_strings()
{
    // update all strings in reverse order
    std::for_each(m_updated_strings.rbegin(), m_updated_strings.rend(),
        [this](IndexedData<ufe::LengthPrefixedString>& ref)
        {
            if (1)
            {
                std::string tmp;
                tmp.reserve(ref.m_data.m_str.size() + 5);
                auto len = ref.m_data.m_new_len;
                for (; len; len >>= 8)
                {
                    tmp.push_back(static_cast<char>(len & 0xFF));
                }
                tmp.append(ref.m_data.m_str.cbegin(), ref.m_data.m_str.cend());

                // erase old size
                len = ref.m_data.m_original_len_unmod;
                auto it = m_raw_data.begin();
                for (; len; len >>= 8)
                {
                    it = m_raw_data.erase(m_raw_data.begin() + ref.m_offset);
                }
                // erase old string data
                m_raw_data.erase(it, it + ref.m_data.m_original_len);

                m_raw_data.insert(m_raw_data.cbegin() + ref.m_offset, tmp.cbegin(), tmp.cend());
            }
        });
}

void JsonReader::binary_object_string(const ufe::BinaryObjectString& bos, const ojson& ctx)
{
    if (ctx.contains("obj_string_id"))
    {
        std::string json_str = ctx["value"];
        if (json_str != bos.m_Value.m_data.m_str)
        {
            spdlog::warn("orig_str: {}", bos.m_Value.m_data.m_str);
            spdlog::warn("json_str: {}", json_str);
            auto lps = bos.m_Value;
            lps.m_data.update_string(json_str);
            m_updated_strings.push_back(lps);
        }
    }
}

void JsonReader::class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt, const ojson& ctx)
{
    const auto& cls = find_class_by_id(ctx, cmt.m_ClassInfo, "class");
    if (!cls.is_null())
    {
        const auto& members = cls["members"];
        auto it_member_names = cmt.m_ClassInfo.MemberNames.cbegin();
        spdlog::debug("Processing class '{}' with id {}", cmt.m_ClassInfo.Name.m_data.m_str, cmt.m_ClassInfo.ObjectId.m_data);
        for (const auto& rec : cmt.m_MemberTypeInfo.Data)
        {
            
            if (json_elem(members, it_member_names->m_data.m_str))
            {
                spdlog::debug(it_member_names->m_data.m_str);
                process(rec, members[it_member_names->m_data.m_str]);
            }
            ++it_member_names;
        }
        spdlog::debug("Done class {}", cmt.m_ClassInfo.ObjectId.m_data);
    }
    else
    {
        spdlog::warn("Class '{}' with id {} not found", cmt.m_ClassInfo.Name.m_data.m_str, cmt.m_ClassInfo.ObjectId.m_data);
    }
}

void JsonReader::class_with_id(const ufe::ClassWithId& cwi, const ojson& ctx)
{
    const auto& cls = find_class_by_id(ctx, cwi.m_ClassInfo, "class_id");
    if (!cls.is_null())
    {
        const auto& members = cls["members"];
        auto it_member_names = cwi.m_ClassInfo.MemberNames.cbegin();
        spdlog::debug("Processing class_id '{}' with id {}", cwi.m_ClassInfo.Name.m_data.m_str, cwi.m_ClassInfo.ObjectId.m_data);
        for (const auto& rec : cwi.m_MemberTypeInfo.Data)
        {

            if (json_elem(members, it_member_names->m_data.m_str))
            {
                spdlog::debug(it_member_names->m_data.m_str);
                process(rec, members[it_member_names->m_data.m_str]);
            }
            ++it_member_names;
        }
        spdlog::debug("Done class_id {}", cwi.m_ClassInfo.ObjectId.m_data);
    }
    else
    {
        spdlog::warn("Class '{}' with id {} not found", cwi.m_ClassInfo.Name.m_data.m_str, cwi.m_ClassInfo.ObjectId.m_data);
    }
}

void JsonReader::array_single_string(const ufe::ArraySingleString& arr, const ojson& ctx)
{
    const auto& values = find_array_by_id(ctx, arr.ObjectId);
    process_array(values, arr.Data, "ArraySingleString", arr.ObjectId);
}

void JsonReader::array_binary(const ufe::BinaryArray& arr, const ojson& ctx)
{
    const auto& values = find_array_by_id(ctx, arr.ObjectId);
    process_array(values, arr.Data, "BinaryArray", arr.ObjectId);
}

void JsonReader::array_single_primitive(const ufe::ArraySinglePrimitive& arr, const ojson& ctx)
{
    const auto& values = find_array_by_id(ctx, arr.ObjectId);
    process_array(values, arr.Data, "ArraySinglePrimitive", arr.ObjectId);
}

void JsonReader::process_array(const ojson& values, const std::vector<std::any>& data, std::string arr_type, int32_t arr_id)
{
    if (values.is_array())
    {
        if (values.size() == data.size())
        {
            auto it_bin_values = data.cbegin();
            for (const auto& json_val : values)
            {
                process(*it_bin_values, json_val);
                ++it_bin_values;
            }
        }
        else
        {
            spdlog::error("{} with id {} values count mismatch with json count", arr_type, arr_id);
        }
    }
}

const ojson& JsonReader::find_class_by_id(const ojson& ctx, const ufe::ClassInfo& ci, std::string class_type)
{
    static ojson dummy(nullptr);
    // root class or array element
    if (ctx.is_array())
    {
        for (const auto& rec : ctx)
        {
            if (rec.contains(class_type))
            {
                auto id = rec[class_type]["id"].get<int32_t>();
                if (id == ci.ObjectId.m_data)
                {
                    return rec[class_type];
                }
            }
        }
    }

    // member value
    if (ctx.is_object() && ctx.contains(class_type))
    {
        return ctx[class_type];
    }

    return dummy;
}

const ojson& JsonReader::find_array_by_id(const ojson& ctx, int32_t arr_id)
{
    static ojson dummy(nullptr);
    // root class or array element
    if (ctx.is_array())
    {
        for (const auto& rec : ctx)
        {
            if (rec.contains("array_id") && rec.contains("values"))
            {
                auto id = rec["array_id"].get<int32_t>();
                if (id == arr_id)
                {
                    return rec["values"];
                }
            }
        }
    }
    return dummy;
}

void JsonReader::process(const std::any& a, const ojson& context)
{
    if (const auto it = m_any_visitor.find(std::type_index(a.type()));
        it != m_any_visitor.cend()) {
        it->second(a, context);
    }
    else {
        spdlog::error("JsonReader unregistered type: {}", a.type().name());
    }
}
