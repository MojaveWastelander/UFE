#include <format>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"
#include "FileWriter.hpp"
#include "BinaryFileParser.hpp"
#include "JsonWriter.hpp"
#include "JsonReader.hpp"

//int xmain()
//{
//    std::filesystem::path item_path = R"(M:\Games\SteamLibrary\steamapps\common\Underrail\data\rules\items\components\bio\psibeetlecarapace)";
//    std::ifstream item_file{ item_path, std::ios::binary };
//    cereal::BinaryInputArchive ar{ item_file };
//
//    ufe::TypeRecord tr;
//    ar(tr);
//
//    try
//    {
//	    while (tr.type() != ufe::ERecordType::MessageEnd)
//	    {
//	        switch (tr.type())
//	        {
//	            case ufe::ERecordType::SerializedStreamHeader:
//	            {
//	                ufe::SerializationHeaderRecord header;
//	                ar(header);
//	            } break;
//	
//	            case ufe::ERecordType::BinaryLibrary:
//	            {
//	                ufe::BinaryLibrary bl;
//	                ar(bl);
//	            } break;
//	
//                case ufe::ERecordType::ClassWithMembersAndTypes:
//                {
//                    ufe::ClassWithMembersAndTypes cls;
//                    ar(cls);
//                } break;
//
//	            default:
//	                throw std::domain_error{ std::format("Record type not supported: {}", static_cast<int>(tr.type())).c_str() };
//	        }
//	
//	        // next record
//	        ar(tr);
//	    }
//    }
//    catch (std::exception& e)
//    {
//		std::cout << e.what() << '\n';
//    }
//}

int main(int arg, char** argv)
{ 
	std::filesystem::path item_path = R"(m:\Games\SteamLibrary\steamapps\common\Underrail\data\rules_2\stores)";//= argv[1];
	std::filesystem::path json_path = item_path ;//= argv[1];
	std::filesystem::path item_path2 = item_path;
    auto new_logger = spdlog::basic_logger_mt("default_logger", "logs/ufe.log", true);
    //spdlog::set_default_logger(new_logger);
	spdlog::set_level(spdlog::level::debug);

	for (const auto& p : fs::recursive_directory_iterator{item_path})
	{
        std::filesystem::path json_path = p;//= argv[1];
        json_path += ".json";
        {
            BinaryFileParser parser;
            if (parser.open(p))
            {
                parser.read_records();

                if (parser.status() != BinaryFileParser::EFileStatus::Invalid &&
                    parser.status() != BinaryFileParser::EFileStatus::Empty)
                {
	                //reader.export_json(json_path);
                    if (parser.status() == BinaryFileParser::EFileStatus::PartialRead)
                    {
                        spdlog::error("Partial file read!!!!");
                        continue;
                    }
	                //JsonWriter writer;
	                //writer.save(json_path, parser.get_records());
                    JsonReader reader;
                    reader.patch(json_path, p, parser);
                    break;
                }
            }
        }
	}

	//FileWriter writer;
	//writer.update_file(item_path2, json_path);
//	
//	register_any_visitor<ufe::SerializationHeaderRecord>([](const ufe::SerializationHeaderRecord& head)
//		{
//			std::cout << head.HeaderId.m_data << '\n';
//		});
//
//	register_any_visitor<ufe::ClassWithMembersAndTypes>([](const ufe::ClassWithMembersAndTypes& cmt)
//		{
//			std::cout << cmt.m_ClassInfo.Name.m_data.m_str << '\n';
//		});
//
//	for (const auto& rec : v)
//	{
//		process(rec);
//	}
	return 0;
}