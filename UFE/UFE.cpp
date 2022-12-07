#include <format>
#include "UFile.hpp"
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
	std::filesystem::path item_path = argv[1];
	std::filesystem::path item_path2 = R"(M:\Games\SteamLibrary\steamapps\common\Underrail\data\rules\items\components\bio\psibeetlecarapace2)";
	spdlog::set_level(spdlog::level::debug);
	{
		FileReader reader;
		if (reader.open(item_path))
		{
			reader.read_records();

			reader.export_json(item_path += ".json");
		}
	}
	//fs::path json_path("__psibeetlecarapace.json");
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