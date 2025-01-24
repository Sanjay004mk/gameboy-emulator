#include "emupch.h"

#include "emu.h"
#include "cpu.h"

#include <renderer/rdr.h>
#include <renderer/time.h>

#include <imgui.h>
#include <imgui_internal.h>

namespace utils
{
	void open_rom(rdr::Window* window, emu::CPU* cpu)
	{
		std::string file = window->OpenFile("Gameboy Rom (.gb)\0*.gb\0");
		if (file != std::string())
		{
			RDR_LOG_INFO("Opening rom {}", file);
			cpu->LoadAndStart(file.c_str());
		}
	}
}

namespace emu
{
	void Emulator::StartEmulator(int argc, const char** argv)
	{
		rdr::Renderer::Init(argc, argv, "emulator");

		{
			rdr::WindowConfiguration config{};
			config.title = "Gameboy Emulator";

			auto& window = mAppInfo.window = rdr::Renderer::InstantiateWindow(config);

			auto& cpu = mAppInfo.cpu = new CPU();
			auto& currentEmulator = mAppInfo.emulators.emplace_back(cpu);

			window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
				{
					if (currentEmulator.OnKeyPressed(e))
						return;

					if (window->IsKeyDown(rdr::Key::LeftControl))
					{
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
							utils::open_rom(window, cpu);
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

			if (argc > 1)
				cpu->LoadAndStart(argv[1]);

			float start, end, ts = 0.f;
			start = end = rdr::Time::GetTime();
			while (!window->ShouldClose())
			{
				ts = end - start;
				start = end;

				rdr::Renderer::BeginFrame(window, {1, 1, 1, 1});

				ImGuiBasicUI();

				currentEmulator.Step(ts);

				rdr::Renderer::EndFrame();

				end = rdr::Time::GetTime();

				rdr::Renderer::PollEvents();
			}

			delete cpu;
		}

		rdr::Renderer::Shutdown();
	}

	void Emulator::ImGuiBasicUI()
	{
		static ImVec2 footerSize(0, 20);
		static ImVec2 topBarSize(0, 20);
		// dockspace
		{
			ImGuiDockNodeFlags docknode_flags = ImGuiDockNodeFlags_None;
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 size = viewport->Size;
			size.y -= footerSize.y + topBarSize.y;
			ImVec2 pos = viewport->Pos;
			pos.y += topBarSize.y;

			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("Dockspace", nullptr, window_flags);
			ImGui::PopStyleVar(3);

			ImGuiIO& io = ImGui::GetIO();

			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

				if (!ImGui::DockBuilderGetNode(dockspace_id))
				{
					ImGui::DockBuilderRemoveNode(dockspace_id);
					ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
					ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

					mAppInfo.GetCurrentEmulator().ImGuiDefaultLayout(dockspace_id);
				}				

				ImGui::DockSpace(dockspace_id, { 0.0f, 0.0f }, docknode_flags);
			}

			ImGui::End();
		}

		// title bar
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 size = viewport->Size;
			size.y = topBarSize.y;
			ImVec2 pos = viewport->Pos;

			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

			ImGuiWindowFlags titleBarFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));

			ImGui::Begin("Title Bar", nullptr, titleBarFlags);
			ImVec2 region = ImGui::GetContentRegionAvail();
			ImGui::PopStyleVar(3);
			if (ImGui::Button("File", ImVec2(ImGui::CalcTextSize("File").x + 14.f, size.y)))
			{
				ImGui::OpenPopup("File Menu");
			}

			if (ImGui::BeginPopup("File Menu"))
			{
				if (ImGui::MenuItem("Open Rom", "Ctrl+O"))
				{
					utils::open_rom(mAppInfo.window, mAppInfo.cpu);
				}
				// TODO : save states, reload, etc.
				ImGui::EndPopup();
			}

			mAppInfo.GetCurrentEmulator().ImGuiMenuBarOptions();

			ImGui::End();
			ImGui::PopStyleColor(2);
		}

		// footer information bar
		{
			// set position of the footer to the bottom of the window and set its size
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGuiIO& io = ImGui::GetIO();
			ImVec2 pos = viewport->Pos;
			ImVec2 size = viewport->Size;
			pos.y += size.y - footerSize.y;
			size.y = footerSize.y;

			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.f, 2.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, footerSize);

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));


			ImGui::Begin("footer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking);

			std::string fps = fmt::format("Frametime: {:.3f} ms | FPS: {:.3f}", 1e3f / io.Framerate, io.Framerate);

			ImVec2 region = ImGui::GetContentRegionAvail();
			ImVec2 textSize = ImGui::CalcTextSize(fps.c_str());

			ImGui::SetCursorPosY((region.y - textSize.y));
			ImGui::SetCursorPosX(region.x - textSize.x);
			ImGui::Text("%s", fps.c_str());

			mAppInfo.GetCurrentEmulator().ImGuiFooterOptions({ region.x - textSize.x, region.y });

			ImGui::End();

			ImGui::PopStyleColor();
			ImGui::PopStyleVar(4);
		}
	}

	Emulator::Emulator(CPU* cpu)
		: mCpu(cpu)
	{

	}

	Emulator::~Emulator()
	{

	}

	void Emulator::Step(float ts)
	{
		ImGui::Begin("Instructions");
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
		ImGui::Begin("Emulator");

		ImVec2 size = ImGui::GetContentRegionAvail();
		if (size.x > size.y)
		{
			ImGui::SetCursorPosX((size.x - size.y) / 2.f);
			size.x = size.y;
		}
		else
		{
			ImGui::SetCursorPosY((size.y - size.x) / 2.f);
			size.y = size.x;
		}
		ImGui::Image(mCpu->GetDisplayTexture()->GetImGuiID(), size);
		ImGui::End();

		ImGui::PopStyleVar();

		ImGui::Begin("Registers");
		ImGui::End();

		ImGui::Begin("Memory");
		ImGui::End();

		// TODO change emulation speed
		static float acc = 0.f;
		if ((acc += ts) < (1.f / 60.f))
			return;

		acc = 0.f;

		mCpu->Update();
	}

	void Emulator::ImGuiDefaultLayout(uint32_t& dockspace_id)
	{
		ImGuiID copy = dockspace_id;
		ImGuiID right = ImGui::DockBuilderSplitNode(copy, ImGuiDir_Right, 0.25f, nullptr, &copy);
		ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.5f, nullptr, &right);
		ImGuiID centre = ImGui::DockBuilderSplitNode(copy, ImGuiDir_Left, 0.75f, nullptr, &copy);
		ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.35f, nullptr, &centre);

		ImGui::DockBuilderDockWindow("Instructions", right);
		ImGui::DockBuilderDockWindow("Registers", right_bottom);
		ImGui::DockBuilderDockWindow("Emulator", centre);
		ImGui::DockBuilderDockWindow("Memory", bottom);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	void Emulator::ImGuiMenuBarOptions()
	{

	}

	void Emulator::ImGuiFooterOptions(const glm::vec2& region)
	{

	}

	bool Emulator::OnKeyPressed(rdr::KeyPressedEvent& e)
	{
		return false;
	}
}