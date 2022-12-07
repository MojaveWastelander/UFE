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

bool FileReader::read(ufe::ClassWithMembersAndTypes& cmt, nlohmann::ordered_json& obj)
{
    nlohmann::ordered_json cls = { {"class", {}} };

    read(cmt.m_ClassInfo);
    cls["class"]["name"] = cmt.m_ClassInfo.Name.m_data.m_str;
    cls["class"]["members"] = {};
    auto& mti = cmt.m_MemberTypeInfo;

    for (int i = 0; i < cmt.m_ClassInfo.MemberCount.m_data; ++i)
    {
        mti.BinaryTypeEnums.push_back(static_cast<ufe::EBinaryTypeEnumeration>(read()));
    }

    for (auto type : mti.BinaryTypeEnums)
    {
        switch (type)
        {
            case ufe::EBinaryTypeEnumeration::Primitive:
            {
                mti.AdditionalInfos.push_back(static_cast<ufe::EPrimitiveTypeEnumeration>(read()));
            } break;
            case ufe::EBinaryTypeEnumeration::String:
                break;
            case ufe::EBinaryTypeEnumeration::Object:
                break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                ufe::LengthPrefixedString str;
                read(str);
                mti.AdditionalInfos.push_back(str);
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                ufe::ClassTypeInfo cti;
                read(cti);
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

    mti.LibraryId = read<int32_t>();

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
    auto cnt = m_file.readsome(lps.m_str.data(), len);
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
    nlohmann::ordered_json cls = { {"class", {}} };
    IndexedData<int32_t> obj_id;

    cls["class"]["name"] = cmt.m_ClassInfo.Name.m_data.m_str;
    cls["class"]["members"] = {};    
    read(obj_id);
    read(cmt.MetadataId);
    const auto& ref_class = get_record(cmt.MetadataId.m_data);
    if (ref_class.has_value())
    {
        const auto& ref = std::any_cast<const ufe::ClassWithMembersAndTypes&>(ref_class);
        cmt.m_ClassInfo = ref.m_ClassInfo;
        cmt.m_MemberTypeInfo = ref.m_MemberTypeInfo;
        cmt.m_ClassInfo.ObjectId = obj_id;
        read_members_data(cmt.m_MemberTypeInfo, cmt.m_ClassInfo, cls["class"]["members"]);
        obj.push_back(cls);
        return true;
    }
    return false;
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
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Char:
                    {
                        process_member<char>(obj, it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Decimal:
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
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Single:
                    {
                        // json uses double internally and float->double casts are imprecise
                        // 
                        float tmp = read<float>();
                        std::string sf = std::to_string(tmp);
                        obj[it_member_names->m_data.m_str] = std::stold(sf);
                        mti.Data.push_back(std::any{ tmp });
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::TimeSpan:
                        break;
                    case ufe::EPrimitiveTypeEnumeration::DateTime:
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
                const auto& bstr = read_record(obj);
                if (bstr.has_value())
                {
                    const auto& str = std::any_cast<const ufe::BinaryObjectString&>(bstr);
                    obj[it_member_names->m_data.m_str] = str.m_Value.m_data.m_str;
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
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                obj[it_member_names->m_data.m_str] = {};
                mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
                // read_record adds new record in json as array, this will add it as object for nested classes
                obj[it_member_names->m_data.m_str] = obj[it_member_names->m_data.m_str].front();
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

bool FileWriter::update_file(fs::path& binary_path, fs::path& json_path)
{
    nlohmann::ordered_json json;
    std::ifstream json_file{ json_path };
    if (json_file)
    {
        json_file >> json;


    }
}
