#include "BinaryFileParser.hpp"
#include <algorithm>

bool BinaryFileParser::open(fs::path file_path)
{
    if (fs::exists(file_path) && fs::is_regular_file(file_path))
    {
        std::cout << "Reading file: " << file_path << '\n';
        m_file.open(file_path, std::ios::binary | std::ios::in | std::ios::out);
        m_file_path = file_path;
        // init json
        m_json =
        {
            {
                "records", {}
            }
        };

        return true;
    }
    else
    {
        spdlog::error("Failed to open '{}'", file_path.string());
    }
    return false;
}

bool BinaryFileParser::export_json(fs::path json_path)
{
    std::ofstream out{ json_path };
    std::cout << "Saving json file: " << json_path << '\n';
    out << std::setw(4) << m_json;
    return true;
}

const std::any& BinaryFileParser::get_record(int32_t id)
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

void BinaryFileParser::read_records()
{
    for (AnyJson a = read_record(); a.dyn_obj.has_value(); a = read_record())
    {
        //spdlog::debug(a.json_obj.dump());
        if (!a.json_obj.empty())
        {
            m_json["records"].push_back(a.json_obj.front());
        }
    }
}

AnyJson BinaryFileParser::read_record()
{
    nlohmann::ordered_json record_json;
    ufe::ERecordType rec = get_record_type();
    spdlog::info("Parsing record type: {}", ufe::ERecordType2str(rec));
    switch (rec)
    {
        case ufe::ERecordType::SerializedStreamHeader:
        {
            ufe::SerializationHeaderRecord rec;
            read(rec);
            return {std::move(rec), record_json};
        } break;
        case ufe::ERecordType::ClassWithId:
        {
            ufe::ClassWithId cwi;
            read(cwi, record_json);
            return { std::move(cwi), record_json };
        } break;
        case ufe::ERecordType::SystemClassWithMembers:
            spdlog::error("Record type not implemented!");
            break;
        case ufe::ERecordType::ClassWithMembers:
            spdlog::error("Record type not implemented!");
            break;
        case ufe::ERecordType::SystemClassWithMembersAndTypes:
        {
            ufe::ClassWithMembersAndTypes cmt;
            read(cmt, record_json, true);
            add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
            return { std::move(cmt), record_json };
        } break;
        case ufe::ERecordType::ClassWithMembersAndTypes:
        {
            ufe::ClassWithMembersAndTypes cmt;
            read(cmt, record_json);
            add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
            return { std::move(cmt), record_json };
        } break;
        case ufe::ERecordType::BinaryObjectString:
        {
            ufe::BinaryObjectString bos;
            read(bos);
            spdlog::debug("object string id: {}, value: '{}'", bos.m_ObjectId, bos.m_Value.m_data.m_str);
            add_record(bos.m_ObjectId, std::any{ bos });
            return { std::move(bos), record_json };
        } break;
        case ufe::ERecordType::BinaryArray:
        {
            ufe::BinaryArray ba;
            read(ba);
            if (ba.Rank > 1)
            {
                spdlog::error("multidimensional array");
                return {};
            }
            for (int i = 0; i < ba.Lengths[0]; ++i)
            {
                switch (ba.TypeEnum)
                {
                    case ufe::EBinaryTypeEnumeration::Primitive:
                        read_primitive_element(std::get<ufe::EPrimitiveTypeEnumeration>(ba.AdditionalTypeInfo), record_json);
                        break;
                    case ufe::EBinaryTypeEnumeration::String:
                        break;
                    case ufe::EBinaryTypeEnumeration::Object:
                        break;
                    case ufe::EBinaryTypeEnumeration::SystemClass:
                        break;
                    case ufe::EBinaryTypeEnumeration::Class:
                    {
                        record_json.push_back(read_record().json_obj);
                    } break;
                    case ufe::EBinaryTypeEnumeration::ObjectArray:
                        break;
                    case ufe::EBinaryTypeEnumeration::StringArray:
                        break;
                    case ufe::EBinaryTypeEnumeration::PrimitiveArray:
                        break;
                    case ufe::EBinaryTypeEnumeration::None:
                        break;
                    default:
                        break;
                }
            }
            return { std::move(ba), record_json };
        } break;
        case ufe::ERecordType::MemberPrimitiveTyped:
            spdlog::error("Record type not implemented!");
            break;
        case ufe::ERecordType::MemberReference:
        {
            ufe::MemberReference ref;
            read(ref);
            spdlog::debug("reference id: {}", ref.m_idRef);
            try
            {
                record_json["reference"] = ref.m_idRef;
            }
            catch (std::exception& e)
            {
                spdlog::error("Reference parse error: {}", e.what());
                spdlog::trace(record_json.dump());
            }
            return { std::move(ref), record_json };
        } break;
        case ufe::ERecordType::ObjectNull:
        {
        } break;
        case ufe::ERecordType::MessageEnd:
            break;
        case ufe::ERecordType::BinaryLibrary:
        {
            ufe::BinaryLibrary bl;
            read(bl);
            add_record(bl.LibraryId.m_data, std::any{ bl });
            return { std::move(bl), record_json };
        } break;
        case ufe::ERecordType::ObjectNullMultiple256:
        {
            uint8_t null_count = read<uint8_t>();
            spdlog::debug("null_multiple_256 count: {}", null_count);
            return { std::move(ufe::ObjectNull{}), record_json };
        } break;
        case ufe::ERecordType::ObjectNullMultiple:
            spdlog::error("Record type not implemented!");
            spdlog::debug("filepos: {}", m_file.tellg());
            break;
        case ufe::ERecordType::ArraySinglePrimitive:
        {
            ufe::ArraySinglePrimitive arr;
            read(arr);
            for (int i = 0; i < arr.Length; ++i)
            {
                nlohmann::ordered_json tmp;
                auto x = arr.PrimitiveTypeEnum;
                read_primitive_element(x, record_json);
                record_json.push_back(tmp);
            }
            return { std::move(arr), record_json };
        } break;
        case ufe::ERecordType::ArraySingleObject:
            spdlog::error("Record type not implemented!");
            spdlog::debug("filepos: {}", m_file.tellg());
            break;
        case ufe::ERecordType::ArraySingleString:
        {
            ufe::ArraySingleString arr;
            read(arr);
            for (int i = 0; i < arr.Length; ++i)
            {
                nlohmann::ordered_json tmp;
                auto record = read_record();
                arr.Data.push_back(record.dyn_obj);
                record_json.push_back(record.json_obj);
            }
            return { std::move(arr), record_json };
        } break;
        case ufe::ERecordType::MethodCall:
        case ufe::ERecordType::MethodReturn:
        case ufe::ERecordType::InvalidType:
        default:
            spdlog::error("Record type not implemented!");
            spdlog::debug("filepos: {}", m_file.tellg());
            break;
    }
    return {};
}

