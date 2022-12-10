#include "FileWriter.hpp"
#include <algorithm>

void FileWriter::get_raw_data()
{
    m_raw_data = m_parser.raw_data();
}

bool FileWriter::read(ufe::ClassWithMembersAndTypes& cmt, const nlohmann::ordered_json& obj)
{
    m_parser.read(cmt.m_ClassInfo);
    auto& mti = cmt.m_MemberTypeInfo;

    for (int i = 0; i < cmt.m_ClassInfo.MemberCount.m_data; ++i)
    {
        mti.BinaryTypeEnums.push_back(static_cast<ufe::EBinaryTypeEnumeration>(m_parser.read()));
    }

    for (auto type : mti.BinaryTypeEnums)
    {
        switch (type)
        {
            case ufe::EBinaryTypeEnumeration::Primitive:
            {
                mti.AdditionalInfos.push_back(static_cast<ufe::EPrimitiveTypeEnumeration>(m_parser.read()));
            } break;
            case ufe::EBinaryTypeEnumeration::String:
                break;
            case ufe::EBinaryTypeEnumeration::Object:
                break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                ufe::LengthPrefixedString str;
                m_parser.read(str);
                mti.AdditionalInfos.push_back(str);
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                ufe::ClassTypeInfo cti;
                m_parser.read(cti);
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

    mti.LibraryId = m_parser.read<int32_t>();

    auto& class_members = find_class_members(cmt.m_ClassInfo.Name.m_data.m_str, obj);
    spdlog::trace(obj.dump());
    spdlog::debug(class_members.dump());
    update_members_data(mti, cmt.m_ClassInfo, class_members);
    return false;
}

bool FileWriter::read(ufe::ClassWithId& cmt, const nlohmann::ordered_json& obj)
{
    IndexedData<int32_t> obj_id;

    m_parser.read(obj_id);
    m_parser.read(cmt.MetadataId);
    const auto& ref_class = m_parser.get_record(cmt.MetadataId.m_data);
    if (ref_class.has_value())
    {
        const auto& ref = std::any_cast<const ufe::ClassWithMembersAndTypes&>(ref_class);
        cmt.m_ClassInfo = ref.m_ClassInfo;
        cmt.m_MemberTypeInfo = ref.m_MemberTypeInfo;
        cmt.m_ClassInfo.ObjectId = obj_id;
        auto& class_members = find_class_members(cmt.m_ClassInfo.Name.m_data.m_str, obj, cmt.MetadataId.m_data);
        spdlog::trace(obj.dump());
        spdlog::debug(class_members.dump());

        update_members_data(cmt.m_MemberTypeInfo, cmt.m_ClassInfo, class_members);
        return true;
    }
    spdlog::error("reference class {} not found", cmt.MetadataId.m_data);
    return false;
}

void FileWriter::update_members_data(ufe::MemberTypeInfo& mti, ufe::ClassInfo& ci, const nlohmann::ordered_json& obj)
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
                    {
                        process_member<char>(obj, it_member_names->m_data.m_str);
                    } break;
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
                    {
                        process_member<int64_t>(obj, it_member_names->m_data.m_str);
                    } break;
                    case ufe::EPrimitiveTypeEnumeration::DateTime:
                    {
                        process_member<int64_t>(obj, it_member_names->m_data.m_str);
                    } break;
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
                if (!m_updated_strings.empty() && obj.contains(it_member_names->m_data.m_str))
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
                if (json_elem(obj, it_member_names->m_data.m_str))
                {
                    mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
                }
                else
                {
                    mti.Data.push_back(read_record(m_empty));
                }
            } break;
            case ufe::EBinaryTypeEnumeration::SystemClass:
            {
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::Class:
            {
                if (json_elem(obj, it_member_names->m_data.m_str))
                {
                    mti.Data.push_back(read_record(obj[it_member_names->m_data.m_str]));
                }
                else
                {
                    mti.Data.push_back(read_record(m_empty));
                }
                ++it_add_info;
            } break;
            case ufe::EBinaryTypeEnumeration::ObjectArray:
            case ufe::EBinaryTypeEnumeration::StringArray:
            case ufe::EBinaryTypeEnumeration::PrimitiveArray:
            case ufe::EBinaryTypeEnumeration::None:
                read_record(obj);
                break;
            default:
                break;
        }
        ++it_member_names;
    }
}

