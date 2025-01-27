#include "emupch.h"

#include "emu.h"
#include "cpu.h"

#include <renderer/rdr.h>
#include <renderer/time.h>

#include <imgui.h>
#include <imgui_internal.h>

static ImVec2 footerSize(0, 20);
static ImVec2 topBarSize(0, 20);

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

	void dockspace_setup_common(const char* window_name)
	{
		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus;

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

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(window_name, nullptr, window_flags);
		ImGui::PopStyleVar(3);

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
			mAppInfo.emulators.emplace_back(new Emulator(cpu));

			mAppInfo.emulators.push_back(new Debugger(cpu));

			window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
				{
					if (mAppInfo.GetCurrentEmulator().OnKeyPressed(e))
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

							// reset emulation
						case rdr::Key::R:
							cpu->Reset(!window->IsKeyDown(rdr::Key::LeftShift));
							RDR_LOG_INFO("Emulation Reset");
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

			mAppInfo.GetCurrentEmulator().OnAttach();

			while (!window->ShouldClose())
			{
				ts = end - start;
				start = end;

				rdr::Renderer::BeginFrame(window, {0.1, 0.1, 0.1, 1});

				if (!window->GetConfig().minimized)
				{
					ImGuiBasicUI();
					mAppInfo.GetCurrentEmulator().OnImGuiUpdate();
				}

				mAppInfo.GetCurrentEmulator().Step(ts);

				rdr::Renderer::EndFrame();

				end = rdr::Time::GetTime();

				rdr::Renderer::PollEvents();
			}

			mAppInfo.GetCurrentEmulator().OnDetach();

			for (auto& emulator : mAppInfo.emulators)
				delete emulator;

			delete cpu;
		}

		rdr::Renderer::Shutdown();
	}

	void Emulator::ImGuiBasicUI()
	{
		mAppInfo.GetCurrentEmulator().ImGuiSetupDockspace();

		// menu bar
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
				if (ImGui::MenuItem("Open Rom without Boot", "Ctrl+Shift+O"))
				{
					utils::open_rom(mAppInfo.window, mAppInfo.cpu);
					mAppInfo.cpu->Reset(false);
				}
				// TODO : save states, reload, etc.
				ImGui::EndPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Options", ImVec2(ImGui::CalcTextSize("Options").x + 14.f, size.y)))
			{
				ImGui::OpenPopup("Options Menu");
			}

			if (ImGui::BeginPopup("Options Menu"))
			{
				if (ImGui::MenuItem("Run"))
				{
					mAppInfo.SetEmulator(0);
				}

				if (ImGui::MenuItem("Debug"))
				{
					mAppInfo.SetEmulator(1);
				}

				ImGui::EndPopup();
			}

			mAppInfo.GetCurrentEmulator().ImGuiMenuBarOptions();

			ImGui::End();
			ImGui::PopStyleColor(2);
		}

		// footer information bar
		{
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
		if ((timeSinceLastUpdate += ts) >= (defaultSpeed / emulationSpeed))
		{
			timeSinceLastUpdate = 0.f;
			mCpu->Update();
		}

	}

	void Emulator::OnImGuiUpdate()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));
		ImGui::Begin("Gameplay", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		ImVec2 size = ImGui::GetContentRegionAvail();

		if (size.x >= size.y)
		{
			ImGui::SetCursorPosX((size.x - size.y) / 2.f);
			size.x = size.y * (160.0f / 144.0f);
		}
		else
		{
			ImGui::SetCursorPosY((size.y - size.x) / 2.f);
			size.y = size.x * (144.0f / 160.0f);
		}

		ImGui::Image(mCpu->GetDisplayTexture()->GetImGuiID(), size);

		ImGui::End();
	}

	void Emulator::ImGuiSetupDockspace()
	{
		utils::dockspace_setup_common("RunDockspaceWindow");

		ImGuiID dockspace_id = ImGui::GetID("RunDockspace");

		if (buildDockspace)
		{
			buildDockspace = false;
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

			ImGui::DockBuilderDockWindow("Gameplay", dockspace_id);

			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGui::DockSpace(dockspace_id, { 0.0f, 0.0f }, ImGuiDockNodeFlags_NoTabBar);

		ImGui::End();
	}

	void Emulator::ImGuiMenuBarOptions()
	{
		ImGui::SameLine();

		if (ImGui::Button("Emulation", ImVec2(ImGui::CalcTextSize("Emulation").x + 14.f, topBarSize.y)))
		{
			ImGui::OpenPopup("Emulation Menu");
		}

		if (ImGui::BeginPopup("Emulation Menu"))
		{
			if (ImGui::MenuItem("Pause", "Ctrl + P"))
				mCpu->Pause();

			if (ImGui::MenuItem("Resume", "Ctrl + P"))
				mCpu->Resume();

			if (ImGui::MenuItem("Reset", "Ctrl + R"))
				mCpu->Reset();

			for (float i = 0.5f; i <= 5.f; i += 0.5f)
			{
				if (ImGui::MenuItem(("Speed: " + fmt::format("{:.1f}", i) + "x").c_str(), "Inc: ] Dec: ["))
					emulationSpeed = i;
			}

			ImGui::InputFloat("Custom Speed", &emulationSpeed);


			ImGui::EndPopup();
		}

	}

	void Emulator::ImGuiFooterOptions(const glm::vec2& region)
	{

	}

	bool Emulator::OnKeyPressed(rdr::KeyPressedEvent& e)
	{
		if (e.GetKeyCode() == rdr::Key::LeftBracket)
		{
			emulationSpeed -= 0.1f;
			return true;
		}
		else if (e.GetKeyCode() == rdr::Key::RightBracket)
		{
			emulationSpeed += 0.1f;
			return true;
		}

		return false;
	}

	Debugger::Debugger(CPU* cpu)
		: Emulator(cpu)
	{

	}

	Debugger::~Debugger()
	{

	}

	void Debugger::Step(float ts)
	{
		switch (emulationMode)
		{
		case Mode::Run:
		{
			if ((timeSinceLastUpdate += ts) >= (defaultSpeed / emulationSpeed))
			{
				timeSinceLastUpdate = 0.f;
				mCpu->Update();
			}
			break;
		}
		case Mode::Step:
		{
			if (shouldStep)
			{
				shouldStep = false;
				mCpu->SingleStepUpdate();
			}
			break;
		}
		}

		if (mCpu->serialPresent)
		{
			consoleOutput += (mCpu->SerialOut());
			consoleScrollToBottom = true;
		}
	}

	bool Debugger::OnKeyPressed(rdr::KeyPressedEvent& e)
	{
		if (Emulator::OnKeyPressed(e))
			return true;

		if (e.GetKeyCode() == rdr::Key::F9)
		{
			shouldStep = true;
			return true;
		}
		else if (e.GetKeyCode() == rdr::Key::F5 || (e.GetKeyCode() == rdr::Key::P && mAppInfo.window->IsKeyDown(rdr::Key::LeftControl)))
		{
			emulationMode = (Mode)(!(bool)(emulationMode));
			return true;
		}

		return false;
	}

	void Debugger::UpdateMemoryView()
	{
		memoryDisplay.clear();
		std::stringstream ss;
		std::string line;

		for (uint32_t i = 0; i < 0x1000; i++)
		{
			ss << std::hex << std::setw(3) << std::setfill('0') << i << "0 : ";
			for (uint32_t j = 0; j < 0x10; j++)
				ss << std::hex << std::setw(2) << std::setfill('0') << ((uint32_t)mCpu->memory.memory[(i * 0x10)+ j]) << " ";
			ss << std::endl;

			std::getline(ss, line);
			memoryDisplay.push_back(line);
		}

	}

	void Debugger::OnImGuiUpdate()
	{
		// Instruction window
		{
			ImGui::Begin("Instructions");

			uint32_t startAddr = glm::max(0, mCpu->pc - 50);
			uint32_t endAddr = glm::min(0xffff, mCpu->pc + 50);
			std::stringstream lines;

			for (uint32_t i = startAddr; i < endAddr; i++)
			{
				if (i == mCpu->pc)
					lines << "> ";

				lines << "0x" << std::hex << i << " (" << std::dec << i << ") : ";
				lines << "0x" << std::hex << ((uint32_t)mCpu->memory.memory[i]) << "\n";
			}

			std::string line;
			while (std::getline(lines, line))
				ImGui::Text(line.c_str());


			ImGui::End();
		}

		// Emulator Window
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
			ImGui::Begin("Emulator");

			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size.x > size.y)
			{
				ImGui::SetCursorPosX((size.x - size.y) / 2.f);
				size.x = size.y * (160.0f / 144.0f);
			}
			else
			{
				ImGui::SetCursorPosY((size.y - size.x) / 2.f);
				size.y = size.x * (144.0f / 160.0f);
			}
			ImGui::Image(mCpu->GetDisplayTexture()->GetImGuiID(), size);
			ImGui::End();

			ImGui::PopStyleVar();
		}

		// PPU Window
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
			ImGui::Begin("PPU");

			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size.x > size.y)
				size.y = size.x /= 2.f;
			else
				size.x = size.y /= 2.f;
			
			rdr::Texture* textures[] = {
				mCpu->ppu.bg.texture,
				mCpu->ppu.window.texture,
				mCpu->ppu.sprite.texture,
				mCpu->ppu.tiles.texture
			};
			uint32_t count = 1;
			for (auto& t : textures)
			{
				ImGui::Image(t->GetImGuiID(), size);
				if (count++ % 2)
					ImGui::SameLine();
			}
			
			ImGui::End();

			ImGui::PopStyleVar();
		}

		// Register window
		{
			ImGui::Begin("Registers");

			ImGui::Text("Registers");

			struct RegInfo
			{
				uint16_t* reg = nullptr;
				bool pad = true;
				char buf[18] = { };
			};

			std::pair<RegInfo, const char*> registers[] = {
				{ { &mCpu->AF.b16 }, "AF"},
				{ { &mCpu->BC.b16 }, "BC"},
				{ { &mCpu->DE.b16 }, "DE"},
				{ { &mCpu->HL.b16 }, "HL"},
				{ { &mCpu->sp, false }, "SP "},
				{ { &mCpu->pc, false }, "PC" },
			};

			for (auto& [reg, name] : registers)
			{
				sprintf_s(reg.buf, "%04x", *(reg.reg));
				if (ImGui::InputText(name, reg.buf, sizeof(reg.buf), ImGuiInputTextFlags_CharsHexadecimal))
				{
					// fill trailing digits with zeroes instead of making the value smaller
					// to avoid register interchage
					for (int i = 0; i < 4; i++)
						if (!reg.buf[i])
							reg.buf[i] = '0';

					uint32_t regval = std::stoul(reg.buf, 0, 16);
					if (regval <= 0xffff)
						*(reg.reg) = (uint16_t)(regval);
				}
			}

			ImGui::Separator();

			ImGui::Text("Flags");

			bool znhc[4] = {};
			uint32_t bit = 7;
			for (auto& b : znhc)
			{
				b = (mCpu->AF.lo & (1 << bit));
				bit--;
			}

			std::pair<bool*, const char*> flags[] = {
				{ &znhc[0], "Z" },
				{ &znhc[1], "N" },
				{ &znhc[2], "H" },
				{ &znhc[3], "C" },
				{ &mCpu->flags.ime, "IME" },
				{ &mCpu->flags.halt, "Halt"},
			};

			uint32_t flagCount = 3;
			for (auto& [flag, name] : flags)
			{
				ImGui::Checkbox(name, flag);
				if (flagCount--)
					ImGui::SameLine();
				else
					flagCount = 1;
			}

			bit = 7;
			for (auto& b : znhc)
			{
				mCpu->AF.lo |= b ? (1 << bit) : 0;
				bit--;
			}

			ImGui::Separator();



			ImGui::End();
		}

		// Memory window
		{
			ImGui::Begin("Memory");

			if (ImGui::Button("Update Memory"))
				UpdateMemoryView();

			for (auto& line : memoryDisplay)
				ImGui::Text(line.c_str());

			ImGui::End();
		}

		// Serial window
		{
			ImGui::Begin("Serial");

			std::string line;
			std::stringstream consoleStream(consoleOutput);
			while (std::getline(consoleStream, line))
				ImGui::TextWrapped(line.c_str());

			if (consoleScrollToBottom)
			{
				consoleScrollToBottom = false;
				ImGui::SetScrollHereY(1.f);
			}

			ImGui::End();
		}

	}

	void Debugger::ImGuiSetupDockspace()
	{
		utils::dockspace_setup_common("DebugDockspaceWindow");

		ImGuiID dockspace_id = ImGui::GetID("DebugDockspace");

		if (buildDockspace)
		{
			buildDockspace = false;
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());
				
			ImGuiID copy = dockspace_id;
			ImGuiID right = ImGui::DockBuilderSplitNode(copy, ImGuiDir_Right, 0.25f, nullptr, &copy);
			ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.5f, nullptr, &right);
			ImGuiID centre = ImGui::DockBuilderSplitNode(copy, ImGuiDir_Left, 0.75f, nullptr, &copy);
			ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.35f, nullptr, &centre);

			ImGui::DockBuilderDockWindow("Instructions", right);
			ImGui::DockBuilderDockWindow("Registers", right_bottom);
			ImGui::DockBuilderDockWindow("Emulator", centre);
			ImGui::DockBuilderDockWindow("PPU", centre);
			ImGui::DockBuilderDockWindow("Memory", bottom);
			ImGui::DockBuilderDockWindow("Serial", bottom);
		
			ImGui::DockBuilderFinish(dockspace_id);
		}

		ImGui::DockSpace(dockspace_id, { 0.0f, 0.0f }, ImGuiDockNodeFlags_None);

		ImGui::End();
	}

	void Debugger::ImGuiMenuBarOptions()
	{
		Emulator::ImGuiMenuBarOptions();

		ImGui::SameLine();

		if (ImGui::Button("Debug", ImVec2(ImGui::CalcTextSize("Debug").x + 14.f, topBarSize.y)))
		{
			ImGui::OpenPopup("Debug Menu");
		}

		if (ImGui::BeginPopup("Debug Menu"))
		{
			if (ImGui::MenuItem("Run", "F5"))
			{
				emulationMode = Mode::Run;
			}

			if (ImGui::MenuItem("Step by Step", "F5"))
			{
				emulationMode = Mode::Step;
			}

			if (ImGui::MenuItem("Step", "F9"))
			{
				shouldStep = true;
			}

			ImGui::EndPopup();
		}
	}

	void Debugger::ImGuiFooterOptions(const glm::vec2& region)
	{
	}
}