void BinaryFileParser::read_primitive_element(ufe::EPrimitiveTypeEnumeration type, nlohmann::ordered_json& obj)
{
    switch (type)
    {
        case ufe::EPrimitiveTypeEnumeration::Boolean:
            read_primitive<bool>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Byte:
            read_primitive<char>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Char:
            read_primitive<char>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Decimal:
            break;
        case ufe::EPrimitiveTypeEnumeration::Double:
            read_primitive<double>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Int16:
            read_primitive<int16_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Int32:
            read_primitive<int32_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Int64:
            read_primitive<int64_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::SByte:
            break;
        case ufe::EPrimitiveTypeEnumeration::Single:
            // special case
            obj.push_back(float2str2double(read<float>()));
            break;
        case ufe::EPrimitiveTypeEnumeration::TimeSpan:
            read_primitive<uint64_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::DateTime:
            read_primitive<uint64_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::UInt16:
            read_primitive<uint16_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::UInt32:
            read_primitive<uint32_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::UInt64:
            read_primitive<uint64_t>(obj);
            break;
        case ufe::EPrimitiveTypeEnumeration::Null:
            break;
        case ufe::EPrimitiveTypeEnumeration::String:
            break;
        default:
            break;
    }
}

bool BinaryFileParser::read(ufe::BinaryLibrary& bl)
{
    read(bl.LibraryId);
    read(bl.LibraryName);
    return true;
}

bool BinaryFileParser::read(ufe::ClassInfo& ci)
{
    read(ci.ObjectId);
    read(ci.Name);
    read(ci.MemberCount);
    ci.MemberNames.resize(ci.MemberCount.m_data);
    std::for_each(ci.MemberNames.begin(), ci.MemberNames.end(),
        [this](auto& data)
        {
            read(data);
        });
    return false;
}

