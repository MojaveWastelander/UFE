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
