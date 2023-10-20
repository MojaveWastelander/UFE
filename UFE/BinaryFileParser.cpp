#include "BinaryFileParser.hpp"
#include <gzip\config.hpp>
#include <gzip\decompress.hpp>
#include <gzip\utils.hpp>
#include <gzip\version.hpp>
#include <zlib.h>
#include <algorithm>

bool BinaryFileParser::open(fs::path file_path)
{
    if (fs::exists(file_path))
    {
        if (fs::is_regular_file(file_path))
        {
	        spdlog::info("Reading file: {} ", file_path.string());
            std::ifstream in_file;
	        in_file.open(file_path, std::ios::binary | std::ios::in | std::ios::out);
	        if (m_file && fs::file_size(file_path) > (GZIP_START_OFF + 4))
	        {
                m_raw_data.resize(GZIP_START_OFF + 4);
                in_file.read(m_raw_data.data(), m_raw_data.size());
                in_file.seekg(0);
                // file is compressed
                if (static_cast<uint8_t>(m_raw_data[GZIP_START_OFF]) == GZIP_MAGIC_1 &&
                    static_cast<uint8_t>(m_raw_data[GZIP_START_OFF + 1]) == GZIP_MAGIC_2)
                {
                    m_header.resize(GZIP_START_OFF);
                    in_file.read(m_header.data(), m_header.size());
                    m_raw_data.resize(fs::file_size(file_path) - GZIP_START_OFF);
                    in_file.read(m_raw_data.data(), m_raw_data.size());
                    try
                    {
                        m_raw_data = gzip::decompress(m_raw_data.data(), m_raw_data.size());
                        m_file.str(m_raw_data);
                        m_file_type = EFileType::Compressed;
                    }
                    catch (std::exception& e)
                    {
                        spdlog::critical("Failed to decompress file: {}", e.what());
                    }
                }
                else if (*reinterpret_cast<uint32_t*>(&m_raw_data[GZIP_START_OFF]) == 0x00000100)
                {
                    // uncompressed file
                    m_header.resize(GZIP_START_OFF);
                    in_file.read(m_header.data(), m_header.size());
                    m_raw_data.resize(fs::file_size(file_path) - GZIP_START_OFF);
                    in_file.read(m_raw_data.data(), m_raw_data.size());
                    m_file.str(m_raw_data);
                    m_file_type = EFileType::Uncompressed;
                }
                else
                {
                    // TODO: handle other file types
                    return false;
                }
                m_file_path = file_path;
	            return true;
	        }
	        spdlog::error("Could not open file for reading");
        }
    }
    else
    {
        spdlog::warn("File '{}' doesn't exist", file_path.string());
    }
    return false;
}

const std::any& BinaryFileParser::get_record(int32_t id)
{
    static std::any dummy;
    for (const auto& rec : m_records)
    {
        if (rec.first == id)
        {
            return rec.second;
        }
    }
    return dummy;
}


bool BinaryFileParser::check_header()
{
    bool ret = true;
    auto curr_off = m_file.tellg();
    m_file.seekg(0);
    ufe::ERecordType rec = get_record_type();
    // default is partially parsed file if at least header is ok
    m_status = EFileStatus::PartialRead;
    if (rec == ufe::ERecordType::SerializedStreamHeader)
    {
        ufe::SerializationHeaderRecord header;
        read(header);
        if (header.RootId.value != 1 ||
            header.HeaderId.value != -1 ||
            header.MajorVersion.value != 1 ||
            header.MinorVersion.value != 0)
        {
            spdlog::debug("Header doesn't match, abort reading file");
            m_status = EFileStatus::Invalid;
            ret = false;
        }
    }
    else
    {
        spdlog::debug("Header record type invalid, abort reading file");
        m_status = EFileStatus::Invalid;
        ret = false;
    }
    m_file.seekg(curr_off);
    return ret;
}

void BinaryFileParser::read_records()
{
    if (check_header()) // skip parsing file if it has invalid header
    {
	    for (std::any a = read_record(); a.has_value(); a = read_record())
	    {
            m_root_records.emplace_back(a);
	    }
    }
    else
    {
        m_status = EFileStatus::Invalid;
    }
}

