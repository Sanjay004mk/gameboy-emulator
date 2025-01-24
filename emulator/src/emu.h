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

	private:
		static void ImGuiBasicUI();

		virtual void ImGuiDefaultLayout(uint32_t& dockspace);
		virtual void ImGuiMenuBarOptions();
		virtual void ImGuiFooterOptions(const glm::vec2& region);

		CPU* mCpu;

		struct ApplicationInformation
		{
			std::vector<Emulator> emulators;
			uint32_t currentEmulator;

			rdr::Window* window;
			emu::CPU* cpu;

			Emulator& GetCurrentEmulator() { return emulators[currentEmulator]; }
		};
		
		static inline ApplicationInformation mAppInfo;
	};
}