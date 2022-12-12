#pragma once
#include "Records.hpp"
#include "JsonWriter.hpp"
#include "JsonReader.hpp"
class JsonHandler
{
public:
    enum class EMode
    {
        ReadJson,
        WriteJson
    };

    bool write_mode() { return m_mode == EMode::WriteJson; }

    nlohmann::ordered_json& class_with_members_and_types(const ufe::ClassWithMembersAndTypes& cmt)
    {
    }
private:
    EMode m_mode;
    JsonWriter m_writer;
    JsonReader m_reader;
    nlohmann::ordered_json m_json;
    nlohmann::ordered_json m_empty;
};

