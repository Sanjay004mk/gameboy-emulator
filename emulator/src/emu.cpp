#include "emupch.h"

#include "emu.h"
#include "cpu.h"

#include <renderer/rdr.h>

namespace emu
{
	static bool emulationPaused = false;

	extern void StartEmulator(int argc, const char** argv)
	{
		rdr::Renderer::Init(argc, argv, "emulator");

		{
			rdr::WindowConfiguration config{};
			config.title = "Gameboy Emulator";

			auto window = rdr::Renderer::InstantiateWindow(config);

			window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
				{
					if (window->IsKeyDown(rdr::Key::LeftControl))
					{
						switch (e.GetKeyCode())
						{
						case rdr::Key::D1:
							RDR_LOG_TRACE("Setting Present Mode to No vsync");
							window->SetPresentMode(rdr::PresentMode::NoVSync);
							break;
						case rdr::Key::D2:
							RDR_LOG_TRACE("Setting Present Mode to Triple buffered");
							window->SetPresentMode(rdr::PresentMode::TripleBuffer);
							break;
						case rdr::Key::D3:
							RDR_LOG_TRACE("Setting Present Mode to Vsync");
							window->SetPresentMode(rdr::PresentMode::VSync);
							break;
						}
					}
				});

			std::unique_ptr<CPU> cpu = std::make_unique<CPU>();

			while (!window->ShouldClose())
			{
				cpu->Update();

//				rdr::Renderer::BeginFrame(window);

	//			rdr::Renderer::EndFrame();

				rdr::Renderer::PollEvents();
			}
		}

		rdr::Renderer::Shutdown();
	}
}