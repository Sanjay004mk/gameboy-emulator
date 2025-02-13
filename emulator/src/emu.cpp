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
	static void open_rom(rdr::Window* window, emu::CPU* cpu)
	{
		std::string file = window->OpenFile("Gameboy Rom (.gb)\0*.gb\0");
		if (file != std::string())
		{
			RDR_LOG_INFO("Opening rom {}", file);
			cpu->LoadAndStart(file.c_str());
		}
	}

	static void open_ram(rdr::Window* window, emu::CPU* cpu)
	{
		std::string file = window->OpenFile("Gameboy Save File (.sav)\0*.sav\0");
		cpu->LoadRAM(file.c_str());
	}

	static void dockspace_setup_common(const char* window_name)
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

	template <typename T>
	static void imgui_uint_setter(T* value, char buf[], const char* name)
	{
		static constexpr const char* pat = sizeof(T) == 1 ? "%02x" : "%04x";
		sprintf_s(buf, sizeof(buf), pat, *(value));
		if (ImGui::InputText(name, buf, sizeof(buf), ImGuiInputTextFlags_CharsHexadecimal))
		{
			// fill trailing digits with zeroes instead of making the value smaller
			// to avoid register interchage
			for (int i = 0; i < sizeof(T) * 2; i++)
				if (!buf[i])
					buf[i] = '0';

			uint32_t regval = std::stoul(buf, 0, 16);
			static constexpr uint32_t max_val = sizeof(T) == 1 ? 0xff : 0xffff;
			if (regval <= max_val)
				*(value) = (T)(regval);
		}
	}

	static constexpr auto imgui_uint8_t_setter = imgui_uint_setter<uint8_t>;
	static constexpr auto imgui_uint16_t_setter = imgui_uint_setter<uint16_t>;

	struct TimerCallback
	{
		float timeout = 1.f;
		std::function<void(void*)> callback;
		void* userPointer = nullptr;
		float acc = 0.f;

		void Tick(float ts)
		{
			acc += ts;
			if (acc >= timeout)
			{
				callback(userPointer);
				acc = 0.f;
			}
		}
	};

	static std::unordered_map<rdr::KeyCode, emu::Input> keyMap = {
		{rdr::Key::A, emu::Input_Button_A},
		{rdr::Key::Z, emu::Input_Button_B},
		{rdr::Key::S, emu::Input_Button_Select},
		{rdr::Key::X, emu::Input_Button_Start},
		{rdr::Key::Up, emu::Input_Button_Up},
		{rdr::Key::Down, emu::Input_Button_Down},
		{rdr::Key::Left, emu::Input_Button_Left},
		{rdr::Key::Right, emu::Input_Button_Right},
	};

	struct EmulationData
	{
		TimerCallback autoSaver;

		emu::ColorPalette paletteEdit = emu::Palettes::defaultPalette;
		std::vector<emu::ColorPalette> customPalettes;

		static inline constexpr uint32_t max_save_slots = 8;
		uint32_t currentSaveSlot = 0, currentLoadSlot = 0, numSaves = 0;
		std::array<emu::EmulationState, max_save_slots> saveStates;
		std::string saveStateFile = "gb-states";
		
		bool colorSelectWindowIsOpen = false;
		ImVec4 color[4] = {};

		bool ColorSelectWindow()
		{
			bool ret = false;

			if (colorSelectWindowIsOpen)
			{
				ImGuiViewport* viewport = ImGui::GetMainViewport();

				ImVec2 size(500, 300);
				ImVec2 pos = viewport->Pos;
				pos.x += (viewport->Size.x - size.x) / 2;
				pos.y += (viewport->Size.y - size.y) / 2;

				ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
				ImGui::SetNextWindowSize(size, ImGuiCond_Once);


				ImGui::Begin("Color Select", &colorSelectWindowIsOpen);
				for (size_t i = 0; i < 4; i++)
				{
					auto& cw = color[i];
					auto& cv = paletteEdit.values[i];
					cw = ImGui::ColorConvertU32ToFloat4(cv);
					if (ImGui::ColorEdit4(("Color - " + std::to_string(i)).c_str(), &cw.x))
					{
						cv = ImGui::ColorConvertFloat4ToU32(cw);
						ret = true;
					}
				}
				ImGui::End();
			}
			return ret;
		}
	};

	static EmulationData emuData;

	static void save_states_to_file(const std::string& file)
	{
		if (emuData.numSaves == 0)
			return;

		for (uint32_t i = 0; i < emuData.numSaves; i++)
		{
			auto& state = emuData.saveStates[i];

			if (!state.valid)
				continue;

			std::ofstream statesFile(file + " " + std::to_string(i), std::ios::binary);
			char rom[256] = {};
			sprintf_s(rom, "%ls", state.rom.c_str());
			statesFile.write(rom, sizeof(rom));

			uint16_t data[] = {
				state.af,
				state.bc,
				state.de,
				state.hl,
				state.pc,
				state.sp
			};

			size_t offs[] = { state.romBankOffset, state.ramOffset };
			bool en[] = { state.romSelect, state.ramEnabled };

			statesFile.write((char*)data, sizeof(data));
			statesFile.write((char*)offs, sizeof(offs));
			statesFile.write((char*)en, sizeof(en));

			statesFile.write((char*)state.memory, sizeof(state.memory));
		}
	}

	static void save_states_to_file_dialog(rdr::Window* window)
	{
		std::string file = window->OpenFile("Save state\0*\0");
		if (file != std::string())
		{
			save_states_to_file(file);
			emuData.saveStateFile = file;
		}
	}

	static void load_states_from_file(const std::string& file)
	{
		for (size_t i = 0; i < emuData.max_save_slots; i++)
		{
			std::string loadFile = file + " " + std::to_string(i);
			if (!std::filesystem::exists(loadFile))
				continue;
			std::ifstream statesFile(loadFile, std::ios::binary);
			auto& state = emuData.saveStates[i];
			char rom[256] = {};
			statesFile.read(rom, sizeof(rom));
			state.rom = rom;

			uint16_t* u16locs[] = {
				&state.af,
				&state.bc,
				&state.de,
				&state.hl,
				&state.pc,
				&state.sp
			};

			size_t* slocs[] = {
				&state.romBankOffset,
				&state.ramOffset
			};

			bool* elocs[] = {
				&state.romSelect,
				&state.ramEnabled
			};

			for (auto& loc : u16locs)
				statesFile.read((char*)loc, 2);

			for (auto& loc : slocs)
				statesFile.read((char*)loc, sizeof(size_t));

			for (auto& loc : elocs)
				statesFile.read((char*)loc, 1);

			statesFile.read((char*)state.memory, sizeof(state.memory));

			state.valid = true;

			emuData.numSaves++;
		}
	}

	static void load_states_from_file_dialog(rdr::Window* window)
	{
		std::string file = window->OpenFile("Save state\0*\0");
		if (file != std::string())
		{
			RDR_LOG_INFO("Loading state {}", file);
			load_states_from_file(file);
			emuData.saveStateFile = file;
		}
	}

	static void save_state(emu::CPU* cpu, int32_t index = -1)
	{
		if (index == -1)
		{
			index = emuData.currentSaveSlot;
			emuData.currentSaveSlot = (emuData.currentSaveSlot + 1) % emuData.max_save_slots;
		}
		auto& state = emuData.saveStates[index];

		emuData.numSaves = glm::min(emuData.max_save_slots, emuData.numSaves + 1);
		cpu->SaveState(state);

		save_states_to_file(emuData.saveStateFile);
	}

	static void load_state(emu::CPU* cpu, int32_t index = -1)
	{
		if (emuData.numSaves == 0)
			return;

		if (index == -1)
		{
			index = emuData.currentLoadSlot;
			emuData.currentLoadSlot = (emuData.currentLoadSlot + 1) % emuData.max_save_slots;
		}

		auto& state = emuData.saveStates[index];
		cpu->LoadState(state);
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

			utils::TimerCallback& autoSaver = utils::emuData.autoSaver;
			autoSaver.timeout = 5.f;
			autoSaver.callback = [&](void* user) { cpu->SaveRAM(); };

			mAppInfo.emulators.push_back(new Emulator(cpu));
			mAppInfo.emulators.push_back(new Debugger(cpu));

			utils::load_states_from_file(utils::emuData.saveStateFile);

			window->SetEventCallback([&](rdr::Event& e) 
				{
					mAppInfo.GetCurrentEmulator().OnEvent(e);
				});

			window->RegisterCallback<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
				{
					mAppInfo.GetCurrentEmulator().OnEvent(e);

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
							cpu->Reset(!window->IsKeyDown(rdr::Key::K));
							break;

							// ram select
						case rdr::Key::L:
							utils::open_ram(window, cpu);
							break;

							// save game
						case rdr::Key::S:
							cpu->SaveRAM();
							break;

							// save states
						case rdr::Key::C:
							utils::save_state(cpu);
							break;

						case rdr::Key::V:
							utils::load_state(cpu);
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

				rdr::Renderer::PollEvents();

				autoSaver.Tick(ts);
				
				end = rdr::Time::GetTime();
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
				if (ImGui::MenuItem("Open Rom", "Ctrl + O"))
				{
					utils::open_rom(mAppInfo.window, mAppInfo.cpu);
				}
				if (ImGui::MenuItem("Open Rom without Boot", "Ctrl + K + O"))
				{
					utils::open_rom(mAppInfo.window, mAppInfo.cpu);
					mAppInfo.cpu->Reset(false);
				}
				if (ImGui::MenuItem("Open Game Save File", "Ctrl + L"))
				{
					utils::open_ram(mAppInfo.window, mAppInfo.cpu);
				}
				if (ImGui::MenuItem("Save Game", "Ctrl + S"))
				{
					mAppInfo.cpu->SaveRAM();
				}
				ImGui::InputFloat("Auto save Interval", &utils::emuData.autoSaver.timeout);
				// TODO : save states, reload, etc.
				ImGui::EndPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Mode", ImVec2(ImGui::CalcTextSize("Mode").x + 14.f, size.y)))
			{
				ImGui::OpenPopup("Mode Menu");
			}

			if (ImGui::BeginPopup("Mode Menu"))
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

			ImGui::SameLine();

			if (ImGui::Button("Color Palette", ImVec2(ImGui::CalcTextSize("Color Palette").x + 14.f, topBarSize.y)))
			{
				ImGui::OpenPopup("Color Palette Menu");
			}

			if (ImGui::BeginPopup("Color Palette Menu"))
			{
				for (auto& [name, pal] : Palettes::allPalettes)
					if (ImGui::MenuItem(name))
						mAppInfo.cpu->SetColorPalette(pal);

				if (ImGui::MenuItem("Custom Color"))
				{
					utils::emuData.paletteEdit = mAppInfo.cpu->GetColorPalette();
					utils::emuData.colorSelectWindowIsOpen = true;
				}

				ImGui::EndPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save state", ImVec2(ImGui::CalcTextSize("Save state").x + 14.f, topBarSize.y)))
			{
				ImGui::OpenPopup("Save state Menu");
			}

			if (ImGui::BeginPopup("Save state Menu")) 
			{

				for (uint32_t i = 0; i < utils::emuData.max_save_slots; i++)
				{
					if (ImGui::MenuItem(("Save slot " + std::to_string(i)).c_str()))
						utils::save_state(mAppInfo.cpu, i);
				}

				ImGui::EndPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Load state", ImVec2(ImGui::CalcTextSize("Load state").x + 14.f, topBarSize.y)))
			{
				ImGui::OpenPopup("Load state Menu");
			}

			if (ImGui::BeginPopup("Load state Menu"))
			{

				for (uint32_t i = 0; i < utils::emuData.max_save_slots; i++)
				{
					if (ImGui::MenuItem(("Save slot " + std::to_string(i)).c_str()))
						utils::load_state(mAppInfo.cpu, i);
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

		if (utils::emuData.ColorSelectWindow())
			mAppInfo.cpu->SetColorPalette(utils::emuData.paletteEdit);
	}

	Emulator::Emulator(CPU* cpu)
		: mCpu(cpu)
	{

	}

	Emulator::~Emulator()
	{

	}

	void Emulator::OnEvent(rdr::Event& e)
	{
		rdr::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
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

				if (utils::keyMap.contains(e.GetKeyCode()))
				{
					mCpu->InputPressed(utils::keyMap.at(e.GetKeyCode()));
					return true;
				}

				return false;
			});

		dispatcher.Dispatch<rdr::KeyReleasedEvent>([&](rdr::KeyReleasedEvent& e)
			{
				CPU* cpu = mCpu;
				if (utils::keyMap.contains(e.GetKeyCode()))
				{
					mCpu->InputReleased(utils::keyMap.at(e.GetKeyCode()));
					return true;
				}
				return false;
			});

	}

	void Emulator::Step(float ts)
	{
		if ((timeSinceLastUpdate += ts) >= (defaultSpeed / emulationSpeed))
		{
			timeSinceLastUpdate = 0.f;
			mCpu->Update();
		}

		// handle joypad input
		rdr::Joystick j = rdr::Renderer::GetJoystick();
		std::unordered_map<int32_t, Input> buttonMap = {
			{1, Input_Button_A},
			{2, Input_Button_B},
			{8, Input_Button_Select},
			{9, Input_Button_Start},
		};
		std::unordered_map<int32_t, Input> hatMap = {
			{1, Input_Button_Up},
			{-1, Input_Button_Down},
			{2, Input_Button_Right},
			{-2, Input_Button_Left},
		};
		if (j.IsEnabled())
		{
			for (auto& [k, v] : buttonMap)
			{
				if (j.GetButton(k))
					mCpu->InputPressed(v);
				else
					mCpu->InputReleased(v);
			}

			auto hat = j.GetHat();
			int32_t val = hat.y ? hat.y : 2 * hat.x;
			if (hatMap.contains(val))
			{
				mCpu->InputPressed(hatMap.at(val));
			}
			else
			{
				for (int32_t i = 4; i < 8; i++)
					mCpu->InputReleased((Input)i);
			}
		}
	}

	void Emulator::OnImGuiUpdate()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 vsize = viewport->Size;
		vsize.y -= topBarSize.y - footerSize.y;
		ImGui::SetNextWindowSize(vsize);

		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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

			for (float i = 1.f; i <= 5.f; i++)
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

	void Debugger::OnEvent(rdr::Event& e)
	{
		Emulator::OnEvent(e);

		if (e.handled)
			return;

		rdr::EventDispatcher dispatcher(e);

		dispatcher.Dispatch<rdr::KeyPressedEvent>([&](rdr::KeyPressedEvent& e)
			{
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
			});
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
				utils::imgui_uint16_t_setter(reg.reg, reg.buf, name);

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

			std::tuple<uint8_t*, const char*, std::array<char, 16>> memreg[] = {
				{mCpu->memory.memory + 0xff40, "LCD Control", {}},
				{mCpu->memory.memory + 0xff41, "LCD Status", {}},
				{mCpu->memory.memory + 0xff44, "LY", {}},
				{mCpu->memory.memory + 0xffff, "Interrupt Enable", {}},
				{mCpu->memory.memory + 0xff0f, "Interrupt Flag", {}},
			};

			for (auto& [reg, name, buf] : memreg)
				utils::imgui_uint8_t_setter(reg, buf.data(), name);

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