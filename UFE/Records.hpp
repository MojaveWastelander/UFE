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

namespace ufe
{
    namespace fs = std::filesystem;

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

    namespace special_types
    {

        struct LengthPrefixedString
        {
            std::string m_str;
            uint64_t m_original_len;
        };

        struct ClassInfo
        {
            int32_t ObjectId = 0;
            LengthPrefixedString Name;
            int32_t MemberCount;
            std::vector<LengthPrefixedString> Members;
        };

        struct ClassTypeInfo
        {
            LengthPrefixedString TypeName;
            int32_t LibraryId;
        };

        using AdditionalInfosType = std::variant<EPrimitiveTypeEnumeration, LengthPrefixedString, ClassTypeInfo>;
        using ClassMembersData = std::variant<uint8_t, int32_t, double, float, LengthPrefixedString, ClassTypeInfo>;
        using PrimitiveData = std::variant<uint8_t, int32_t, double, float, bool>;

        struct MemberTypeInfo
        {
            std::vector<EBinaryTypeEnumeration> BinaryTypeEnums;
            std::vector<AdditionalInfosType> AdditionalInfos;
        };
    }

    class FileBuf
    {
    public:
        FileBuf() = default;
        FileBuf(const fs::path& file_path)
        {
            read_file(file_path);
        }

        std::error_condition read_file(const fs::path& file_path)
        {
            if (fs::exists(file_path) && fs::is_regular_file(file_path))
            {
                std::ifstream in{ file_path, std::ios_base::binary };
                m_buff.resize(fs::file_size(file_path));
                in.read(reinterpret_cast<char*>(m_buff.data()), m_buff.size());
                m_cur_pos = 0;
            }
            else
            {
                return std::make_error_condition(std::errc::no_such_file_or_directory);
            }
            return std::error_condition{};
        }

        template <typename T>
        T read()
        {
            return read_impl<T>();
        }

    private:
        template <typename T>
        T read_impl() 
        {
            //static_assert(false, "Not implemented");
            return T{};
        }

        template <>
        uint8_t read_impl<uint8_t>() 
        {
            return m_buff[m_cur_pos++];
        }

