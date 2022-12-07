#include "UFile.hpp"
#include <algorithm>

bool FileReader::read(ufe::BinaryLibrary& bl)
{
    read(bl.LibraryId);
    read(bl.LibraryName);
    return true;
}

bool FileReader::read(ufe::ClassInfo& ci)
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

bool FileReader::read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj, bool system_class /* = false */)
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
            case ufe::EBinaryTypeEnumeration::PrimitiveArray:
            case ufe::EBinaryTypeEnumeration::None:
                spdlog::debug("\t{}", ufe::EBinaryTypeEnumeration2str(type));
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


bool FileReader::read(ufe::LengthPrefixedString& lps)
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

bool FileReader::read(ufe::ClassTypeInfo& cti)
{
    read(cti.TypeName);
    cti.LibraryId = read<int32_t>();
    return true;
}


bool FileReader::read(ufe::BinaryObjectString& bos)
{
    bos.m_ObjectId = read<int32_t>();
    read(bos.m_Value);
    return true;
}

bool FileReader::read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj)
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

bool FileReader::read(ufe::SerializationHeaderRecord& header)
{
    read(header.RootId);
    read(header.HeaderId);
    read(header.MajorVersion);
    read(header.MinorVersion);
    return true;
}

void FileReader::read_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, nlohmann::ordered_json& obj)
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
                        // json uses double internally and float->double casts are imprecise
                        // 
                        float tmp = read<float>();
                        std::string sf = std::to_string(tmp);
                        obj[it_member_names->m_data.m_str] = std::stold(sf);
                        spdlog::debug("\t{} = {}", it_member_names->m_data.m_str, tmp);
                        mti.Data.push_back(std::any{ tmp });
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::TimeSpan:
                    {
                        process_member<int64_t>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::DateTime:
                        throw std::exception("Not implemented!!!");
                            break;
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
                const auto& bstr = read_record(obj);
                if (bstr.has_value())
                {
                    try
                    {
                        const auto& str = std::any_cast<const ufe::BinaryObjectString&>(bstr);
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
                mti.Data.push_back(bstr);
            } break;
            case ufe::EBinaryTypeEnumeration::Object:
            {
                obj[it_member_names->m_data.m_str] = {};
                mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
            } break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                obj[it_member_names->m_data.m_str] = {};
                mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                //obj[it_member_names->m_data.m_str] = {};
                nlohmann::ordered_json tmp;
                mti.Data.push_back(read_record(tmp));
                // read_record adds new record in json as array, this will add it as object for nested classes
                // and in case of not null objects
                spdlog::debug(tmp.dump());
                if (!tmp.empty())
                {
                    obj[it_member_names->m_data.m_str] = tmp;
                    spdlog::debug(obj[it_member_names->m_data.m_str].dump());
                }
                ++it_add_info;
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
        ++it_member_names;
    }
}

template<>
bool FileReader::read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& data)
{
    data.m_offset = m_file.tellg();
    read(data.m_data);
    return true;
}

