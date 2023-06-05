#pragma once
#include <filesystem>
#include <vector>
#include <gzip/compress.hpp>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gzip/version.hpp>
#include <zlib.h>

#include "Records.hpp"

namespace fs = std::filesystem;

class UFile
{
public:
private:
    fs::path m_file_path;

};

