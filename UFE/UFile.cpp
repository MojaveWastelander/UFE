#include "UFile.hpp"
#include <algorithm>

bool FileParser::open(fs::path file_path)
{
    if (fs::exists(file_path) && fs::is_regular_file(file_path))
    {
        std::cout << "Reading file: " << file_path << '\n';
        m_file.open(file_path, std::ios::binary | std::ios::in | std::ios::out);
        m_file_path = file_path;
        // init json
        m_export =
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

void FileParser::write(const char* data, std::streamsize size, int offset)
{
    m_file.seekg(offset);
    m_file.write(data, size);
}

bool FileParser::export_json(fs::path json_path)
{
    std::ofstream out{ json_path };
    std::cout << "Saving json file: " << json_path << '\n';
    out << std::setw(4) << m_export;
    return true;
}

const std::any& FileParser::get_record(int32_t id)
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

void FileParser::read_records()
{
    for (std::any a = read_record(m_export["records"]); a.has_value(); a = read_record(m_export["records"]))
    {
    }
}

std::any FileParser::read_record(nlohmann::ordered_json& obj)
{
    ufe::ERecordType rec = get_record_type();
    spdlog::info("Parsing record type: {}", ufe::ERecordType2str(rec));
    switch (rec)
    {
        case ufe::ERecordType::SerializedStreamHeader:
        {
            ufe::SerializationHeaderRecord rec;
            read(rec);
            return std::any{ std::move(rec) };
        } break;
        case ufe::ERecordType::ClassWithId:
        {
            ufe::ClassWithId cwi;
            read(cwi, obj);
            return std::any{ std::move(cwi) };
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
            read(cmt, obj, true);
            add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
            return std::any{ std::move(cmt) };
        } break;
        case ufe::ERecordType::ClassWithMembersAndTypes:
        {
            ufe::ClassWithMembersAndTypes cmt;
            read(cmt, obj);
            add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
            return std::any{ std::move(cmt) };
        } break;
        case ufe::ERecordType::BinaryObjectString:
        {
            ufe::BinaryObjectString bos;
            read(bos);
            spdlog::debug("object string id: {}, value: '{}'", bos.m_ObjectId, bos.m_Value.m_data.m_str);
            add_record(bos.m_ObjectId, std::any{ bos });
            return std::any{ std::move(bos) };
        } break;
        case ufe::ERecordType::BinaryArray:
            spdlog::error("Record type not implemented!");
            break;
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
                obj["reference"] = ref.m_idRef;
            }
            catch (std::exception& e)
            {
                spdlog::error("Reference parse error: {}", e.what());
                spdlog::debug(obj.dump());
            }
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
        case ufe::ERecordType::ObjectNullMultiple:
        case ufe::ERecordType::ArraySinglePrimitive:
        case ufe::ERecordType::ArraySingleObject:
        case ufe::ERecordType::ArraySingleString:
        case ufe::ERecordType::MethodCall:
        case ufe::ERecordType::MethodReturn:
        case ufe::ERecordType::InvalidType:
        default:
            spdlog::error("Record type not implemented!");
            spdlog::debug("filepos: {}", m_file.tellg());
            break;
    }
    return std::any{};
}

bool FileParser::read(ufe::BinaryLibrary& bl)
{
    read(bl.LibraryId);
    read(bl.LibraryName);
    return true;
}

bool FileParser::read(ufe::ClassInfo& ci)
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

bool FileParser::read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj, bool system_class /* = false */)
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


bool FileParser::read(ufe::LengthPrefixedString& lps)
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

bool FileParser::read(ufe::ClassTypeInfo& cti)
{
    read(cti.TypeName);
    cti.LibraryId = read<int32_t>();
    return true;
}


bool FileParser::read(ufe::BinaryObjectString& bos)
{
    bos.m_ObjectId = read<int32_t>();
    read(bos.m_Value);
    return true;
}

bool FileParser::read(ufe::ClassWithId& cmt, nlohmann::ordered_json& obj)
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

bool FileParser::read(ufe::SerializationHeaderRecord& header)
{
    read(header.RootId);
    read(header.HeaderId);
    read(header.MajorVersion);
    read(header.MinorVersion);
    return true;
}

bool FileParser::read(ufe::MemberReference& ref)
{
    ref.m_idRef = read<int32_t>();
    return true;
}

