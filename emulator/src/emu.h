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
		virtual bool OnKeyPressed(rdr::KeyPressedEvent& e);

		static void StartEmulator(int argc, const char** argv);

	protected:
		virtual void ImGuiSetupDockspace();
		virtual void ImGuiMenuBarOptions();
		virtual void ImGuiFooterOptions(const glm::vec2& region);

		CPU* mCpu = nullptr;
		bool buildDockspace = true;

	private:
		static void ImGuiBasicUI();

		struct ApplicationInformation
		{
			std::vector<Emulator*> emulators;
			uint32_t currentEmulator = 1;

			rdr::Window* window = nullptr;
			emu::CPU* cpu = nullptr;

			Emulator& GetCurrentEmulator() { return *(emulators[currentEmulator]); }
		};
		
		static inline ApplicationInformation mAppInfo;
	};

	class Debugger : public Emulator
	{
	public:
		Debugger(CPU* cpu);
		~Debugger();

		void Step(float timestep) override;
		bool OnKeyPressed(rdr::KeyPressedEvent& e) override;

	private:
		void OnImGuiUpdate();
		void ImGuiSetupDockspace() override;
		void ImGuiMenuBarOptions() override;
		void ImGuiFooterOptions(const glm::vec2& region) override;

		std::string consoleOutput;
	};
}