bool FileWriter::read_record(nlohmann::ordered_json& obj)
{
    ufe::ERecordType rec = m_reader.get_record_type();
    ufe::log_line(std::format("Parsing record: {}", ufe::ERecordType2str(rec)));
    switch (rec)
    {
        case ufe::ERecordType::SerializedStreamHeader:
        {
            ufe::SerializationHeaderRecord rec;
            m_reader.read(rec);
        } break;
        case ufe::ERecordType::ClassWithId:
        {
            ufe::ClassWithId cwi;
            read(cwi, obj);
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
            m_reader.add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
        } break;
        case ufe::ERecordType::BinaryObjectString:
        {
            ufe::BinaryObjectString bos;
            m_reader.read(bos);
            m_reader.add_record(bos.m_ObjectId, std::any{ bos });
        } break;
        case ufe::ERecordType::BinaryArray:
            break;
        case ufe::ERecordType::MemberPrimitiveTyped:
            break;
        case ufe::ERecordType::MemberReference:
        {
            ufe::MemberReference ref;
            m_reader.read(ref);
        } break;
        case ufe::ERecordType::ObjectNull:
        {
        } break;
        case ufe::ERecordType::MessageEnd: return false;
        case ufe::ERecordType::BinaryLibrary:
        {
            ufe::BinaryLibrary bl;
            m_reader.read(bl);
            m_reader.add_record(bl.LibraryId.m_data, std::any{ bl });
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
    return true;
}

bool FileWriter::read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj)
{
    m_reader.read(cmt.m_ClassInfo);
    auto& mti = cmt.m_MemberTypeInfo;

    for (int i = 0; i < cmt.m_ClassInfo.MemberCount.m_data; ++i)
    {
        mti.BinaryTypeEnums.push_back(static_cast<ufe::EBinaryTypeEnumeration>(m_reader.read()));
    }

    for (auto type : mti.BinaryTypeEnums)
    {
        switch (type)
        {
            case ufe::EBinaryTypeEnumeration::Primitive:
            {
                mti.AdditionalInfos.push_back(static_cast<ufe::EPrimitiveTypeEnumeration>(m_reader.read()));
            } break;
            case ufe::EBinaryTypeEnumeration::String:
                break;
            case ufe::EBinaryTypeEnumeration::Object:
                break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                ufe::LengthPrefixedString str;
                m_reader.read(str);
                mti.AdditionalInfos.push_back(str);
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                ufe::ClassTypeInfo cti;
                m_reader.read(cti);
                mti.AdditionalInfos.push_back(cti);
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

    mti.LibraryId = m_reader.read<int32_t>();

    auto& json_class = find_class(cmt.m_ClassInfo.Name.m_data.m_str, obj);
    std::cerr << '\n' << obj.dump() << '\n';
    std::cerr << '\n' << json_class.dump() << '\n';
    std::cerr << '\n' << json_class["class"]["members"].dump() << '\n';
    update_members_data(mti, cmt.m_ClassInfo, json_class["class"]["members"]);
    return false;
}

bool FileWriter::read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj)
{
    IndexedData<int32_t> obj_id;

    m_reader.read(obj_id);
    m_reader.read(cmt.MetadataId);
    const auto& ref_class = m_reader.get_record(cmt.MetadataId.m_data);
    if (ref_class.has_value())
    {
        const auto& ref = std::any_cast<const ufe::ClassWithMembersAndTypes&>(ref_class);
        cmt.m_ClassInfo = ref.m_ClassInfo;
        cmt.m_MemberTypeInfo = ref.m_MemberTypeInfo;
        cmt.m_ClassInfo.ObjectId = obj_id;
        auto& json_class = find_class(cmt.m_ClassInfo.Name.m_data.m_str, obj);
        std::cerr << '\n' << obj.dump() << '\n';
        std::cerr << '\n' << json_class.dump() << '\n';
        std::cerr << '\n' << json_class["class"]["members"].dump() << '\n';

        update_members_data(cmt.m_MemberTypeInfo, cmt.m_ClassInfo, json_class["class"]["members"]);
        return true;
    }
    return false;
}

void FileWriter::update_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, nlohmann::ordered_json& obj)
{
    auto it_add_info = mti.AdditionalInfos.cbegin();
    auto it_member_names = ci.MemberNames.cbegin();

    for (auto type : mti.BinaryTypeEnums)
    {
        switch (type)
        {
            case ufe::EBinaryTypeEnumeration::Primitive:
            {
                std::cout << obj.dump() << '\n';
                switch (std::get<ufe::EPrimitiveTypeEnumeration>(*it_add_info))
                {
                    case ufe::EPrimitiveTypeEnumeration::Boolean:
                    {
                        process_member<bool>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Byte:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Char:
                    {
                        process_member<char>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Decimal:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Double:
                    {
                        process_member<double>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int16:
                    {
                        process_member<int16_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int32:
                    {
                        process_member<int32_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int64:
                    {
                        process_member<int64_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::SByte:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Single:
                    {
                        // json uses double internally and float->double casts are imprecise
                        // 
                        //float tmp = read<float>();
                        //std::string sf = std::to_string(tmp);
                        //obj[it_member_names->m_data.m_str] = std::stold(sf);
                        //mti.Data.push_back(std::any{ tmp });
                        process_member<float>(obj, it_member_names->m_data.m_str);;
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::TimeSpan:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::DateTime:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::UInt16:
                    {
                        process_member<uint16_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt32:
                    {
                        process_member<uint32_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt64:
                    {
                        process_member<uint64_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Null:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::String:
                        break;
                    default:
                        break;
                }
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::String:
            {
                const auto& bstr = m_reader.read_record(obj);
                if (bstr.has_value())
                {
                    const auto& str = std::any_cast<const ufe::BinaryObjectString&>(bstr);
                }
                mti.Data.push_back(bstr);
            } break;
            case ufe::EBinaryTypeEnumeration::Object:
            {
                mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
            } break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                std::cerr << obj[it_member_names->m_data.m_str].dump() << '\n';
                mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
                ++it_add_info;
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
        ++it_member_names;
    }
}

bool FileWriter::update_file(fs::path& binary_path, fs::path& json_path)
{
    nlohmann::ordered_json json;
    nlohmann::ordered_json dummy;
    std::ifstream json_file{ json_path };
    if (json_file)
    {
        json_file >> json;
        
        if (m_reader.open(binary_path))
        {
            while (read_record(json))
            {
            }
        }
    }
    return false;
}
