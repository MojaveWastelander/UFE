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


std::string_view ufe::EPrimitiveTypeEnumeration2str(EPrimitiveTypeEnumeration rec)
{
    switch (rec)
    {
        case EPrimitiveTypeEnumeration::Boolean: return "Boolean";
        case EPrimitiveTypeEnumeration::Byte: return "Byte";
        case EPrimitiveTypeEnumeration::Char: return "Char";
        case EPrimitiveTypeEnumeration::Decimal: return "Decimal";
        case EPrimitiveTypeEnumeration::Double: return "Double";
        case EPrimitiveTypeEnumeration::Int16: return "Int16";
        case EPrimitiveTypeEnumeration::Int32: return "Int32";
        case EPrimitiveTypeEnumeration::Int64: return "Int64";
        case EPrimitiveTypeEnumeration::SByte: return "SByte";
        case EPrimitiveTypeEnumeration::Single: return "Single";
        case EPrimitiveTypeEnumeration::TimeSpan: return "TimeSpan";
        case EPrimitiveTypeEnumeration::DateTime: return "DateTime";
        case EPrimitiveTypeEnumeration::UInt16: return "UInt16";
        case EPrimitiveTypeEnumeration::UInt32: return "UInt32";
        case EPrimitiveTypeEnumeration::UInt64: return "UInt64";
        case EPrimitiveTypeEnumeration::Null: return "Null";
        case EPrimitiveTypeEnumeration::String: return "String";
        default:
            return "InvalidType";
    }

}

bool ufe::operator==(const LengthPrefixedString& lhs, const std::string& rhs)
{
    return lhs.m_str == rhs;
}

std::string_view ufe::EBinaryTypeEnumeration2str(EBinaryTypeEnumeration rec)
{
    switch (rec)
    {
        case EBinaryTypeEnumeration::Primitive: return "Primitive";
        case EBinaryTypeEnumeration::String: return "String";
        case EBinaryTypeEnumeration::Object: return "Object";
        case EBinaryTypeEnumeration::SystemClass: return "SystemClass";
        case EBinaryTypeEnumeration::Class: return "Class";
        case EBinaryTypeEnumeration::ObjectArray: return "ObjectArray";
        case EBinaryTypeEnumeration::StringArray: return "StringArray";
        case EBinaryTypeEnumeration::PrimitiveArray: return "PrimitiveArray";
        case EBinaryTypeEnumeration::None: return "None";
        default:
            return "InvalidType";
    }
}
