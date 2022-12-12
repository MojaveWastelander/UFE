#pragma once
#include <filesystem>
#include <vector>
#include <fstream>
#include <map>
#include <type_traits>
#include "IndexedData.hpp"
#include <any>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "Records.hpp"

class JsonReader
{
public:
    nlohmann::ordered_json& class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt)
    {
    }
private:
    nlohmann::ordered_json m_json;
};

