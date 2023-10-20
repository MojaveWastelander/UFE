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
#include <string_view>
#include "IndexedData.hpp"
#include <nlohmann/json.hpp>


using ojson = nlohmann::ordered_json;
// json uses double internally and float->double casts are imprecise
// thus converting via strings
double float2str2double(float f);

constexpr uint8_t GZIP_MAGIC_1 = 0x1F;
constexpr uint8_t GZIP_MAGIC_2 = 0x8B;
constexpr uint8_t GZIP_START_OFF = 24;

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

    std::string_view ERecordType2str(ERecordType rec);

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
    std::string_view EBinaryTypeEnumeration2str(EBinaryTypeEnumeration rec);

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
    std::string_view EPrimitiveTypeEnumeration2str(EPrimitiveTypeEnumeration rec);

    enum class EBinaryArrayTypeEnumeration
    {
        Single,
        Jagged,
        Rectangular,
        SingleOffset,
        JaggedOffset,
        RectangularOffset
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
        std::string string;
        uint64_t m_original_len;
        uint64_t m_original_len_unmod;
        uint64_t m_new_len = 0;
        void update_string(const std::string& s)
        {
            if (string != s)
            {
                uint64_t tmp = s.size();
                for (int i = 0; tmp; tmp >>= 7, ++i)
                {
                    m_new_len |= (tmp & 0x7F) << (8 * i);
                    if (tmp >> 7)
                    {
                        m_new_len |= 0x80UL << (8 * i);
                    }
                }
                string = s;
            }
        }
    };

    bool operator==(const LengthPrefixedString& lhs, const std::string& rhs);

    struct ClassInfo
    {
        IndexedData<int32_t> ObjectId;
        IndexedData<LengthPrefixedString> Name;
        IndexedData<int32_t> MemberCount;
        std::vector<IndexedData<LengthPrefixedString>> MemberNames;
    };

    struct ClassTypeInfo
    {
        LengthPrefixedString TypeName;
        int32_t LibraryId;
    };

    struct BinaryObjectString
    {
        int32_t m_ObjectId;
        IndexedData<LengthPrefixedString> m_Value;
    };

    struct MemberReference
    {
        int32_t m_idRef;
    };    

    struct ObjectNull
    {
    };

    struct ObjectNullMultiple256
    {
        uint8_t NullCount;
    };
    struct ClassWithMembersAndTypes;
    using AdditionalInfosType = std::variant<EPrimitiveTypeEnumeration, LengthPrefixedString, ClassTypeInfo>;
    using ClassMembersData = std::variant<uint8_t, int32_t, double, float, bool, LengthPrefixedString, ClassTypeInfo, BinaryObjectString, ClassWithMembersAndTypes, MemberReference, ObjectNull>;

    struct BinaryArray
    {
        int32_t ObjectId;
        EBinaryArrayTypeEnumeration BinaryArrayTypeEnum;
        int32_t Rank;
        std::vector<int32_t> Lengths;
        std::vector<int32_t> LowerBounds;
        EBinaryTypeEnumeration TypeEnum;
        AdditionalInfosType AdditionalTypeInfo;
        std::vector<std::any> Data;
    };

    struct ArraySingleString
    {
        int32_t ObjectId;
        int32_t Length;
        std::vector<std::any> Data;
    };
    
    struct ArraySinglePrimitive
    {
        int32_t ObjectId;
        int32_t Length;
        EPrimitiveTypeEnumeration PrimitiveTypeEnum;
        std::vector<std::any> Data;
    };

    struct MemberTypeInfo
    {
        std::vector<EBinaryTypeEnumeration> BinaryTypeEnums;
        std::vector<AdditionalInfosType> AdditionalInfos;
        int32_t LibraryId;
        std::vector<std::any> Data;
        void reserve(int size)
        {
            BinaryTypeEnums.resize(size);
            // manual fill
            AdditionalInfos.reserve(size);
            Data.reserve(size);
        }
    };

    class FileReader;

    struct SerializationHeaderRecord
    {
        IndexedData<int32_t> RootId;
        IndexedData<int32_t> HeaderId;
        IndexedData<int32_t> MajorVersion;
        IndexedData<int32_t> MinorVersion;
    };

    struct BinaryLibrary
    {
        IndexedData<int32_t> LibraryId;
        IndexedData<LengthPrefixedString> LibraryName;
    };

    struct ClassWithMembersAndTypes
    {
        ClassInfo m_ClassInfo;
        MemberTypeInfo m_MemberTypeInfo;
    };

    struct ClassWithId
    {
        IndexedData<int32_t> MetadataId;
        ClassInfo m_ClassInfo;
        MemberTypeInfo m_MemberTypeInfo;
    };
}
