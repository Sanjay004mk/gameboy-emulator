#include "emupch.h"

#include "emu.h"
#include "cpu.h"

#include <renderer/rdr.h>
#include <renderer/time.h>

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
			std::unique_ptr<CPU> cpu = std::make_unique<CPU>();

			window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
				{
					if (window->IsKeyDown(rdr::Key::LeftControl))
					{
						std::string file;
						switch (e.GetKeyCode())
						{
							// v sync
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

							// rom select
						case rdr::Key::O:
							file = window->OpenFile("Gameboy Rom (.gb)\0*.gb\0");
							if (file != std::string())
							{
								RDR_LOG_INFO("Opening rom {}", file);
								cpu->LoadAndStart(file.c_str());
							}
							break;

							// play / pause emulation
						case rdr::Key::P:
							cpu->Toggle();
							RDR_LOG_INFO("Emulation is {}", cpu->isRunning() ? "Running" : "Paused");
							break;

							// fullscreen toggle
						case rdr::Key::F11:
							RDR_LOG_INFO("Fullscreen: {}", !window->GetConfig().fullscreen);
							window->SetFullscreen(!window->GetConfig().fullscreen);
							break;

						}
					}
				});


			rdr::TextureBlitInformation blitInfo;
			blitInfo.srcMax = { 256, 256 };
			glm::ivec2 size(glm::min(window->GetConfig().size.x, window->GetConfig().size.y));

			if (window->GetConfig().size.x > window->GetConfig().size.y)
				blitInfo.dstMin.x = (window->GetConfig().size.x - size.x) / 2;
			else
				blitInfo.dstMin.y = (window->GetConfig().size.y - size.y) / 2;

			blitInfo.dstMax = blitInfo.dstMin + size;
			blitInfo.filter = rdr::SamplerFilter::Nearest;

			window->RegisterCallback<rdr::WindowResizeEvent>([&](rdr::WindowResizeEvent& e)
				{
					if (e.GetHeight() == 0 || e.GetWidth() == 0)
						return;

					uint32_t x = e.GetWidth(), y = e.GetHeight();
					glm::ivec2 size(glm::min(x, y));

					if (x > y)
						blitInfo.dstMin.x = (x - y) / 2;
					else
						blitInfo.dstMin.y = (y - x) / 2;

					blitInfo.dstMax = blitInfo.dstMin + size;
				});



			

			while (!window->ShouldClose())
			{
				float start, end;
				start = end = rdr::Time::GetTime();

				rdr::Renderer::BeginFrame(window);

				cpu->Update();

				rdr::Renderer::BlitToWindow(cpu->GetDisplayTexture(), blitInfo);

				rdr::Renderer::EndFrame();

				// TODO Wait for
				end = rdr::Time::GetTime();
				while ((end - start) < (1.f / 60.f))
					end = rdr::Time::GetTime();

				rdr::Renderer::PollEvents();
			}
		}

		rdr::Renderer::Shutdown();
	}
}