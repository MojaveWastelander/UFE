#include "Records.hpp"



std::unique_ptr<ufe::Record> ufe::records::create_record(ERecordType rec_type)
{
    switch (rec_type)
    {
    case ERecordType::SerializedStreamHeader: return std::make_unique<Record>(SerializationHeaderRecord{});
        break;
    case ERecordType::ClassWithId: return std::make_unique<Record>(ClassWithId{});
        break;
    case ERecordType::ClassWithMembersAndTypes: return std::make_unique<Record>(ClassWithMembersAndTypes{});
        break;
    case ERecordType::BinaryLibrary: return std::make_unique<Record>(BinaryLibrary{});
        break;
    default:
        throw std::domain_error{std::format("Record type not supported: {}", static_cast<int>(rec_type)).c_str()};
    }
}

ufe::ERecordType ufe::records::next_record_type(FileBuf& fb)
{
    return static_cast<ERecordType>(fb.read<uint8_t>());
}
