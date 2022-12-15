#include "CLIParser.hpp"

CLIParser::CLIParser()
{
    try
    {
        m_app.add_option("path", m_base_path, "file/directory to be processed")->check(CLI::ExistingPath);
        m_app.add_flag("-e,--export", m_export, "export file(s) to json, resulting filename is '<parsed_filename>.json'");
        m_app.add_flag("-p,--patch", m_patch, "patch existing binary file(s) with respective json file(s)");
        //m_app.add_option("-p,--patch", m_patch_dir, "output directory for json files, default is parsed file directory");
        //auto out_opt = m_app.add_option("-o,--outdir", m_out_dir, "output directory for json files, default is parsed file directory");
        //m_app.add_option("-r,--rootdir", m_root_dir, "root directory used as reference for creating original directory structure")->needs(out_opt);
        m_app.add_flag("-v,--validate", m_validate, "verify file(s) integrity for packed/unpacked files");
        m_app.add_option("-l,--loglevel", m_logging_level, "set logging level, [trace, debug, info, warn, error, critical, off], default info")->check(CLI::Range(0, 6));
        m_app.add_flag("--log_file", m_log_file, "log to file 'ufe.log' instead of console");
    }
    catch (std::exception& e)
    {
        spdlog::error(e.what());
    }
}

int CLIParser::parse(int argc, char** argv)
{
    try 
    {
        m_app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e) 
    {
        return m_app.exit(e);
    }
    if (base_path().empty())
    {
        auto err = CLI::Error{ "Path validation", "Invalid base path", CLI::ExitCodes::ValidationError };
        return m_app.exit(err);
    }
    return 0;
}
