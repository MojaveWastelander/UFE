#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <format>
#include "UFile.hpp"
#include <vector>
namespace fs = std::filesystem;

void read(ufe::Record& record, ufe::FileBuf& fb)
{
    record.read(fb);
}

int WinMain()
{
    using namespace nana;

    ufe::FileBuf fb{ R"(M:\Games\SteamLibrary\steamapps\common\Underrail\data\rules\items\components\bio\psibeetlecarapace)" };
    std::vector<std::unique_ptr<ufe::Record>> records;

    ufe::ERecordType rec_type;
    while ((rec_type = ufe::records::next_record_type(fb)) != ufe::ERecordType::MessageEnd)
    {
        records.push_back(ufe::records::create_record(rec_type));
        records.back()->read(fb);
    }

    ////Define a form.
    //form fm;

    ////Define a label and display a text.
    //label lab{ fm, "Hello, <bold blue size=16>Nana C++ Library</>" };
    //lab.format(true);

    ////Define a button and answer the click event.
    //button btn{ fm, "Quit" };
    //btn.events().click([&fm] {
    //    fm.close();
    //    });

    ////Layout management
    //fm.div("vert <><<><weight=80% text><>><><weight=24<><button><>><>");
    //fm["text"] << lab;
    //fm["button"] << btn;
    //fm.collocate();

    ////Show the form
    //fm.show();

    ////Start to event loop process, it blocks until the form is closed.
    //exec();
}