bool BinaryFileParser::read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj, bool system_class /* = false */)
{
    nlohmann::ordered_json cls = { {"class", {}} };

    read(cmt.m_ClassInfo);
    cls["class"]["name"] = cmt.m_ClassInfo.Name.m_data.m_str;
    cls["class"]["id"] = cmt.m_ClassInfo.ObjectId.m_data;
    cls["class"]["members"] = {};
    auto& mti = cmt.m_MemberTypeInfo;

    for (int i = 0; i < cmt.m_ClassInfo.MemberCount.m_data; ++i)
    {
        mti.BinaryTypeEnums.push_back(static_cast<ufe::EBinaryTypeEnumeration>(read()));
    }
    spdlog::debug("class name: {}", cmt.m_ClassInfo.Name.m_data.m_str);
    spdlog::debug("class id: {}", cmt.m_ClassInfo.ObjectId.m_data);
    spdlog::debug("members count: {}", cmt.m_ClassInfo.MemberCount.m_data);
    for (auto type : mti.BinaryTypeEnums)
    {
        switch (type)
        {
            case ufe::EBinaryTypeEnumeration::Primitive:
            {
                auto ptype = static_cast<ufe::EPrimitiveTypeEnumeration>(read());
                mti.AdditionalInfos.push_back(ptype);
                spdlog::debug("\t{} -> {}", ufe::EBinaryTypeEnumeration2str(type), ufe::EPrimitiveTypeEnumeration2str(ptype));
            } break;
            case ufe::EBinaryTypeEnumeration::String:
                spdlog::debug("\t{}", ufe::EBinaryTypeEnumeration2str(type));
                break;
            case ufe::EBinaryTypeEnumeration::Object:
                spdlog::debug("\t{}", ufe::EBinaryTypeEnumeration2str(type));
                break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                spdlog::debug("\t{}", ufe::EBinaryTypeEnumeration2str(type));
                ufe::LengthPrefixedString str;
                read(str);
                mti.AdditionalInfos.push_back(str);
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                spdlog::debug("\t{}", ufe::EBinaryTypeEnumeration2str(type));
                ufe::ClassTypeInfo cti;
                read(cti);
                mti.AdditionalInfos.push_back(cti);
            } break;
            case ufe::EBinaryTypeEnumeration::ObjectArray:
            case ufe::EBinaryTypeEnumeration::StringArray:
                break;
            case ufe::EBinaryTypeEnumeration::PrimitiveArray:
            {
                auto ptype = static_cast<ufe::EPrimitiveTypeEnumeration>(read());
                mti.AdditionalInfos.push_back(ptype);
                spdlog::debug("\t{} -> {}", ufe::EBinaryTypeEnumeration2str(type), ufe::EPrimitiveTypeEnumeration2str(ptype));
            } break;
            case ufe::EBinaryTypeEnumeration::None:
                break;
            default:
                break;
        }
    }
    if (!system_class)
    {
        mti.LibraryId = read<int32_t>();
        spdlog::debug("library id: {}", mti.LibraryId);
    }
    read_members_data(mti, cmt.m_ClassInfo, cls["class"]["members"]);
    try
    {
        obj.push_back(cls);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return false;
}


bool BinaryFileParser::read(ufe::LengthPrefixedString& lps)
{
    uint32_t len = 0;
    lps.m_original_len = 0;
    for (int i = 0; i < 5; ++i)
    {
        uint8_t seg = read();
        lps.m_original_len |= static_cast<uint64_t>(seg) << (8 * i);
        len |= static_cast<uint32_t>(seg & 0x7F) << (7 * i);
        if ((seg & 0x80) == 0x00) break;
    }
    lps.m_str.resize(len);
    m_file.read(lps.m_str.data(), len);
    return true;
}

bool BinaryFileParser::read(ufe::ClassTypeInfo& cti)
{
    read(cti.TypeName);
    cti.LibraryId = read<int32_t>();
    return true;
}


bool BinaryFileParser::read(ufe::BinaryObjectString& bos)
{
    bos.m_ObjectId = read<int32_t>();
    read(bos.m_Value);
    return true;
}

