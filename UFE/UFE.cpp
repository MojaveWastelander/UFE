#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include <fmt/format.h>
#include <gzip/compress.hpp>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gzip/version.hpp>
#include <zlib.h>
#include "UFile.hpp"
namespace fs = std::filesystem;

void read(ufe::Record& record, ufe::FileBuf& fb)
{
    record.read(fb);
}

int WinMain()
{
    using namespace nana;

    ufe::FileBuf fb{ R"(M:\Games\SteamLibrary\steamapps\common\Underrail\data\rules\items\components\bio\psibeetlecarapace)" };
    fb.read<uint8_t>();

    ufe::Record header{ ufe::records::SerializationHeaderRecord{} };
    header.read(fb);
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