std::any BinaryFileParser::read_record()
{
    ufe::ERecordType rec = get_record_type();
    spdlog::debug("Parsing record type: {}", ufe::ERecordType2str(rec));
    auto record_type_not_implemented = [this](ufe::ERecordType rec)
        {
            spdlog::debug("Record type {} ({:x}) not implemented!", ufe::ERecordType2str(rec), static_cast<uint8_t>(rec));
            spdlog::debug("filepos: {}", static_cast<uint64_t>(m_file.tellg()));
        };

    switch (rec)
    {
        case ufe::ERecordType::SerializedStreamHeader:
            return get_SerializedStreamHeader();

        case ufe::ERecordType::ClassWithId:
            return get_ClassWithId();

        case ufe::ERecordType::SystemClassWithMembers:
            record_type_not_implemented(rec);
            break;

        case ufe::ERecordType::ClassWithMembers:
            record_type_not_implemented(rec);
            break;

        case ufe::ERecordType::SystemClassWithMembersAndTypes:
            return get_SystemClassWithMembersAndTypes();

        case ufe::ERecordType::ClassWithMembersAndTypes:
            return get_ClassWithMembersAndTypes();

        case ufe::ERecordType::BinaryObjectString:
            return get_BinaryObjectString();

        case ufe::ERecordType::BinaryArray:
            return get_BinaryArray();

        case ufe::ERecordType::MemberPrimitiveTyped:
            record_type_not_implemented(rec);
            break;

        case ufe::ERecordType::MemberReference:
            return get_MemberReference();

        case ufe::ERecordType::ObjectNull:
            return std::any{ufe::ObjectNull{}};

        case ufe::ERecordType::MessageEnd:
        {
            m_status = EFileStatus::FullRead;
            spdlog::info("File parsed successfully");
        } break;

        case ufe::ERecordType::BinaryLibrary:
            return get_BinaryLibrary();

        case ufe::ERecordType::ObjectNullMultiple256:
            return get_ObjectNullMultiple256();

        case ufe::ERecordType::ObjectNullMultiple:
            record_type_not_implemented(rec);
            break;

        case ufe::ERecordType::ArraySinglePrimitive:
            return get_ArraySinglePrimitive();

        case ufe::ERecordType::ArraySingleObject:
            record_type_not_implemented(rec);
            break;
        case ufe::ERecordType::ArraySingleString:
            return get_ArraySingleString();

        case ufe::ERecordType::MethodCall:   [[fallthrough]];
        case ufe::ERecordType::MethodReturn: [[fallthrough]];
        case ufe::ERecordType::InvalidType:  [[fallthrough]];
        default:
            record_type_not_implemented(rec);
            break;
    }
    return {};
}

std::any BinaryFileParser::get_ArraySingleString()
{
    ufe::ArraySingleString arr;
    read(arr);
    spdlog::debug("array_single_string  id {}, elements count {}", arr.ObjectId, arr.Length);
    for (int i = 0; i < arr.Length; ++i)
    {
        auto record = read_record();
        if (record.type() == std::type_index(typeid(ufe::ObjectNullMultiple256)))
        {
            // increase elements count if is a packed null object
            const auto obj = std::any_cast<ufe::ObjectNullMultiple256>(record);
            i += obj.NullCount;
        }
        arr.Data.push_back(record);
    }
    return arr;
}

std::any BinaryFileParser::get_ArraySinglePrimitive()
{
    ufe::ArraySinglePrimitive arr;
    read(arr);
    for (int i = 0; i < arr.Length; ++i)
    {
        nlohmann::ordered_json tmp;
        auto x = arr.PrimitiveTypeEnum;
        read_primitive_element(x);
    }
    return arr;
}

std::any BinaryFileParser::get_ObjectNullMultiple256()
{
    ufe::ObjectNullMultiple256 obj;
    obj.NullCount = read<uint8_t>();
    spdlog::debug("null_multiple_256 count: {}", obj.NullCount);
    return obj;
}

std::any BinaryFileParser::get_BinaryLibrary()
{
    ufe::BinaryLibrary bl;
    read(bl);
    add_record(bl.LibraryId.value, std::any{ bl });
    return bl;
}

