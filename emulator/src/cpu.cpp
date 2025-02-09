#include "emupch.h"
#include "cpu.h"

#include <renderer/time.h>

namespace emu
{
	CPU::CPU()
		: flags(AF.lo), memory([this](Input ip) { return this->GetInputState(ip); }), ppu(memory, [this]() { vblank = true; }), spu(memory)
	{
		memory.spuWriteCallback = [this](uint32_t adr, uint8_t v) { this->spu.WriteCallback(adr, v); };
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

	void CPU::Reset(bool withBootRom)
	{
		pc = withBootRom ? 0x0 : 0x100;

		AF.b16 = 0x01b0;
		BC.b16 = 0x0013;
		DE.b16 = 0x00d8;
		HL.b16 = 0x014d;
		sp = 0xfffe;

		memory.Reset();

		if (withBootRom)
			memory.LoadBootRom();

		running = true;
		divCycle = 0;
		timerCycles = 0;

		flags.ime = false;
		flags.halt = false;
	}

	void CPU::Resume()
	{
		running = true;
	}

	void CPU::Toggle()
	{
		running = !running;
	}

	void CPU::LoadAndStart(const char* file, bool withBootRom)
	{
		LoadRom(file);
		Reset(withBootRom);
	}

	void CPU::Update()
	{
		if (!running)
			return;

		uint32_t totalCycles = 0;

		vblank = false;
		while (!vblank)
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
		spu.step(stepCycle);

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

	void CPU::InputPressed(Input ip)
	{
		inputState |= (1 << ip);
	}

	void CPU::InputReleased(Input ip)
	{
		inputState &= ~(1 << ip);
	}

	bool CPU::GetInputState(Input ip)
	{
		return (bool)(inputState & (1 << ip));
	}

	std::string CPU::SerialOut()
	{
		std::string ret = serialData;
		serialPresent = false;
		serialData = "";
		return ret;
	}
}