void FileParser::read_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, nlohmann::ordered_json& obj)
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
                auto nested_data = read_record(tmp);
                mti.Data.push_back(nested_data);
                // read_record adds new record in json as array, this will add it as object for nested classes
                // and in case of not null objects
                if (!tmp.empty())
                {
                    if (nested_data.type() == typeid(ufe::ClassWithMembersAndTypes) || nested_data.type() == typeid(ufe::ClassWithId))
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
                obj[it_member_names->m_data.m_str] = {};
                mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
            } break;
            default:
                break;
        }
        ++it_member_names;
    }
}

std::vector<char> FileParser::raw_data()
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
bool FileParser::read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& data)
{
    data.m_offset = m_file.tellg();
    read(data.m_data);
    return true;
}

bool FileWriter::read_record(nlohmann::ordered_json& obj)
{
    ufe::ERecordType rec = m_reader.get_record_type();
    spdlog::debug("Parsing record: {}", ufe::ERecordType2str(rec));
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
            m_updated_strings.push_back(bos.m_Value);
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

void FileWriter::get_raw_data()
{
    m_raw_data = m_reader.raw_data();
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

    auto& json_class = find_class_members(cmt.m_ClassInfo.Name.m_data.m_str, obj);
    spdlog::trace(obj.dump());
    spdlog::trace(json_class.dump());
    update_members_data(mti, cmt.m_ClassInfo, json_class);
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
        auto& json_class = find_class_members(cmt.m_ClassInfo.Name.m_data.m_str, obj, cmt.MetadataId.m_data);
        spdlog::trace(obj.dump());
        spdlog::trace(json_class.dump());

        update_members_data(cmt.m_MemberTypeInfo, cmt.m_ClassInfo, json_class);
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
                read_record(obj);
                if (obj.contains(it_member_names->m_data.m_str))
                {
                    if (m_updated_strings.back().m_data.m_original_len < 0x80)
                    {
                        m_updated_strings.back().m_data.update_string(obj[it_member_names->m_data.m_str]);
                    }
                    else
                    {
                        spdlog::error("String requires file size modification, skipping");
                    }
                }
                //mti.Data.push_back(bstr);
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
                spdlog::trace(obj[it_member_names->m_data.m_str].dump());
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

nlohmann::ordered_json& FileWriter::find_class_members(std::string name, nlohmann::ordered_json& obj, int ref_id /*= 0*/)
{
    static nlohmann::ordered_json dummy;

    // looking for a ClassWithId records
    // searching by name and id
    if (ref_id)
    {
        if (obj.contains("class_id") && obj["class_id"]["ref_id"] == ref_id)
        {
            return obj["class_id"]["members"];
        }
        else
        {
            spdlog::warn("class_id doesn't match");
        }
    }

    // nested class not in array
    if (obj.contains("class") && obj["class"]["name"] == name)
    {
        return obj["class"]["members"];
    }

    for (auto& rec : obj["records"])
    {
        if (rec["class"]["name"] == name)
        {
            return rec["class"]["members"];
        }
    }
    return dummy;
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
            m_updated_strings.reserve(200);
            get_raw_data();
            while (read_record(json))
            {
            }

            // update all strings in reverse order
            std::for_each(m_updated_strings.rbegin(), m_updated_strings.rend(),
                [this](IndexedData<ufe::LengthPrefixedString>& ref)
                {
                    if (ref.m_data.m_original_len < 0x80)
                    {
                        uint8_t new_len = static_cast<uint8_t>(ref.m_data.m_new_len);
                        // check if string was updated
                        if (new_len > 0)
                        {
                            memcpy(reinterpret_cast<void*>(&m_raw_data[ref.m_offset]), reinterpret_cast<void*>(&new_len), 1);
                            m_raw_data.erase(m_raw_data.cbegin() + ref.m_offset + 1, m_raw_data.cbegin() + ref.m_offset + 1 + ref.m_data.m_original_len);
                            m_raw_data.insert(m_raw_data.cbegin() + ref.m_offset + 1, ref.m_data.m_str.cbegin(), ref.m_data.m_str.cend());
                        }

                        //m_reader.write(reinterpret_cast<const char*>(&len), 1, ref.m_offset);
                        //m_reader.write(ref.m_data.m_str.data(), ref.m_data.m_original_len, ref.m_offset + 1);

                    }                                        
                });

            m_reader.close();
            std::ofstream out{ binary_path, std::ios::binary | std::ios::trunc };
            out.write(m_raw_data.data(), m_raw_data.size());
        }
    }
    return false;
}