bool BinaryFileParser::read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj)
{
    nlohmann::ordered_json cls = { {"class_id", {}} };
    IndexedData<int32_t> obj_id;

    read(obj_id);
    read(cmt.MetadataId);
    const auto& ref_class = get_record(cmt.MetadataId.m_data);
    if (ref_class.has_value())
    {
        const auto& ref = std::any_cast<const ufe::ClassWithMembersAndTypes&>(ref_class);
        cmt.m_ClassInfo = ref.m_ClassInfo;
        cmt.m_MemberTypeInfo = ref.m_MemberTypeInfo;
        cmt.m_ClassInfo.ObjectId = obj_id;
        cls["class_id"]["name"] = cmt.m_ClassInfo.Name.m_data.m_str;
        cls["class_id"]["id"] = cmt.m_ClassInfo.ObjectId.m_data;
        cls["class_id"]["ref_id"] = cmt.MetadataId.m_data;
        cls["class_id"]["members"] = {};
        read_members_data(cmt.m_MemberTypeInfo, cmt.m_ClassInfo, cls["class_id"]["members"]);
        obj.push_back(cls);
        return true;
    }
    return false;
}

bool BinaryFileParser::read(ufe::SerializationHeaderRecord& header)
{
    read(header.RootId);
    read(header.HeaderId);
    read(header.MajorVersion);
    read(header.MinorVersion);
    return true;
}

bool BinaryFileParser::read(ufe::MemberReference& ref)
{
    ref.m_idRef = read<int32_t>();
    return true;
}

bool BinaryFileParser::read(ufe::ArraySingleString& arr_str)
{
    arr_str.ObjectId = read<int32_t>();
    arr_str.Length = read<int32_t>();
    return true;
}

bool BinaryFileParser::read(ufe::ArraySinglePrimitive& arr_prim)
{
    arr_prim.ObjectId = read<int32_t>();
    arr_prim.Length = read<int32_t>();
    arr_prim.PrimitiveTypeEnum = static_cast<ufe::EPrimitiveTypeEnumeration>(read());
    return true;
}

bool BinaryFileParser::read(ufe::BinaryArray& arr_bin)
{
    arr_bin.ObjectId = read<int32_t>();
    arr_bin.BinaryArrayTypeEnum = static_cast<ufe::EBinaryArrayTypeEnumeration>(read());
    arr_bin.Rank = read<int32_t>();
    arr_bin.Lengths.resize(arr_bin.Rank);
    for (auto& len : arr_bin.Lengths)
    {
        len = read<int32_t>();
    }
    if (arr_bin.BinaryArrayTypeEnum == ufe::EBinaryArrayTypeEnumeration::JaggedOffset ||
        arr_bin.BinaryArrayTypeEnum == ufe::EBinaryArrayTypeEnumeration::RectangularOffset ||
        arr_bin.BinaryArrayTypeEnum == ufe::EBinaryArrayTypeEnumeration::SingleOffset)
    {
        arr_bin.LowerBounds.resize(arr_bin.Rank);
        for (auto& len : arr_bin.LowerBounds)
        {
            len = read<int32_t>();
        }
    }
    arr_bin.TypeEnum = static_cast<ufe::EBinaryTypeEnumeration>(read());
    switch (arr_bin.TypeEnum)
    {
        case ufe::EBinaryTypeEnumeration::Primitive:
            arr_bin.AdditionalTypeInfo = static_cast<ufe::EPrimitiveTypeEnumeration>(read());
            break;
        case ufe::EBinaryTypeEnumeration::String:
            break;
        case ufe::EBinaryTypeEnumeration::Object:
            break;
        case ufe::EBinaryTypeEnumeration::SystemClass:
        {
            ufe::LengthPrefixedString lps;
            read(lps);
            arr_bin.AdditionalTypeInfo = lps;
        } break;
        case ufe::EBinaryTypeEnumeration::Class:
        {
            ufe::ClassTypeInfo cti;
            read(cti);
            arr_bin.AdditionalTypeInfo = cti;
        } break;
        case ufe::EBinaryTypeEnumeration::ObjectArray:
            break;
        case ufe::EBinaryTypeEnumeration::StringArray:
            break;
        case ufe::EBinaryTypeEnumeration::PrimitiveArray:
            arr_bin.AdditionalTypeInfo = static_cast<ufe::EPrimitiveTypeEnumeration>(read());
            break;
        case ufe::EBinaryTypeEnumeration::None:
            break;
        default:
            break;
    }
    return true;
}

