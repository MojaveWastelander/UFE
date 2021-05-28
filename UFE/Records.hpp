#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <system_error>
#include <fstream>
#include <filesystem>

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

    namespace special_types
    {
        struct LengthPrefixedString
        {
            std::string m_str;
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
            static_assert(false, "Not implemented");
            return T{};
        }

        template <>
        uint8_t read_impl() 
        {
            return m_buff[m_cur_pos++];
        }

        template <>
        int32_t read_impl() 
        {
            int32_t val = 0;
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]);
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]) << 8;
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]) << 16;
            val |= static_cast<uint32_t>(m_buff[m_cur_pos++]) << 24;
            return val;
        }

        template <>
        special_types::LengthPrefixedString read_impl()
        {
            special_types::LengthPrefixedString str;
            uint32_t len = 0;
            for (int i = 0; i < 5; ++i)
            {
                uint8_t seg = read<uint8_t>();
                len |= static_cast<uint32_t>(seg & 0x7F) << (7 * i);
                if ((seg & 0x80) == 0x00) break;
            }
        }

        std::vector<uint8_t> m_buff;
        size_t m_cur_pos;
    public:
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
	        void read(FileBuf& fb) {}
	        void write(FileBuf& fb) {}


	    };
    }
}
