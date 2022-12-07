#include "Records.hpp"

void ufe::log_line(const std::string_view message, const std::source_location location /*= std::source_location::current()*/)
{
    std::cout << "file: "
        << location.file_name() << "("
        << location.line() << ":"
        << location.column() << ") `"
        << location.function_name() << "`: "
        << message << '\n';
}

std::string_view ufe::ERecordType2str(ERecordType rec)
{
    switch (rec)
    {
        case ERecordType::SerializedStreamHeader: return "SerializedStreamHeader";
        case ERecordType::ClassWithId: return "ClassWithId";
        case ERecordType::SystemClassWithMembers: return "SystemClassWithMembers";
        case ERecordType::ClassWithMembers: return "ClassWithMembers";
        case ERecordType::SystemClassWithMembersAndTypes: return "SystemClassWithMembersAndTypes";
        case ERecordType::ClassWithMembersAndTypes: return "ClassWithMembersAndTypes";
        case ERecordType::BinaryObjectString: return "BinaryObjectString";
        case ERecordType::BinaryArray: return "BinaryArray";
        case ERecordType::MemberPrimitiveTyped: return "MemberPrimitiveTyped";
        case ERecordType::MemberReference: return "MemberReference";
        case ERecordType::ObjectNull: return "ObjectNull";
        case ERecordType::MessageEnd: return "MessageEnd";
        case ERecordType::BinaryLibrary: return "BinaryLibrary";
        case ERecordType::ObjectNullMultiple256: return "ObjectNullMultiple256";
        case ERecordType::ObjectNullMultiple: return "ObjectNullMultiple";
        case ERecordType::ArraySinglePrimitive: return "ArraySinglePrimitive";
        case ERecordType::ArraySingleObject: return "ArraySingleObject";
        case ERecordType::ArraySingleString: return "ArraySingleString";
        case ERecordType::MethodCall: return "MethodCall";
        case ERecordType::MethodReturn: return "MethodReturn";
        case ERecordType::InvalidType: return "InvalidType";
        default:
            return "InvalidType";
    }
}