void BinaryFileParser::read_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, nlohmann::ordered_json& obj)
{
    auto it_add_info = mti.AdditionalInfos.cbegin();
    auto it_member_names = ci.MemberNames.cbegin();

    for (auto type : mti.BinaryTypeEnums)
    {
        switch (type)
        {
            case ufe::EBinaryTypeEnumeration::Primitive:
            {
                switch (std::get<ufe::EPrimitiveTypeEnumeration>(*it_add_info))
                {
                    case ufe::EPrimitiveTypeEnumeration::Boolean:
                    {
                        process_member<bool>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Byte:
                    {
                        process_member<uint8_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Char:
                    {
                        process_member<char>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Decimal:
                        throw std::exception("Not implemented!!!");
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Double:
                    {
                        process_member<double>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int16:
                    {
                        process_member<int16_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int32:
                    {
                        process_member<int32_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int64:
                    {
                        process_member<int64_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::SByte:
                        throw std::exception("Not implemented!!!");
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Single:
                    {
                        // 
                        float tmp = read<float>();
                        obj[it_member_names->m_data.m_str] = float2str2double(tmp);
                        spdlog::debug("\t{} = {}", it_member_names->m_data.m_str, tmp);
                        mti.Data.push_back(std::any{ tmp });
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::TimeSpan:
                    {
                        process_member<int64_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::DateTime:
                    {
                        process_member<int64_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt16:
                    {
                        process_member<uint16_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt32:
                    {
                        process_member<uint32_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt64:
                    {
                        process_member<uint64_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Null:
                        throw std::exception("Not implemented!!!");
                        break;
                    case ufe::EPrimitiveTypeEnumeration::String:
                        throw std::exception("Not implemented!!!");
                        break;
                    default:
                        break;
                }
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::String:
            {
                auto bstr = read_record();
                if (bstr.dyn_obj.has_value())
                {
                    try
                    {
                        const auto& str = std::any_cast<const ufe::BinaryObjectString&>(bstr.dyn_obj);
                        obj[it_member_names->m_data.m_str] = str.m_Value.m_data.m_str;
                    }
                    catch (std::exception& e)
                    {
                        spdlog::error(e.what());
                    }
                }
                else
                {
                    obj[it_member_names->m_data.m_str] = {};
                }
                mti.Data.push_back(bstr.dyn_obj);
            } break;
            case ufe::EBinaryTypeEnumeration::Object:
            {
                auto rec = read_record();
                obj[it_member_names->m_data.m_str] = rec.json_obj;
                mti.Data.push_back(rec.dyn_obj);
            } break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                auto rec = read_record();
                obj[it_member_names->m_data.m_str] = rec.json_obj;
                mti.Data.push_back(rec.dyn_obj);
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                //obj[it_member_names->m_data.m_str] = {};
                nlohmann::ordered_json tmp;
                auto nested_data = read_record();
                mti.Data.push_back(nested_data.dyn_obj);
                // read_record adds new record in json as array, this will add it as object for nested classes
                // and in case of not null objects
                if (!tmp.empty())
                {
                    if (nested_data.dyn_obj.type() == typeid(ufe::ClassWithMembersAndTypes) || nested_data.dyn_obj.type() == typeid(ufe::ClassWithId))
                    {
                        obj[it_member_names->m_data.m_str] = tmp.front();
                    }
                    else
                    {
                        // assing whole data
                        obj[it_member_names->m_data.m_str] = tmp;
                    }
                    spdlog::debug(obj[it_member_names->m_data.m_str].dump());
                }
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::ObjectArray:
            case ufe::EBinaryTypeEnumeration::StringArray:
            case ufe::EBinaryTypeEnumeration::PrimitiveArray:
            case ufe::EBinaryTypeEnumeration::None:
            {
                auto record = read_record();
                obj[it_member_names->m_data.m_str] = record.json_obj;
                mti.Data.push_back(record.dyn_obj);
            } break;
            default:
                break;
        }
        ++it_member_names;
    }
}

std::vector<char> BinaryFileParser::raw_data()
{
    std::vector<char> tmp;
    auto curr_offset = m_file.tellg();
    m_file.seekg(0);
    tmp.resize(fs::file_size(m_file_path));
    m_file.read(tmp.data(), tmp.size());
    m_file.seekg(curr_offset);
    return tmp;
}

template<>
bool BinaryFileParser::read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& data)
{
    data.m_offset = m_file.tellg();
    read(data.m_data);
    return true;
}

