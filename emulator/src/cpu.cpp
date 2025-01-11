#include "emupch.h"
#include "cpu.h"

#include <renderer/time.h>

namespace utils
{
	uint8_t gameboyBootROM[256] = {
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB,
	0x21, 0x26, 0xFF, 0x0E, 0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3,
	0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0, 0x47, 0x11, 0x04, 0x01,
	0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22,
	0x23, 0x05, 0x20, 0xF9, 0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99,
	0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20, 0xF9, 0x2E, 0x0F, 0x18,
	0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20,
	0xF7, 0x1D, 0x20, 0xF2, 0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62,
	0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06, 0x7B, 0xE2, 0x0C, 0x3E,
	0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17,
	0xC1, 0xCB, 0x11, 0x17, 0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9,
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
	0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
	0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C, 0x21, 0x04, 0x01, 0x11,
	0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE,
	0x3E, 0x01, 0xE0, 0x50
	};
}

namespace emu
{
	

	CPU::CPU()
		: flags(AF.lo)
	{
		rdr::BufferConfiguration bufferConfig;
		bufferConfig.cpuVisible = true;
		bufferConfig.enableCopy = true;
		bufferConfig.persistentMap = true;
		bufferConfig.size = 256 * 256 * 4;
		bufferConfig.type = rdr::BufferType::StagingBuffer;

		stagingBuffer = new rdr::Buffer(bufferConfig);
		cpuBuffer.resize(256 * 256);

		rdr::TextureConfiguration textureConfig;
		textureConfig.format = rdr::TextureFormat(4, rdr::eDataType::Ufloat8);
		textureConfig.size = { 256, 256 };
		textureConfig.type.Set<rdr::TextureType::copyEnable>(true);
		
		displayTexture = new rdr::Texture(textureConfig);

		memset(cpuBuffer.data(), 0xff, cpuBuffer.size() * 4);
		memcpy_s(stagingBuffer->GetData(), stagingBuffer->GetConfig().size, cpuBuffer.data(), cpuBuffer.size() * 4);
		displayTexture->SetData(stagingBuffer);
	}

	CPU::~CPU()
	{
		delete stagingBuffer;
		delete displayTexture;
	}

	void CPU::LoadRom(const char* file)
	{
		std::ifstream bootrom(file, std::ios::binary | std::ios::in);
		size_t size = std::filesystem::file_size(file);

		bootrom.read((char*)memory.memory, std::min(size, (size_t)std::numeric_limits<uint16_t>::max()));

		memcpy_s(memory.memory, sizeof(memory.memory), utils::gameboyBootROM, sizeof(utils::gameboyBootROM));

	}

	void CPU::Pause()
	{
		running = false;
	}

	void CPU::Reset()
	{
		pc = 0x100;

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
			memory.memory[0xFF45] = 0x00;
			memory.memory[0xFF47] = 0xFC;
			memory.memory[0xFF48] = 0xFF;
			memory.memory[0xFF49] = 0xFF;
			memory.memory[0xFF4A] = 0x00;
			memory.memory[0xFF4B] = 0x00;
			memory.memory[0xFFFF] = 0x00;
		}

		running = true;
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

		uint32_t stepCycle = 1, totalCycles = 1;

		while (totalCycles < (frequency / 60))
		{
			stepCycle = 1;

			if (!flags.halt)
				stepCycle = step();

			if (flags.enableImeCountdown > 0)
			{
				if ((--flags.enableImeCountdown) == 0)
					flags.ime = true;
			}

			updateTimer(stepCycle);
			handleInterrupts();

			totalCycles += stepCycle;
		}

		setTextureData();
	}

	void CPU::setTextureData()
	{
		uint32_t bgTileData = memory[0xff40] & (1 << 4) ? 0x8000 : 0x8800;
		uint32_t bgTileMap = memory[0xff40] & (1 << 3) ? 0x9c00 : 0x9800;

		// tmp
		uint32_t color_palette[] = {
			0xffffffff,
			0xff606060,
			0xff303030,
			0xff010101
		};

		for (uint32_t i = 0; i < 256; i++)
		{
			for (uint32_t j = 0; j < 256; j++)
			{
				uint32_t tileNumber = memory[bgTileMap + ((i / 8) * 4) + (j / 8)]; // 32 x 32 byte tilemap -> 8 x 8 pixels per tile

				//  every tile is represented by 16 bytes;
				//  every 2 consecutive bytes represents a row (8 pixels)
				//  the palette to use (0 - 3) is given by taking the corresponding bits from the 2 bytes
				//  eg. for 2nd pixel from the left take bit 6 from both bytes and flip them
				//  0x8800 represents signed indexing

				uint32_t location = 
					bgTileData == 0x8000 ? 
					bgTileData + (tileNumber * 0x10) + ((i % 8) * 2) :
					bgTileData + ((int8_t)tileNumber * 0x10) + ((i % 8) * 2); // signed indexing

				uint16_t tileData = memory.As<uint16_t>(location); // tile row

				uint32_t index = 0;
				index = ((tileData << (j % 8)) & (0x8000)) ? 0x1 : 0x0;
				index |= ((tileData << (j & 8)) & (0x0080)) ? 0x2 : 0x0;

				RDR_ASSERT_MSG_BREAK(index < 4, "Index wrong");
				cpuBuffer[i * 256 + j] = color_palette[index];
			}
		}

		memcpy_s(stagingBuffer->GetData(), stagingBuffer->GetConfig().size, cpuBuffer.data(), cpuBuffer.size() * 4);
		displayTexture->SetData(stagingBuffer, false);
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
		static uint32_t divCycle = 0, timerCycles = 0;
		
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

			while (timerCycles >= freq)
			{
				memory.memory[0xff05]++;
				if (memory.memory[0xff05] == 0x0)
				{
					memory.memory[0xff05] = memory.memory[0xff06];
					memory.memory[0xff0f] |= 4;
				}
				timerCycles -= freq;
			}
		}
	}
}