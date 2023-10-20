#include <format>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
//#include <cereal/cereal.hpp>
//#include <cereal/archives/binary.hpp>
//#include <cereal/types/array.hpp>
//#include <cereal/types/vector.hpp>
//#include <cereal/types/string.hpp>
//#include <cereal/types/variant.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"
#include "BinaryFileParser.hpp"
#include "JsonWriter.hpp"
#include "JsonReader.hpp"
#include "CLIParser.hpp"
#include <windows.h>
#define WIN32_LEAN_AND_MEAN


bool skip_path(const std::filesystem::path& p)
{
    if (fs::is_directory(p) || (p.has_extension() && p.extension() == ".json"))
    {
        return true;
    }
    return false;
}

void parse_file(const fs::path& p, const CLIParser& cli)
{
    BinaryFileParser parser;
    auto file_status = [](BinaryFileParser::EFileStatus status) -> std::string_view
    {
        switch (status)
        {
            case BinaryFileParser::EFileStatus::Empty:
                return "empty";
            case BinaryFileParser::EFileStatus::PartialRead:
                return "partial read";
            case BinaryFileParser::EFileStatus::FullRead:
                return "validated";
            case BinaryFileParser::EFileStatus::Invalid:
            default:
                return "invalid";
        }
    };

    if (!skip_path(p) && parser.open(p))
    {
        parser.read_records();

        if (parser.status() != BinaryFileParser::EFileStatus::Invalid &&
            parser.status() != BinaryFileParser::EFileStatus::Empty)
        {
            //reader.export_json(json_path);
            if (parser.status() == BinaryFileParser::EFileStatus::PartialRead)
            {
                spdlog::warn("Partial file read!");
                //continue;
            }
            fs::path json_path = p;
            json_path += ".json";
            if (cli.export_mode())
            {
                JsonWriter writer;
                writer.save(json_path, parser.get_records());
            }

            if (cli.patch())
            {
                JsonReader reader;
                reader.patch(json_path, p, parser);
            }
        }

        if (cli.validate())
        {
            if (parser.status() != BinaryFileParser::EFileStatus::Invalid &&
                parser.status() != BinaryFileParser::EFileStatus::Empty)
            {
                if (parser.file_type() == BinaryFileParser::EFileType::Compressed)
                {
                    spdlog::info("File '{}' is compressed, validation status '{}'", p.string(), file_status(parser.status()));
                }
                else if (parser.file_type() == BinaryFileParser::EFileType::Uncompressed)
                {
                    spdlog::info("File '{}' is uncompressed, validation status '{}'", p.string(), file_status(parser.status()));
                }
                else
                {
                    spdlog::info("File '{}' is raw, validation status '{}'", p.string(), file_status(parser.status()));
                }
            }
            else
            {
                spdlog::trace("File '{}' not supported");
            }
        }
    }
}

void parse_directory(const CLIParser& cli)
{
    for (const auto& p : fs::recursive_directory_iterator{ cli.base_path() })
    {
        parse_file(p, cli);
    }
}

void parse(const CLIParser& cli)
{
    if (fs::is_regular_file(cli.base_path()))
    {
        parse_file(cli.base_path(), cli);
    }
    else if (fs::is_directory(cli.base_path()))
    {
        parse_directory(cli);
    }
    else
    {
        spdlog::error("Path '{}' cannot be parsed", cli.base_path().string());
    }
}


void validate_directory(fs::path dir)
{
    auto file_status = [](BinaryFileParser::EFileStatus status) -> std::string_view
    {
        switch (status)
        {
            case BinaryFileParser::EFileStatus::Empty:
                return "empty";
            case BinaryFileParser::EFileStatus::PartialRead:
                return "partial read";
            case BinaryFileParser::EFileStatus::FullRead:
                return "validated";
            case BinaryFileParser::EFileStatus::Invalid:
            default:
                return "invalid";
        }
    };

    for (const auto& p : fs::recursive_directory_iterator{ dir })
    {
        // skip json files
        if (skip_path(p))
        {
            continue;
        }

        BinaryFileParser parser;
        if (parser.open(p))
        {
            parser.read_records();

            if (parser.status() != BinaryFileParser::EFileStatus::Invalid &&
                parser.status() != BinaryFileParser::EFileStatus::Empty)
            {
                if (parser.file_type() == BinaryFileParser::EFileType::Compressed)
                {
                    spdlog::info("File '{}' is compressed, validation status '{}'", p.path().string(), file_status(parser.status()));
                }
                else if (parser.file_type() == BinaryFileParser::EFileType::Uncompressed)
                {
                    spdlog::info("File '{}' is uncompressed, validation status '{}'", p.path().string(), file_status(parser.status()));
                }
                else
                {
                    spdlog::info("File '{}' is raw, validation status '{}'", p.path().string(), file_status(parser.status()));
                }
            }
            else
            {
                spdlog::trace("File '{}' not supported");
            }
        }
    }

}

int main(int arg, char** argv)
{ 
    CLIParser cli;
    auto ret = cli.parse(arg, argv);
    if (ret != 0)
    {
        return ret;
    }

    if (cli.log_file())
    {
        std::array<TCHAR, MAX_PATH> path;
        ::GetModuleFileName(nullptr, path.data(), path.size());
        fs::path exe_path(path.begin(), path.end());
        exe_path.remove_filename();
        exe_path += "ufe.log";
        auto file_logger = spdlog::basic_logger_mt("default_logger", exe_path.string(), true);
        spdlog::set_default_logger(file_logger);
    }
    spdlog::set_pattern("[%H:%M:%S][%^%l%$] %v");
	spdlog::set_level(cli.logging_level());
    if (cli.export_mode() || cli.validate() || cli.patch())
    {
        parse(cli);
    }
	return 0;
}