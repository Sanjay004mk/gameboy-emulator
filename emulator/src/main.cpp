#include <emupch.h>
#include <renderer/rdr.h>

int main(int argc, const char** argv) 
{
    rdr::Renderer::Init(argc, argv, "emulator");

    rdr::WindowConfiguration config{};
    config.title = "Gameboy Emulator";

    auto window = rdr::Renderer::InstantiateWindow(config);

    window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
        {
            if (window->IsKeyDown(rdr::Key::LeftShift))
                RDR_LOG_INFO("{}", e);
        });

    RDR_LOG_INFO("Starting Main Loop");
    while (!window->ShouldClose())
    {
        rdr::Renderer::PollEvents();
    }

    rdr::Renderer::Shutdown();
    return 0;
}