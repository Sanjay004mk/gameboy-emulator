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

			rdr::TextureBlitInformation blitInfo;
			blitInfo.srcMax = { 256, 256 };
			glm::ivec2 size(glm::min(window->GetConfig().size.x, window->GetConfig().size.y));

			if (window->GetConfig().size.x > window->GetConfig().size.y)
				blitInfo.dstMin.x = (window->GetConfig().size.x - size.x) / 2;
			else
				blitInfo.dstMin.y = (window->GetConfig().size.y - size.y) / 2;

			blitInfo.dstMax = blitInfo.dstMin + size;
			blitInfo.filter = rdr::SamplerFilter::Nearest;

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