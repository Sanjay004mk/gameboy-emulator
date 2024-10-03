#include <emupch.h>
#include <renderer/rdr.h>

int main(int argc, const char** argv) 
{
    rdr::Renderer::Init(argc, argv, "emulator");

    rdr::WindowConfiguration config{};
    config.title = "Gameboy Emulator";

    auto window = rdr::Renderer::InstantiateWindow(config);

    RDR_LOG_INFO("Starting Main Loop");
    while (!window->ShouldClose())
    {
        rdr::Renderer::PollEvents();
    }

    rdr::Renderer::Shutdown();
    return 0;
}