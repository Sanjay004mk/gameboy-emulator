#pragma once

#include <renderer/rdr.h>

#include "cpu.h"

namespace emu
{
	class Emulator
	{
	public:
		Emulator(CPU* cpu);
		~Emulator();

		virtual void Step(float timestep);
		virtual void OnEvent(rdr::Event& e);
		virtual void HandleJoypad();

		static void StartEmulator(int argc, const char** argv);

	protected:
		virtual void OnDetach() {}
		virtual void OnAttach() {}

		virtual void OnImGuiUpdate();

		virtual void ImGuiSetupDockspace();
		virtual void ImGuiMenuBarOptions();
		virtual void ImGuiFooterOptions(const glm::vec2& region);

		CPU* mCpu = nullptr;
		bool buildDockspace = true;
		bool useJoypad = true;
		float emulationSpeed = 1.f;
		float timeSinceLastUpdate = 0.f;

		static inline constexpr float defaultSpeed = (1.f / 60.f);
		struct ApplicationInformation
		{
			std::vector<Emulator*> emulators;
			uint32_t currentEmulator = 1;

			rdr::Window* window = nullptr;
			emu::CPU* cpu = nullptr;

			void SetEmulator(uint32_t index)
			{
				GetCurrentEmulator().OnDetach();

				currentEmulator = index;
				
				GetCurrentEmulator().buildDockspace = true;
				GetCurrentEmulator().OnAttach();
			}

			Emulator& GetCurrentEmulator() { return *(emulators[currentEmulator]); }
		};
		
		static inline ApplicationInformation mAppInfo;

	private:
		static void ImGuiBasicUI();
	};

	class Debugger : public Emulator
	{
	public:
		Debugger(CPU* cpu);
		~Debugger();

		void Step(float timestep) override;
		void OnEvent(rdr::Event& e) override;

	private:
		void OnImGuiUpdate() override;
		void ImGuiSetupDockspace() override;
		void ImGuiMenuBarOptions() override;
		void ImGuiFooterOptions(const glm::vec2& region) override;

		void UpdateMemoryView();

		void OnAttach() override { mCpu->ppu.SetDebugMode(true); }
		void OnDetach() override { mCpu->ppu.SetDebugMode(false); }

		std::string consoleOutput;
		bool consoleScrollToBottom = true;

		enum class Mode { Run = 0, Step = 1 };

		Mode emulationMode = Mode::Step;
		bool shouldStep = false;
		std::vector<std::string> memoryDisplay;
	};
}