#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <system_error>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <variant>
#include <source_location>
#include <iostream>


namespace ufe
{
    namespace fs = std::filesystem;
    void log_line(const std::string_view message,
        const std::source_location location =
        std::source_location::current());

	enum class ERecordType : uint8_t
	{
        SerializedStreamHeader = 0,
        ClassWithId = 1,
        SystemClassWithMembers = 2,
        ClassWithMembers = 3,
        SystemClassWithMembersAndTypes = 4,
        ClassWithMembersAndTypes = 5,
        BinaryObjectString = 6,
        BinaryArray = 7,
        MemberPrimitiveTyped = 8,
        MemberReference = 9,
        ObjectNull = 10,
        MessageEnd = 11,
        BinaryLibrary = 12,
        ObjectNullMultiple256 = 13,
        ObjectNullMultiple = 14,
        ArraySinglePrimitive = 15,
        ArraySingleObject = 16,
        ArraySingleString = 17,
        MethodCall = 21,
        MethodReturn = 22,
        InvalidType = 255
    };

    enum class EBinaryTypeEnumeration : uint8_t
    {
        Primitive = 0,
        String = 1,
        Object = 2,
        SystemClass = 3,
        Class = 4,
        ObjectArray = 5,
        StringArray = 6,
        PrimitiveArray = 7,
        None
    };

    enum class EPrimitiveTypeEnumeration
    {
        Boolean = 1,
        Byte = 2,
        Char = 3,
        Decimal = 5,
        Double = 6,
        Int16 = 7,
        Int32 = 8,
        Int64 = 9,
        SByte = 10,
        Single = 11,
        TimeSpan = 12,
        DateTime = 13,
        UInt16 = 14,
        UInt32 = 15,
        UInt64 = 16,
        Null = 17,
        String = 18
    };
    using PrimitiveData = std::variant<uint8_t, int32_t, double, float, bool>;

    class TypeRecord
    {
    public:
        ERecordType type() { return m_type; }
        template<class Archive>
        void load(Archive& archive)
        {
            archive(m_type);
        }
    private:
        ERecordType m_type;
    };

    struct LengthPrefixedString
    {
        std::string m_str;
        uint64_t m_original_len;
        template <class Archive>
        void load(Archive& archive)
        {
            uint32_t len = 0;
            m_original_len = 0;
            for (int i = 0; i < 5; ++i)
            {
                uint8_t seg;
                archive(seg);
                m_original_len |= static_cast<uint64_t>(seg) << (8 * i);
                len |= static_cast<uint32_t>(seg & 0x7F) << (7 * i);
                if ((seg & 0x80) == 0x00) break;
            }
            m_str.resize(len);
            archive(m_str);
        }
    };