const nlohmann::ordered_json& FileWriter::find_class_members(std::string name, const nlohmann::ordered_json& obj, int ref_id /*= 0*/)
{
    static nlohmann::ordered_json dummy;

    if (obj.is_null()) return dummy;

    // direct search for ref_id in current context
    if (ref_id)
    {
        if (obj.contains("class_id") && obj["class_id"]["ref_id"] == ref_id)
        {
            return obj["class_id"]["members"];
        }
    }

    // nested class not in array
    if (obj.contains("class"))
    {
        if (obj["class"]["name"] == name)
        {
            return obj["class"]["members"];
        }
    }

    for (auto& rec : m_json["records"])
    {
        if (ref_id)
        {
            if (rec.contains("class_id") && rec["class_id"]["ref_id"] == ref_id)
            {
                return rec["class_id"]["members"];
            }
        }
        else
        {
            if (rec.contains("class") && rec["class"]["name"] == name)
            {
                return rec["class"]["members"];
            }
        }
    }
    
    // couldn't find any reference
    if (ref_id)
    {
        spdlog::error("class_id ref {} not in current context", ref_id);
    }

    return dummy;
}

bool FileWriter::update_file(fs::path& binary_path, fs::path& json_path)
{
    nlohmann::ordered_json dummy;
    std::ifstream json_file{ json_path };
    if (json_file)
    {
        json_file >> m_json;
        
        if (m_parser.open(binary_path))
        {
            m_empty = {};
            m_updated_strings.reserve(200);
            get_raw_data();
            try
            {
                while (read_record(m_json["records"]))
                {
                }
            }
            catch (std::exception& e)
            {
                spdlog::critical("Failed to update file: {}", e.what());
                return false;
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

            m_parser.close();
            //std::ofstream out{ binary_path, std::ios::binary | std::ios::trunc };
            //out.write(m_raw_data.data(), m_raw_data.size());
        }
    }
    return false;
}
bool FileWriter::read_record(const nlohmann::ordered_json& obj)
{
    auto offset = m_parser.offset();
    ufe::ERecordType rec = m_parser.get_record_type();
    spdlog::debug("Parsing record: {} at {}", ufe::ERecordType2str(rec), offset);
    switch (rec)
    {
        case ufe::ERecordType::SerializedStreamHeader:
        {
            ufe::SerializationHeaderRecord rec;
            m_parser.read(rec);
        } break;
        case ufe::ERecordType::ClassWithId:
        {
            ufe::ClassWithId cwi;
            read(cwi, obj);
        } break;
        case ufe::ERecordType::SystemClassWithMembers:
            return false;
            break;
        case ufe::ERecordType::ClassWithMembers:
            return false;
            break;
        case ufe::ERecordType::SystemClassWithMembersAndTypes:
            return false;
            break;
        case ufe::ERecordType::ClassWithMembersAndTypes:
        {
            ufe::ClassWithMembersAndTypes cmt;
            read(cmt, obj);
            m_parser.add_record(cmt.m_ClassInfo.ObjectId.m_data, std::any{ cmt });
        } break;
        case ufe::ERecordType::BinaryObjectString:
        {
            ufe::BinaryObjectString bos;
            m_parser.read(bos);
            m_parser.add_record(bos.m_ObjectId, std::any{ bos });
            m_updated_strings.push_back(bos.m_Value);
        } break;
        case ufe::ERecordType::BinaryArray:
            return false;
            break;
        case ufe::ERecordType::MemberPrimitiveTyped:
            return false;
            break;
        case ufe::ERecordType::MemberReference:
        {
            ufe::MemberReference ref;
            m_parser.read(ref);
        } break;
        case ufe::ERecordType::ObjectNull:
        {
        } break;
        case ufe::ERecordType::MessageEnd: return false;
        case ufe::ERecordType::BinaryLibrary:
        {
            ufe::BinaryLibrary bl;
            m_parser.read(bl);
            m_parser.add_record(bl.LibraryId.m_data, std::any{ bl });
        } break;
        case ufe::ERecordType::ObjectNullMultiple256:
            return false;
            break;
        case ufe::ERecordType::ObjectNullMultiple:
            return false;
            break;
        case ufe::ERecordType::ArraySinglePrimitive:
            return false;
            break;
        case ufe::ERecordType::ArraySingleObject:
            return false;
            break;
        case ufe::ERecordType::ArraySingleString:
        {
            ufe::ArraySingleString arr;
            m_parser.read(arr);
            for (int i = 0; i < arr.Length; ++i)
            {
                auto record = m_parser.read_record();
                arr.Data.push_back(record.dyn_obj);
            }
        } break;
        case ufe::ERecordType::MethodCall:
            return false;
            break;
        case ufe::ERecordType::MethodReturn:
            return false;
            break;
        case ufe::ERecordType::InvalidType:
            return false;
            break;
        default:
            break;
    }
    return true;
}