std::any BinaryFileParser::get_MemberReference()
{
    ufe::MemberReference ref;
    read(ref);
    spdlog::debug("reference id: {}", ref.m_idRef);
    return ref;
}

std::any BinaryFileParser::get_BinaryArray()
{
    ufe::BinaryArray ba;
    read(ba);
    if (ba.Rank > 1)
    {
        spdlog::error("multidimensional array");
        return {};
    }
    spdlog::debug("binary array id {}, elements type '{}'", ba.ObjectId, ufe::EBinaryTypeEnumeration2str(ba.TypeEnum));
    for (int i = 0; i < ba.Lengths[0]; ++i)
    {
        switch (ba.TypeEnum)
        {
        case ufe::EBinaryTypeEnumeration::Primitive:
            ba.Data.emplace_back(read_primitive_element(std::get<ufe::EPrimitiveTypeEnumeration>(ba.AdditionalTypeInfo)));
            break;
        case ufe::EBinaryTypeEnumeration::String:
            break;
        case ufe::EBinaryTypeEnumeration::Object:
            break;
        case ufe::EBinaryTypeEnumeration::SystemClass:
            break;
        case ufe::EBinaryTypeEnumeration::Class:
        {
            ba.Data.emplace_back(read_record());
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
    return ba;
}

std::any BinaryFileParser::get_BinaryObjectString()
{
    ufe::BinaryObjectString bos;
    read(bos);
    spdlog::debug("object string id: {}, value: '{}'", bos.m_ObjectId, bos.m_Value.value.string);
    add_record(bos.m_ObjectId, std::any{ bos });
    return bos;
}

std::any BinaryFileParser::get_ClassWithMembersAndTypes()
{
    ufe::ClassWithMembersAndTypes cmt;
    read(cmt);
    add_record(cmt.m_ClassInfo.ObjectId.value, std::any{ cmt });
    return cmt;
}

std::any BinaryFileParser::get_SystemClassWithMembersAndTypes()
{
    ufe::ClassWithMembersAndTypes cmt;
    read(cmt, true);
    add_record(cmt.m_ClassInfo.ObjectId.value, std::any{ cmt });
    return cmt;
}

std::any BinaryFileParser::get_ClassWithId()
{
    ufe::ClassWithId cwi;
    read(cwi);
    return cwi;
}

std::any BinaryFileParser::get_SerializedStreamHeader()
{
    ufe::SerializationHeaderRecord rec;
    read(rec);
    return rec;
}

std::any BinaryFileParser::read_primitive_element(ufe::EPrimitiveTypeEnumeration type)
{
    switch (type)
    {
        case ufe::EPrimitiveTypeEnumeration::Boolean:
            return read_primitive<bool>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Byte:
            return read_primitive<char>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Char:
            return read_primitive<char>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Decimal:
            break;
        case ufe::EPrimitiveTypeEnumeration::Double:
            return read_primitive<double>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Int16:
            return read_primitive<int16_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Int32:
            return read_primitive<int32_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Int64:
            return read_primitive<int64_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::SByte:
            break;
        case ufe::EPrimitiveTypeEnumeration::Single:
            // special case
            return read_primitive<float>();
            break;
        case ufe::EPrimitiveTypeEnumeration::TimeSpan:
            return read_primitive<uint64_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::DateTime:
            return read_primitive<uint64_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::UInt16:
            return read_primitive<uint16_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::UInt32:
            return read_primitive<uint32_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::UInt64:
            return read_primitive<uint64_t>();
            break;
        case ufe::EPrimitiveTypeEnumeration::Null:
            break;
        case ufe::EPrimitiveTypeEnumeration::String:
            break;
        default:
            break;
    }
    spdlog::warn("{} primitive type not implemented", ufe::EPrimitiveTypeEnumeration2str(type));
    return {};
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
    ci.MemberNames.resize(ci.MemberCount.value);
    std::for_each(ci.MemberNames.begin(), ci.MemberNames.end(),
        [this](auto& data)
        {
            read(data);
        });
    return false;
}

bool BinaryFileParser::read(ufe::ClassWithMembersAndTypes& cmt, bool system_class /* = false */)
{
    read(cmt.m_ClassInfo);
    auto& mti = cmt.m_MemberTypeInfo;

    for (int i = 0; i < cmt.m_ClassInfo.MemberCount.value; ++i)
    {
        mti.BinaryTypeEnums.push_back(static_cast<ufe::EBinaryTypeEnumeration>(read()));
    }
    spdlog::debug("class name: {}", cmt.m_ClassInfo.Name.value.string);
    spdlog::debug("class id: {}", cmt.m_ClassInfo.ObjectId.value);
    spdlog::debug("members count: {}", cmt.m_ClassInfo.MemberCount.value);
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
    read_members_data(mti, cmt.m_ClassInfo);
    return false;
}


bool BinaryFileParser::read(ufe::LengthPrefixedString& lps)
{
    uint32_t len = 0;
    lps.m_original_len = 0;
    lps.m_original_len_unmod = 0;
    for (int i = 0; i < 5; ++i)
    {
        uint8_t seg = read();
        lps.m_original_len_unmod |= static_cast<uint64_t>(seg) << (8 * i);
        lps.m_original_len |= static_cast<uint64_t>(seg) << (8 * i);
        len |= static_cast<uint32_t>(seg & 0x7F) << (7 * i);
        if ((seg & 0x80) == 0x00) break;
    }
    lps.m_original_len = len;
    lps.string.resize(len);
    m_file.read(lps.string.data(), len);
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

bool BinaryFileParser::read(ufe::ClassWithId& cmt)
{
    IndexedData<int32_t> obj_id;

    read(obj_id);
    read(cmt.MetadataId);
    const auto& ref_class = get_record(cmt.MetadataId.value);
    if (ref_class.has_value())
    {
        const auto& ref = std::any_cast<const ufe::ClassWithMembersAndTypes&>(ref_class);
        cmt.m_ClassInfo = ref.m_ClassInfo;
        cmt.m_MemberTypeInfo = ref.m_MemberTypeInfo;
        // clear data from copied reference
        cmt.m_MemberTypeInfo.Data.clear();
        cmt.m_ClassInfo.ObjectId = obj_id;
        read_members_data(cmt.m_MemberTypeInfo, cmt.m_ClassInfo);
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

void BinaryFileParser::read_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci)
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
                        process_member<bool>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Byte:
                    {
                        process_member<uint8_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Char:
                    {
                        process_member<char>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Decimal:
                        throw std::exception("Not implemented!!!");
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Double:
                    {
                        process_member<double>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int16:
                    {
                        process_member<int16_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int32:
                    {
                        process_member<int32_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::Int64:
                    {
                        process_member<int64_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::SByte:
                        throw std::exception("Not implemented!!!");
                        break;
                    case ufe::EPrimitiveTypeEnumeration::Single:
                    {
                        process_member<float>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::TimeSpan:
                    {
                        process_member<int64_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::DateTime:
                    {
                        process_member<int64_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt16:
                    {
                        process_member<uint16_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt32:
                    {
                        process_member<uint32_t>(it_member_names, mti);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::UInt64:
                    {
                        process_member<uint64_t>(it_member_names, mti);
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
                mti.Data.push_back(bstr);
            } break;
            case ufe::EBinaryTypeEnumeration::Object:
            {
                auto rec = read_record();
                mti.Data.push_back(rec);
            } break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                auto rec = read_record();
                mti.Data.push_back(rec);
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                auto nested_data = read_record();
                mti.Data.push_back(nested_data);
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::ObjectArray:
            case ufe::EBinaryTypeEnumeration::StringArray:
            case ufe::EBinaryTypeEnumeration::PrimitiveArray:
            case ufe::EBinaryTypeEnumeration::None:
            {
                auto record = read_record();
                mti.Data.push_back(record);
            } break;
            default:
                break;
        }
        ++it_member_names;
    }
}

std::vector<char> BinaryFileParser::raw_data() const
{
    return std::vector<char>(m_raw_data.begin(), m_raw_data.end());
}

template<>
bool BinaryFileParser::read<ufe::LengthPrefixedString>(IndexedData<ufe::LengthPrefixedString>& data)
{
    data.offset = m_file.tellg();
    read(data.value);
    return true;
}