    struct ClassInfo
    {
        int32_t ObjectId;
        LengthPrefixedString Name;
        int32_t MemberCount;
        std::vector<LengthPrefixedString> MemberNames;
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(ObjectId, Name, MemberCount);
            MemberNames.resize(MemberCount);
            archive(MemberNames);
        }
    };

    struct ClassTypeInfo
    {
        LengthPrefixedString TypeName;
        int32_t LibraryId;
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(TypeName, LibraryId);
        }
    };

    struct BinaryObjectString
    {
        int32_t m_ObjectId;
        LengthPrefixedString m_Value;
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(m_ObjectId, m_Value);
        }
    };

    struct MemberReference
    {
        int32_t m_idRef;
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(m_idRef);
        }
    };    

    struct ObjectNull
    {
        template<class Archive>
        void serialize(Archive& archive)
        {
        }
    };
    class ClassWithMembersAndTypes;
    using AdditionalInfosType = std::variant<EPrimitiveTypeEnumeration, LengthPrefixedString, ClassTypeInfo>;
    using ClassMembersData = std::variant<uint8_t, int32_t, double, float, bool, LengthPrefixedString, ClassTypeInfo, BinaryObjectString, ClassWithMembersAndTypes, MemberReference, ObjectNull>;

    struct MemberTypeInfo
    {
        std::vector<EBinaryTypeEnumeration> BinaryTypeEnums;
        std::vector<AdditionalInfosType> AdditionalInfos;
        int32_t LibraryId;
        std::vector<ClassMembersData> Data;
        void reserve(int size)
        {
            // filled by cereal
            BinaryTypeEnums.resize(size);
            // manual fill
            AdditionalInfos.reserve(size);
            Data.reserve(size);
        }
        template<class Archive>
        void load(Archive& archive)
        {
            archive(BinaryTypeEnums);
            uint8_t prim_type;
            for (auto type : BinaryTypeEnums)
            {
                switch (type)
                {
                    case ufe::EBinaryTypeEnumeration::Primitive:
                    {
                        archive(prim_type);
                        AdditionalInfos.push_back(static_cast<EPrimitiveTypeEnumeration>(prim_type));
                    } break;
                    case ufe::EBinaryTypeEnumeration::String:
                        break;
                    case ufe::EBinaryTypeEnumeration::Object:
                        break;
                    case ufe::EBinaryTypeEnumeration::SystemClass:
                    {
                        LengthPrefixedString str;
                        archive(str);
                        AdditionalInfos.push_back(str);
                    } break;
                    case ufe::EBinaryTypeEnumeration::Class:
                    {
                        ClassTypeInfo cti;
                        archive(cti);
                        AdditionalInfos.push_back(cti);
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

            archive(LibraryId);

            auto it_add_info = AdditionalInfos.cbegin();

            for (auto type : BinaryTypeEnums)
            {
                switch (type)
                {
                    case ufe::EBinaryTypeEnumeration::Primitive:
                    {                        
                        switch (std::get<EPrimitiveTypeEnumeration>(*it_add_info))
                        {
                            case ufe::EPrimitiveTypeEnumeration::Boolean:
                            {
                                bool tmp;
                                archive(tmp);
                                Data.push_back(tmp);
                            } break;
                            case ufe::EPrimitiveTypeEnumeration::Byte:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::Char:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::Decimal:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::Double:
                            {
                                double tmp;
                                archive(tmp);
                                Data.push_back(tmp);
                            } break;
                            case ufe::EPrimitiveTypeEnumeration::Int16:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::Int32:
                            {
                                int32_t tmp;
                                archive(tmp);
                                Data.push_back(tmp);
                            } break;
                            case ufe::EPrimitiveTypeEnumeration::Int64:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::SByte:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::Single:
                            {
                                float tmp;
                                archive(tmp);
                                Data.push_back(tmp);
                            } break;
                            case ufe::EPrimitiveTypeEnumeration::TimeSpan:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::DateTime:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::UInt16:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::UInt32:
                                break;
                            case ufe::EPrimitiveTypeEnumeration::UInt64:
                                break;
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
                        TypeRecord tr;
                        archive(tr);
                        BinaryObjectString bos;
                        archive(bos);
                        Data.push_back(bos);
                    } break;
                    case ufe::EBinaryTypeEnumeration::Object:
                    {
                        TypeRecord tr;
                        archive(tr);
                        switch (tr.type())
                        {
                            case ERecordType::MemberReference:
                            {
                                MemberReference mref;
                                archive(mref);
                                Data.push_back(mref);
                            } break;
                            case ERecordType::ObjectNull:
                            {
                                Data.push_back(ObjectNull{});
                            } break;
                            default:
                                log_line("Object type skipped");
                        }
                    } break;
                    case ufe::EBinaryTypeEnumeration::SystemClass:
                    {
                        ++it_add_info;
                    } break;
                    case ufe::EBinaryTypeEnumeration::Class:
                    {
                        TypeRecord tr;
                        archive(tr);
                        switch (tr.type())
                        {
                            case ERecordType::ClassWithMembersAndTypes:
                            {
                                ClassWithMembersAndTypes cls;
                                archive(cls);
                                Data.push_back(cls);
                            } break;
                            case ERecordType::MemberReference:
                            {
                                MemberReference mref;
                                archive(mref);
                                Data.push_back(mref);
                            } break;
                            default:
                                break;
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
            }
        }
    };


    class SerializationHeaderRecord
    {
    public:
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(RootId, HeaderId, MajorVersion, MinorVersion);
        }
    private:
        int32_t RootId;
        int32_t HeaderId;
        int32_t MajorVersion;
        int32_t MinorVersion;
    };

    class BinaryLibrary
    {
    public:
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(LibraryId, LibraryName);
        }
    private:
        int32_t LibraryId;
        LengthPrefixedString LibraryName;
    };

    class ClassWithMembersAndTypes
    {
    public:
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(m_ClassInfo);
            m_MemberTypeInfo.reserve(m_ClassInfo.MemberCount);
            archive(m_MemberTypeInfo);
        }
    private:
        ClassInfo m_ClassInfo;
        MemberTypeInfo m_MemberTypeInfo;
    };

    class ClassWithId
    {
    public:
    private:
        ClassInfo m_ClassInfo;
        MemberTypeInfo m_MemberTypeInfo;
    };
}
