#pragma once
#include <filesystem>
#include <vector>
#include <fstream>
#include <type_traits>
#include "IndexedData.hpp"
//#include <gzip/compress.hpp>
//#include <gzip/config.hpp>
//#include <gzip/decompress.hpp>
//#include <gzip/utils.hpp>
//#include <gzip/version.hpp>
//#include <zlib.h>

#include "Records.hpp"

namespace fs = std::filesystem;

class FileReader
{
public:
    bool open(fs::path file_path)
    {
        if (fs::exists(file_path) && fs::is_regular_file(file_path))
        {
            m_file.open(file_path, std::ios::binary);
            return true;
        }
        return false;
    }

    ufe::ERecordType get_record_type()
    {
        char type;
        m_file.read(&type, 1);
        return static_cast<ufe::ERecordType>(type);
    }

    template <typename T>
    bool read(IndexedData<T>& data)
    {
        static_assert(!std::is_fundamental_v<T>, "Type doesn't have specialization");
        data.m_offset = m_file.tellg();
        m_file.read(reinterpret_cast<char*>(&data.m_data, sizeof(data.m_data)));
    }
private:
    std::ifstream m_file;
    fs::path m_file_path;
};

class UFile
{
public:
private:
    fs::path m_file_path;

};

