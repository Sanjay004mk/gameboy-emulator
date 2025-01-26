#include "emupch.h"
#include "cpu.h"

#include <renderer/time.h>

namespace emu
{
	CPU::CPU()
		: flags(AF.lo), ppu(memory)
	{
	}

	CPU::~CPU()
	{
	}

	void CPU::LoadRom(const char* file)
	{
		memory.Init(file);
	}

	void CPU::Pause()
	{
		running = false;
	}

	void CPU::Reset()
	{
		memory.LoadBootRom();

		pc = 0x0;

		AF.b16 = 0x01b0;
		BC.b16 = 0x0013;
		DE.b16 = 0x00d8;
		HL.b16 = 0x014d;
		sp = 0xfffe;

		{
			memory.memory[0xFF05] = 0x00;
			memory.memory[0xFF06] = 0x00;
			memory.memory[0xFF07] = 0x00;
			memory.memory[0xFF10] = 0x80;
			memory.memory[0xFF11] = 0xBF;
			memory.memory[0xFF12] = 0xF3;
			memory.memory[0xFF14] = 0xBF;
			memory.memory[0xFF16] = 0x3F;
			memory.memory[0xFF17] = 0x00;
			memory.memory[0xFF19] = 0xBF;
			memory.memory[0xFF1A] = 0x7F;
			memory.memory[0xFF1B] = 0xFF;
			memory.memory[0xFF1C] = 0x9F;
			memory.memory[0xFF1E] = 0xBF;
			memory.memory[0xFF20] = 0xFF;
			memory.memory[0xFF21] = 0x00;
			memory.memory[0xFF22] = 0x00;
			memory.memory[0xFF23] = 0xBF;
			memory.memory[0xFF24] = 0x77;
			memory.memory[0xFF25] = 0xF3;
			memory.memory[0xFF26] = 0xF1;
			memory.memory[0xFF40] = 0x91;
			memory.memory[0xFF42] = 0x00;
			memory.memory[0xFF43] = 0x00;
			memory.memory[0xFF44] = 0x90;
			memory.memory[0xFF45] = 0x00;
			memory.memory[0xFF47] = 0xFC;
			memory.memory[0xFF48] = 0xFF;
			memory.memory[0xFF49] = 0xFF;
			memory.memory[0xFF4A] = 0x00;
			memory.memory[0xFF4B] = 0x00;
			memory.memory[0xFFFF] = 0x00;

			memory.memory[0xff30] = 0x84;
			memory.memory[0xff31] = 0x40;
			memory.memory[0xff32] = 0x43;
			memory.memory[0xff33] = 0xaa;
			memory.memory[0xff34] = 0x2d;
			memory.memory[0xff35] = 0x78;
			memory.memory[0xff36] = 0x92;
			memory.memory[0xff37] = 0x3c;
			memory.memory[0xff38] = 0x60;
			memory.memory[0xff39] = 0x59;
			memory.memory[0xff3a] = 0x59;
			memory.memory[0xff3b] = 0xb0;
			memory.memory[0xff3c] = 0x34;
			memory.memory[0xff3d] = 0xb8;
			memory.memory[0xff3e] = 0x2e;
			memory.memory[0xff3f] = 0xda;
		}

		running = true;
		divCycle = 0;
		timerCycles = 0;
	}

	void CPU::Resume()
	{
		running = true;
	}

	void CPU::Toggle()
	{
		running = !running;
	}

	void CPU::LoadAndStart(const char* file)
	{
		LoadRom(file);
		Reset();
	}

	void CPU::Update()
	{
		if (!running)
			return;

		uint32_t totalCycles = 0;

		while (totalCycles < (frequency / 60))
			totalCycles += SingleStepUpdate();
	}

	uint32_t CPU::SingleStepUpdate()
	{
		uint32_t stepCycle = 4;

		if (!flags.halt)
			stepCycle = step();

		if (memory[0xff02] == 0x81) {
			serialPresent = true;
			char c = memory[0xff01];
			serialData += c;
			memory.memory[0xff02] = 0x0;
		}

		ppu.step(stepCycle);

		updateTimer(stepCycle);
		handleInterrupts();

		return stepCycle;
	}

	void CPU::handleInterrupts()
	{
		if ((memory[0xffff] & memory[0xff0f]) && flags.halt)
			flags.halt = false;

		if (flags.ime)
		{
			uint8_t interrupts = memory[0xffff] & memory[0xff0f];

			uint32_t index = 0;
			while (interrupts && index < 5)
			{
				if (interrupts & 0x1)
				{
					flags.ime = false;
					sp -= 2;
					memory.SetAs<uint16_t>(sp, pc);
					memory(0xff0f, memory[0xff0f] & ~(1 << index));

					switch (index)
					{
					case 0: // V-Blank
						pc = 0x40;
						break;
					case 1: // LCD STAT
						pc = 0x48;
						break;
					case 2: // Timer
						pc = 0x50;
						break;
					case 3: // Serial
						pc = 0x58;
						break;
					case 4: // Joypad
						pc = 0x60;
						break;
					}

					break; // interrupt handled
				}
				index++;
				interrupts >>= 1;
			}

		}
	}

	void CPU::updateTimer(uint32_t cycles)
	{
		// increment divider clock every 256 cycles <=> 16384 times / sec
		if ((divCycle += cycles) >= 256)
		{
			divCycle -= 256;
			memory.memory[0xff04]++;
		}

		if (memory.memory[0xff07] & 0x4) // Timer enabled
		{
			timerCycles += cycles;

			uint32_t rate = memory.memory[0xff07] & 3;
			uint32_t freq = 0;
			switch (rate)
			{
			case 0:
				freq = 1024; 
				break;
			case 1:
				freq = 16;
				break;
			case 2:
				freq = 64;
				break;
			case 3:
				freq = 256;
				break;
			}

			while (timerCycles > freq)
			{
				memory.memory[0xff05]++;
				if (memory.memory[0xff05] == 0x00)
				{
					memory.memory[0xff05] = memory.memory[0xff06];
					memory.memory[0xff0f] |= 4;
				}

				timerCycles -= freq;
			}
		}
	}

	std::string CPU::SerialOut()
	{
		std::string ret = serialData;
		serialPresent = false;
		serialData = "";
		return ret;
	}
}