        template <>
        int32_t read_impl<int32_t>()
        {
            int32_t val = 0;
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]);
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]) << 8;
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]) << 16;
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]) << 24;
            return val;
        }

        template <>
        special_types::LengthPrefixedString read_impl<special_types::LengthPrefixedString>()
        {
            special_types::LengthPrefixedString str;
            uint32_t len = 0;
            str.m_original_len = 0;
            for (int i = 0; i < 5; ++i)
            {
                uint8_t seg = read<uint8_t>();
                str.m_original_len |= static_cast<uint64_t>(seg) << (8 * i);
                len |= static_cast<uint32_t>(seg & 0x7F) << (7 * i);
                if ((seg & 0x80) == 0x00) break;
            }

            str.m_str.resize(len);
            std::generate_n(str.m_str.begin(), len,
                [this]()
                {
                    return read<uint8_t>();
                });
            return str;
        }

        template<>
        special_types::ClassInfo read_impl<special_types::ClassInfo>()
        {
            special_types::ClassInfo ci;
            ci.ObjectId = read<int32_t>();
            ci.Name = read<special_types::LengthPrefixedString>();
            ci.MemberCount = read<int32_t>();
            ci.Members.reserve(ci.MemberCount);
            for (int i = 0; i < ci.MemberCount; ++i)
            {
                ci.Members.emplace_back(read<special_types::LengthPrefixedString>());
            }
            return ci;
        }

        template<>
        special_types::ClassTypeInfo read_impl<special_types::ClassTypeInfo>()
        {
            special_types::ClassTypeInfo cti;
            cti.TypeName = read<special_types::LengthPrefixedString>();
            cti.LibraryId = read<int32_t>();
            return cti;
        }

        std::vector<uint8_t> m_buff;
        size_t m_cur_pos;
    };


    struct IRecord
    {
        virtual void read(FileBuf& fb) = 0;
        virtual void write(FileBuf& fb) = 0;
    };

    template <class T>
    struct WrappingRecord : IRecord
    {
        T m_record;
        explicit WrappingRecord(T&& record) : m_record{ std::move(record) } {}

        void read(FileBuf& fb) override { m_record.read(fb); }
        void write(FileBuf& fb) override { m_record.write(fb); }
    };

    struct Record
    {
        std::shared_ptr<IRecord> m_rec_ptr;

        template <class T>
        Record(T &&t)
        {
            m_rec_ptr = std::make_shared<WrappingRecord<T>>(std::move(t));
        }

        void read(FileBuf& fb) { m_rec_ptr->read(fb);  }
        void write(FileBuf& fb) { m_rec_ptr->write(fb); }
    };

    namespace records
    {
	    struct RecordBase
	    {
	    protected:
	        RecordBase(ERecordType type) :
	            m_type{ type } {}
	        ERecordType m_type;
	    };
	    
	    struct SerializationHeaderRecord : RecordBase
	    {
	        SerializationHeaderRecord() :
	            RecordBase{ ERecordType::SerializedStreamHeader } {}
	        void read(FileBuf& fb) 
            {
                RootId = fb.read<int32_t>();
                HeaderId = fb.read<int32_t>();
                MajorVersion = fb.read<int32_t>();
                MinorVersion = fb.read<int32_t>();
            }

	        void write(FileBuf& fb) {}

            int32_t RootId = 0;
            int32_t HeaderId = 0;
            int32_t MajorVersion = 0;
            int32_t MinorVersion = 0;
	    };
	
	    struct BinaryLibrary : RecordBase
	    {
	        BinaryLibrary() :
	            RecordBase{ ERecordType::BinaryLibrary } {}
	        void read(FileBuf& fb) 
            {
                LibraryId = fb.read<int32_t>();
                LibraryName = fb.read<special_types::LengthPrefixedString>();
            }

	        void write(FileBuf& fb) {}

            uint32_t LibraryId = 0;
            special_types::LengthPrefixedString LibraryName{};

	    };

        struct ClassWithId : RecordBase
        {
            ClassWithId() :
                RecordBase{ ERecordType::ClassWithId } {}
            void read(FileBuf& fb) 
            {
                ObjectId = fb.read<int32_t>();
                MetadataId = fb.read<int32_t>();
            }
            void write(FileBuf& fb) {}

            int32_t ObjectId = 0;
            int32_t MetadataId = 0;
        };

        struct SystemClassWithMembers : RecordBase
        {
            SystemClassWithMembers() :
                RecordBase{ ERecordType::SystemClassWithMembers } {}
            void read(FileBuf& fb) {}
            void write(FileBuf& fb) {}
        };

        struct ClassWithMembers : RecordBase
        {
            ClassWithMembers() :
                RecordBase{ ERecordType::ClassWithMembers } {}
            void read(FileBuf& fb) {}
            void write(FileBuf& fb) {}
        };

        struct SystemClassWithMembersAndTypes : RecordBase
        {
            SystemClassWithMembersAndTypes() :
                RecordBase{ ERecordType::SystemClassWithMembersAndTypes } {}
            void read(FileBuf& fb) {}
            void write(FileBuf& fb) {}
        };

        struct ClassWithMembersAndTypes : RecordBase
        {
            ClassWithMembersAndTypes() :
                RecordBase{ ERecordType::ClassWithMembersAndTypes } {}
            void read(FileBuf& fb) 
            {
                ClassInfo = fb.read<special_types::ClassInfo>();

                // read additional infos manually
                MemberTypeInfo.BinaryTypeEnums.resize(ClassInfo.MemberCount);
                MemberTypeInfo.AdditionalInfos.reserve(ClassInfo.MemberCount);
                std::generate_n(MemberTypeInfo.BinaryTypeEnums.begin(), ClassInfo.MemberCount,
                    [&fb]()
                    {
                        return static_cast<EBinaryTypeEnumeration>(fb.read<uint8_t>());
                    });

                for (const auto& type : MemberTypeInfo.BinaryTypeEnums)
                {
                    switch (type)
                    {
                        case EBinaryTypeEnumeration::Primitive:
                        case EBinaryTypeEnumeration::PrimitiveArray:
                        {
                            MemberTypeInfo.AdditionalInfos.emplace_back(special_types::AdditionalInfosType( static_cast<EPrimitiveTypeEnumeration>(fb.read<uint8_t>()) ));
                        } break;
                        case EBinaryTypeEnumeration::Class:
                        {
                            MemberTypeInfo.AdditionalInfos.emplace_back(special_types::AdditionalInfosType(fb.read<special_types::ClassTypeInfo>()));
                        } break;
                        case EBinaryTypeEnumeration::SystemClass:
                        {
                            MemberTypeInfo.AdditionalInfos.emplace_back(special_types::AdditionalInfosType(fb.read<special_types::LengthPrefixedString>()));
                        } break;
                        default:
                            break;
                    }
                }
                LibraryId = fb.read<uint32_t>();
                
                for (const auto& type : MemberTypeInfo.BinaryTypeEnums)
                {
                    switch (type)
                    {
                    case EBinaryTypeEnumeration::Primitive: 
                    }
                }
            }
            void write(FileBuf& fb) {}

            special_types::ClassInfo ClassInfo;
            special_types::MemberTypeInfo MemberTypeInfo;
            int32_t LibraryId;
            std::vector <std::unique_ptr<special_types::ClassMembersData>> MembersData;
        };

        struct BinaryObjectString : RecordBase
        {
            BinaryObjectString() :
                RecordBase{ ERecordType::BinaryObjectString } {}
            void read(FileBuf& fb) 
            {
                ObjectId = fb.read<int32_t>();
                Value = fb.read<special_types::LengthPrefixedString>();
            }
            void write(FileBuf& fb) {}
            int32_t ObjectId;
            special_types::LengthPrefixedString Value;
        };

        struct MemberReference : RecordBase
        {
            MemberReference() :
                RecordBase{ ERecordType::MemberReference } {}
            void read(FileBuf& fb) {}
            void write(FileBuf& fb) {}
        };

        struct ObjectNull : RecordBase
        {
            ObjectNull() :
                RecordBase{ ERecordType::ObjectNull } {}
            void read(FileBuf& fb) {}
            void write(FileBuf& fb) {}
        };

        std::unique_ptr<Record> create_record(ERecordType rec_type);
        ERecordType next_record_type(FileBuf& fb);
    }
}
