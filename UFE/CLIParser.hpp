#pragma once
#include "CLI/App.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Config.hpp"
#include <filesystem>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"
class CLIParser
{
public:
    CLIParser();
    int parse(int argc, char** argv);
    spdlog::level::level_enum logging_level() const { return static_cast<spdlog::level::level_enum>(m_logging_level); }
    bool export_mode() const { return m_export; }
    const std::filesystem::path& base_path() const { return m_base_path; }
    const std::filesystem::path& out_dir() const { return m_out_dir; }
    const std::filesystem::path& root_dir() const { return m_root_dir; }
    bool validate() const { return m_validate; }
    bool patch() const { return m_patch; }
    bool log_file() const { return m_log_file; }
private:
    CLI::App m_app;
    int m_logging_level = spdlog::level::info;
    bool m_export = false;
    bool m_log_file = false;
    std::filesystem::path m_base_path;
    std::filesystem::path m_out_dir;
    std::filesystem::path m_root_dir;
    std::filesystem::path m_patch_dir;
    bool m_validate = false;
    bool m_patch = false;
};

