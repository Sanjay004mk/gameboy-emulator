#include "emupch.h"
#include "cpu.h"

#include <renderer/time.h>
#include <iomanip>
//
//#define LOAD_BLARGG_ROMS

namespace utils
{
#if defined(LOAD_BLARGG_ROMS)
	static std::string op;
	static std::vector<std::string> roms;
	static uint32_t idx = 0;

#endif

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

#if defined(LOAD_BLARGG_ROMS)
		{
			for (auto& entry : std::filesystem::directory_iterator("gb-test-roms-master/cpu_instrs/individual"))
				utils::roms.push_back(entry.path().string());

			LoadRom(utils::roms[utils::idx++].c_str());
			pc = 0x100;
		}
#endif

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

#if defined(LOAD_BLARGG_ROMS)
			if (memory[0xff02] == 0x81)
			{
				std::cout << memory.As<char>(0xff01);
				utils::op += memory.As<char>(0xff01);
				memory(0xff02, 0);
			}
#endif

			if (flags.enableImeCountdown > 0)
			{
				if ((--flags.enableImeCountdown) == 0)
					flags.ime = true;
			}

			updateTimer(stepCycle);
			handleInterrupts();

#if defined(LOAD_BLARGG_ROMS)
			if (utils::op.contains("Passed\n"))
			{
				memset(memory.memory, 0, sizeof(memory.memory));

				if (utils::idx >= utils::roms.size())
					return;
				LoadRom(utils::roms[utils::idx++].c_str());
				pc = 0x100;

				utils::op = "";
			}
#endif

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

	uint32_t CPU::step()
	{
		uint8_t opcode = memory[pc++];
		uint32_t cycles = 0;

#define HANDLE_OP(opcode, fn) \
case opcode:\
{\
fn \
}\
break;

		switch (opcode)
		{
			HANDLE_OP(0xcb, {
				cycles = prefix();
				});

			HANDLE_OP(0x00, {
				cycles = 4;
				});
			HANDLE_OP(0x01, {
				cycles = ld(BC.b16);
				});
			HANDLE_OP(0x02, {
				cycles = str(BC.b16, AF.hi);
				});
			HANDLE_OP(0x03, {
				cycles = inc(BC.b16);
				});
			HANDLE_OP(0x04, {
				cycles = inc(BC.hi);
				});
			HANDLE_OP(0x05, {
				cycles = dec(BC.hi);
				});
			HANDLE_OP(0x06, {
				cycles = ld(BC.hi);
				});
			HANDLE_OP(0x07, {
				cycles = (rlc<uint8_t, true>(AF.hi));
				});
			HANDLE_OP(0x08, {
				memory.SetAs<uint16_t>(memory.As<uint16_t>(pc), sp);
				pc += 2;
				cycles = 20;
				});
			HANDLE_OP(0x09, {
				cycles = add(HL.b16, BC.b16);
				});
			HANDLE_OP(0x0a, {
				cycles = ldr(AF.hi, BC.b16);
				});
			HANDLE_OP(0x0b, {
				cycles = dec(BC.b16);
				});
			HANDLE_OP(0x0c, {
				cycles = inc(BC.lo);
				});
			HANDLE_OP(0x0d, {
				cycles = dec(BC.lo);
				});
			HANDLE_OP(0x0e, {
				cycles = ld(BC.lo);
				});
			HANDLE_OP(0x0f, {
				cycles = (rrc<uint8_t, true>(AF.hi));
				});

			HANDLE_OP(0x10, {
				// TODO stop emulation (maybe?) istr: STOP
				cycles = 4;
				});
			HANDLE_OP(0x11, {
				cycles = ld(DE.b16);
				});
			HANDLE_OP(0x12, {
				cycles = str(DE.b16, AF.hi);
				});
			HANDLE_OP(0x13, {
				cycles = inc(DE.b16);
				});
			HANDLE_OP(0x14, {
				cycles = inc(DE.hi);
				});
			HANDLE_OP(0x15, {
				cycles = dec(DE.hi);
				});
			HANDLE_OP(0x16, {
				cycles = ld(DE.hi);
				});
			HANDLE_OP(0x17, {
				cycles = (rl<uint8_t, true>(AF.hi));
				});
			HANDLE_OP(0x18, {
				cycles = jr();
				});
			HANDLE_OP(0x19, {
				cycles = add(HL.b16, DE.b16);
				});
			HANDLE_OP(0x1a, {
				cycles = ldr(AF.hi, DE.b16);
				});
			HANDLE_OP(0x1b, {
				cycles = dec(DE.b16);
				});
			HANDLE_OP(0x1c, {
				cycles = inc(DE.lo);
				});
			HANDLE_OP(0x1d, {
				cycles = dec(DE.lo);
				});
			HANDLE_OP(0x1e, {
				cycles = ld(DE.lo);
				});
			HANDLE_OP(0x1f, {
				cycles = (rr<uint8_t, true>(AF.hi));
				});

			HANDLE_OP(0x20, {
				cycles = flags.z() ? jr<false>() : jr();
				});
			HANDLE_OP(0x21, {
				cycles = ld(HL.b16);
				});
			HANDLE_OP(0x22, {
				cycles = (str<uint8_t, true, false>(HL.b16, AF.hi));
				});
			HANDLE_OP(0x23, {
				cycles = inc(HL.b16);
				});
			HANDLE_OP(0x24, {
				cycles = inc(HL.hi);
				});
			HANDLE_OP(0x025, {
				cycles = dec(HL.hi);
				});
			HANDLE_OP(0x26, {
				cycles = ld(HL.hi);
				});
			HANDLE_OP(0x27, {
				cycles = daa();
				});
			HANDLE_OP(0x28, {
				cycles = flags.z() ? jr() : jr<false>();
				});
			HANDLE_OP(0x29, {
				cycles = add(HL.b16, HL.b16);
				});
			HANDLE_OP(0x2a, {
				cycles = (ldr<uint8_t, true, false>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x2b, {
				cycles = dec(HL.b16);
				});
			HANDLE_OP(0x2c, {
				cycles = inc(HL.lo);
				});
			HANDLE_OP(0x2d, {
				cycles = dec(HL.lo);
				});
			HANDLE_OP(0x2e, {
				cycles = ld(HL.lo);
				});
			HANDLE_OP(0x2f, {
				cycles = cpl();
				});

			HANDLE_OP(0x30, {
				cycles = flags.c() ? jr<false>() : jr();
				});
			HANDLE_OP(0x31, {
				cycles = ld(sp);
				});
			HANDLE_OP(0x32, {
				cycles = (str<uint8_t, false, true>(HL.b16, AF.hi));
				});
			HANDLE_OP(0x33, {
				cycles = inc(sp);
				});
			HANDLE_OP(0x34, {
				cycles = (inc<uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x35, {
				cycles = (dec<uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x36, {
				cycles = stc(HL.b16);
				});
			HANDLE_OP(0x37, {
				cycles = scf();
				});
			HANDLE_OP(0x38, {
				cycles = flags.c() ? jr() : jr<false>();
				});
			HANDLE_OP(0x39, {
				cycles = add(HL.b16, sp);
				});
			HANDLE_OP(0x3a, {
				cycles = (ldr<uint8_t, false, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x3b, {
				cycles = dec(sp);
				});
			HANDLE_OP(0x3c, {
				cycles = inc(AF.hi);
				});
			HANDLE_OP(0x3d, {
				cycles = dec(AF.hi);
				});
			HANDLE_OP(0x3e, {
				cycles = ld(AF.hi);
				});
			HANDLE_OP(0x3f, {
				cycles = ccf();
				});

			HANDLE_OP(0x40, {
				cycles = mv(BC.hi, BC.hi);
				});
			HANDLE_OP(0x41, {
				cycles = mv(BC.hi, BC.lo);
				});
			HANDLE_OP(0x42, {
				cycles = mv(BC.hi, DE.hi);
				});
			HANDLE_OP(0x43, {
				cycles = mv(BC.hi, DE.lo);
				});
			HANDLE_OP(0x44, {
				cycles = mv(BC.hi, HL.hi);
				});
			HANDLE_OP(0x45, {
				cycles = mv(BC.hi, HL.lo);
				});
			HANDLE_OP(0x46, {
				cycles = ldr(BC.hi, HL.b16);
				});
			HANDLE_OP(0x47, {
				cycles = mv(BC.hi, AF.hi);
				});
			HANDLE_OP(0x48, {
				cycles = mv(BC.lo, BC.hi);
				});
			HANDLE_OP(0x49, {
				cycles = mv(BC.lo, BC.lo);
				});
			HANDLE_OP(0x4a, {
				cycles = mv(BC.lo, DE.hi);
				});
			HANDLE_OP(0x4b, {
				cycles = mv(BC.lo, DE.lo);
				});
			HANDLE_OP(0x4c, {
				cycles = mv(BC.lo, HL.hi);
				});
			HANDLE_OP(0x4d, {
				cycles = mv(BC.lo, HL.lo);
				});
			HANDLE_OP(0x4e, {
				cycles = ldr(BC.lo, HL.b16);
				});
			HANDLE_OP(0x4f, {
				cycles = mv(BC.lo, AF.hi);
				});

			HANDLE_OP(0x50, {
				cycles = mv(DE.hi, BC.hi);
				});
			HANDLE_OP(0x51, {
				cycles = mv(DE.hi, BC.lo);
				});
			HANDLE_OP(0x52, {
				cycles = mv(DE.hi, DE.hi);
				});
			HANDLE_OP(0x53, {
				cycles = mv(DE.hi, DE.lo);
				});
			HANDLE_OP(0x54, {
				cycles = mv(DE.hi, HL.hi);
				});
			HANDLE_OP(0x55, {
				cycles = mv(DE.hi, HL.lo);
				});
			HANDLE_OP(0x56, {
				cycles = ldr(DE.hi, HL.b16);
				});
			HANDLE_OP(0x57, {
				cycles = mv(DE.hi, AF.hi);
				});
			HANDLE_OP(0x58, {
				cycles = mv(DE.lo, BC.hi);
				});
			HANDLE_OP(0x59, {
				cycles = mv(DE.lo, BC.lo);
				});
			HANDLE_OP(0x5a, {
				cycles = mv(DE.lo, DE.hi);
				});
			HANDLE_OP(0x5b, {
				cycles = mv(DE.lo, DE.lo);
				});
			HANDLE_OP(0x5c, {
				cycles = mv(DE.lo, HL.hi);
				});
			HANDLE_OP(0x5d, {
				cycles = mv(DE.lo, HL.lo);
				});
			HANDLE_OP(0x5e, {
				cycles = ldr(DE.lo, HL.b16);
				});
			HANDLE_OP(0x5f, {
				cycles = mv(DE.lo, AF.hi);
				});

			HANDLE_OP(0x60, {
				cycles = mv(HL.hi, BC.hi);
				});
			HANDLE_OP(0x61, {
				cycles = mv(HL.hi, BC.lo);
				});
			HANDLE_OP(0x62, {
				cycles = mv(HL.hi, DE.hi);
				});
			HANDLE_OP(0x63, {
				cycles = mv(HL.hi, DE.lo);
				});
			HANDLE_OP(0x64, {
				cycles = mv(HL.hi, HL.hi);
				});
			HANDLE_OP(0x65, {
				cycles = mv(HL.hi, HL.lo);
				});
			HANDLE_OP(0x66, {
				cycles = ldr(HL.hi, HL.b16);
				});
			HANDLE_OP(0x67, {
				cycles = mv(HL.hi, AF.hi);
				});
			HANDLE_OP(0x68, {
				cycles = mv(HL.lo, BC.hi);
				});
			HANDLE_OP(0x69, {
				cycles = mv(HL.lo, BC.lo);
				});
			HANDLE_OP(0x6a, {
				cycles = mv(HL.lo, DE.hi);
				});
			HANDLE_OP(0x6b, {
				cycles = mv(HL.lo, DE.lo);
				});
			HANDLE_OP(0x6c, {
				cycles = mv(HL.lo, HL.hi);
				});
			HANDLE_OP(0x6d, {
				cycles = mv(HL.lo, HL.lo);
				});
			HANDLE_OP(0x6e, {
				cycles = ldr(HL.lo, HL.b16);
				});
			HANDLE_OP(0x6f, {
				cycles = mv(HL.lo, AF.hi);
				});

			HANDLE_OP(0x70, {
				cycles = str(HL.b16, BC.hi);
				});
			HANDLE_OP(0x71, {
				cycles = str(HL.b16, BC.lo);
				});
			HANDLE_OP(0x72, {
				cycles = str(HL.b16, DE.hi);
				});
			HANDLE_OP(0x73, {
				cycles = str(HL.b16, DE.lo);
				});
			HANDLE_OP(0x74, {
				cycles = str(HL.b16, HL.hi);
				});
			HANDLE_OP(0x75, {
				cycles = str(HL.b16, HL.lo);
				});
			HANDLE_OP(0x76, {
				flags.halt = true;
				cycles = 4; 
				});
			HANDLE_OP(0x77, {
				cycles = str(HL.b16, AF.hi);
				});
			HANDLE_OP(0x78, {
				cycles = mv(AF.hi, BC.hi);
				});
			HANDLE_OP(0x79, {
				cycles = mv(AF.hi, BC.lo);
				});
			HANDLE_OP(0x7a, {
				cycles = mv(AF.hi, DE.hi);
				});
			HANDLE_OP(0x7b, {
				cycles = mv(AF.hi, DE.lo);
				});
			HANDLE_OP(0x7c, {
				cycles = mv(AF.hi, HL.hi);
				});
			HANDLE_OP(0x7d, {
				cycles = mv(AF.hi, HL.lo);
				});
			HANDLE_OP(0x7e, {
				cycles = ldr(AF.hi, HL.b16);
				});
			HANDLE_OP(0x7f, {
				cycles = mv(AF.hi, AF.hi);
				});

			HANDLE_OP(0x80, {
				cycles = add(AF.hi, BC.hi);
				});
			HANDLE_OP(0x81, {
				cycles = add(AF.hi, BC.lo);
				});
			HANDLE_OP(0x82, {
				cycles = add(AF.hi, DE.hi);
				});
			HANDLE_OP(0x83, {
				cycles = add(AF.hi, DE.lo);
				})
			HANDLE_OP(0x84, {
				cycles = add(AF.hi, HL.hi);
				});
			HANDLE_OP(0x85, {
				cycles = add(AF.hi, HL.lo);
				});
			HANDLE_OP(0x86, {
				cycles = (add<uint8_t, false, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x87, {
				cycles = add(AF.hi, AF.hi);
				});
			HANDLE_OP(0x88, {
				cycles = (add<uint8_t, true>(AF.hi, BC.hi));
				});
			HANDLE_OP(0x89, {
				cycles = (add<uint8_t, true>(AF.hi, BC.lo));
				});
			HANDLE_OP(0x8a, {
				cycles = (add<uint8_t, true>(AF.hi, DE.hi));
				});
			HANDLE_OP(0x8b, {
				cycles = (add<uint8_t, true>(AF.hi, DE.lo));
				});
			HANDLE_OP(0x8c, {
				cycles = (add<uint8_t, true>(AF.hi, HL.hi));
				});
			HANDLE_OP(0x8d, {
				cycles = (add<uint8_t, true>(AF.hi, HL.lo));
				});
			HANDLE_OP(0x8e, {
				cycles = (add<uint8_t, true, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x8f, {
				cycles = (add<uint8_t, true>(AF.hi, AF.hi));
				});

			HANDLE_OP(0x90, {
				cycles = sub(AF.hi, BC.hi);
				});
			HANDLE_OP(0x91, {
				cycles = sub(AF.hi, BC.lo);
				});
			HANDLE_OP(0x92, {
				cycles = sub(AF.hi, DE.hi);
				});
			HANDLE_OP(0x93, {
				cycles = sub(AF.hi, DE.lo);
				});
			HANDLE_OP(0x94, {
				cycles = sub(AF.hi, HL.hi);
				});
			HANDLE_OP(0x95, {
				cycles = sub(AF.hi, HL.lo);
				});
			HANDLE_OP(0x96, {
				cycles = (sub<uint8_t, false, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x97, {
				cycles = sub(AF.hi, AF.hi);
				});
			HANDLE_OP(0x98, {
				cycles = (sub<uint8_t, true>(AF.hi, BC.hi));
				});
			HANDLE_OP(0x99, {
				cycles = (sub<uint8_t, true>(AF.hi, BC.lo));
				});
			HANDLE_OP(0x9a, {
				cycles = (sub<uint8_t, true>(AF.hi, DE.hi));
				});
			HANDLE_OP(0x9b, {
				cycles = (sub<uint8_t, true>(AF.hi, DE.lo));
				});
			HANDLE_OP(0x9c, {
				cycles = (sub<uint8_t, true>(AF.hi, HL.hi));
				});
			HANDLE_OP(0x9d, {
				cycles = (sub<uint8_t, true>(AF.hi, HL.lo));
				});
			HANDLE_OP(0x9e, {
				cycles = (sub<uint8_t, true, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x9f, {
				cycles = (sub<uint8_t, true>(AF.hi, AF.hi));
				});

			HANDLE_OP(0xa0, {
				cycles = opand(AF.hi, BC.hi);
				});
			HANDLE_OP(0xa1, {
				cycles = opand(AF.hi, BC.lo);
				});
			HANDLE_OP(0xa2, {
				cycles = opand(AF.hi, DE.hi);
				});
			HANDLE_OP(0xa3, {
				cycles = opand(AF.hi, DE.lo);
				});
			HANDLE_OP(0xa4, {
				cycles = opand(AF.hi, HL.hi);
				});
			HANDLE_OP(0xa5, {
				cycles = opand(AF.hi, HL.lo);
				});
			HANDLE_OP(0xa6, {
				cycles = (opand<uint8_t, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0xa7, {
				cycles = opand(AF.hi, AF.hi);
				});
			HANDLE_OP(0xa8, {
				cycles = opxor(AF.hi, BC.hi);
				});
			HANDLE_OP(0xa9, {
				cycles = opxor(AF.hi, BC.lo);
				});
			HANDLE_OP(0xaa, {
				cycles = opxor(AF.hi, DE.hi);
				});
			HANDLE_OP(0xab, {
				cycles = opxor(AF.hi, DE.lo);
				});
			HANDLE_OP(0xac, {
				cycles = opxor(AF.hi, HL.hi);
				});
			HANDLE_OP(0xad, {
				cycles = opxor(AF.hi, HL.lo);
				});
			HANDLE_OP(0xae, {
				cycles = (opxor<uint8_t, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0xaf, {
				cycles = opxor(AF.hi, AF.hi);
				});

			HANDLE_OP(0xb0, {
				cycles = opor(AF.hi, BC.hi);
				});
			HANDLE_OP(0xb1, {
				cycles = opor(AF.hi, BC.lo);
				});
			HANDLE_OP(0xb2, {
				cycles = opor(AF.hi, DE.hi);
				});
			HANDLE_OP(0xb3, {
				cycles = opor(AF.hi, DE.lo);
				});
			HANDLE_OP(0xb4, {
				cycles = opor(AF.hi, HL.hi);
				});
			HANDLE_OP(0xb5, {
				cycles = opor(AF.hi, HL.lo);
				});
			HANDLE_OP(0xb6, {
				cycles = (opor<uint8_t, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0xb7, {
				cycles = opor(AF.hi, AF.hi);
				});
			HANDLE_OP(0xb8, {
				cycles = cp(AF.hi, BC.hi);
				});
			HANDLE_OP(0xb9, {
				cycles = cp(AF.hi, BC.lo);
				});
			HANDLE_OP(0xba, {
				cycles = cp(AF.hi, DE.hi);
				});
			HANDLE_OP(0xbb, {
				cycles = cp(AF.hi, DE.lo);
				});
			HANDLE_OP(0xbc, {
				cycles = cp(AF.hi, HL.hi);
				});
			HANDLE_OP(0xbd, {
				cycles = cp(AF.hi, HL.lo);
				});
			HANDLE_OP(0xbe, {
				cycles = (cp<uint8_t, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0xbf, {
				cycles = cp(AF.hi, AF.hi);
				});

			HANDLE_OP(0xc0, {
				cycles = flags.z() ? ret<false>() : ret();
				});
			HANDLE_OP(0xc1, {
				cycles = pop(BC.b16);
				});
			HANDLE_OP(0xc2, {
				cycles = flags.z() ? jp<false>() : jp();
				});
			HANDLE_OP(0xc3, {
				cycles = jp();
				});
			HANDLE_OP(0xc4, {
				cycles = flags.z() ? call<false>() : call();
				});
			HANDLE_OP(0xc5, {
				cycles = push(BC.b16);
				});
			HANDLE_OP(0xc6, {
				cycles = (add<uint8_t, false, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xc7, {
				cycles = rst<0x00>();
				});
			HANDLE_OP(0xc8, {
				cycles = flags.z() ? ret() : ret<false>();
				});
			HANDLE_OP(0xc9, {
				cycles = (ret<true, false>());
				});
			HANDLE_OP(0xca, {
				cycles = flags.z() ? jp() : jp<false>();
				});
			HANDLE_OP(0xcc, {
				cycles = flags.z() ? call() : call<false>();
				});
			HANDLE_OP(0xcd, {
				cycles = call();
				});
			HANDLE_OP(0xce, {
				cycles = (add<uint8_t, true, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xcf, {
				cycles = rst<0x08>();
				});

			HANDLE_OP(0xd0, {
				cycles = flags.c() ? ret<false>() : ret();
				});
			HANDLE_OP(0xd1, {
				cycles = pop(DE.b16);
				});
			HANDLE_OP(0xd2, {
				cycles = flags.c() ? jp<false>() : jp();
				});
			HANDLE_OP(0xd3, {
				RDR_LOG_ERROR("Invalid opcode 0xD3");
				});
			HANDLE_OP(0xd4, {
				cycles = flags.c() ? call<false>() : call();
				});
			HANDLE_OP(0xd5, {
				cycles = push(DE.b16);
				});
			HANDLE_OP(0xd6, {
				cycles = (sub<uint8_t, false, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xd7, {
				cycles = rst<0x10>();
				});
			HANDLE_OP(0xd8, {
				cycles = flags.c() ? ret() : ret<false>();
				});
			HANDLE_OP(0xd9, {
				cycles = reti();
				});
			HANDLE_OP(0xda, {
				cycles = flags.c() ? jp() : jp<false>();
				});
			HANDLE_OP(0xdb, {
				RDR_LOG_ERROR("Invalid opcode 0xDB");
				});
			HANDLE_OP(0xdc, {
				cycles = flags.c() ? call() : call<false>();
				});
			HANDLE_OP(0xdd, {
				RDR_LOG_ERROR("Invalid opcode 0xDD");
				});
			HANDLE_OP(0xde, {
				cycles = (sub<uint8_t, true, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xdf, {
				cycles = rst<0x18>();
				});

			HANDLE_OP(0xe0, {
				uint16_t addr = 0xff00 + memory[pc++];
				cycles = (str(addr, AF.hi)) + 4;
				});
			HANDLE_OP(0xe1, {
				cycles = pop(HL.b16);
				});
			HANDLE_OP(0xe2, {
				uint16_t addr = 0xff00 + BC.lo;
				cycles = str(addr, AF.hi);
				});
			HANDLE_OP(0xe3, {
				RDR_LOG_ERROR("Invalid opcode 0xE3");
				});
			HANDLE_OP(0xe4, {
				RDR_LOG_ERROR("Invalid opcode 0xE4");
				});
			HANDLE_OP(0xe5, {
				cycles = push(HL.b16);
				});
			HANDLE_OP(0xe6, {
				cycles = (opand<uint8_t, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xe7, {
				cycles = rst<0x20>();
				});
			HANDLE_OP(0xe8, {
				cycles = addsp();
				});
			HANDLE_OP(0xe9, {
				cycles = jpreg(HL.b16);
				});
			HANDLE_OP(0xea, {
				uint16_t addr = memory.As<uint16_t>(pc);
				cycles = (str(addr, AF.hi)) + 8;
				pc += 2;
				});
			HANDLE_OP(0xeb, {
				RDR_LOG_ERROR("Invalid opcode 0xEB");
				});
			HANDLE_OP(0xec, {
				RDR_LOG_ERROR("Invalid opcode 0xEC");
				});
			HANDLE_OP(0xed, {
				RDR_LOG_ERROR("Invalid opcode 0xED");
				});
			HANDLE_OP(0xee, {
				cycles = (opxor<uint8_t, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xef, {
				cycles = rst<0x28>();
				});

			HANDLE_OP(0xf0, {
				uint16_t addr = 0xff00 + memory[pc++];
				cycles = (ldr(AF.hi, addr)) + 4;
				});
			HANDLE_OP(0xf1, {
				cycles = pop(AF.b16);
				AF.lo &= 0xf0;
				});
			HANDLE_OP(0xf2, {
				uint16_t addr = 0xff00 + BC.lo;
				cycles = ldr(AF.hi, addr);
				});
			HANDLE_OP(0xf3, {
				cycles = di();
				});
			HANDLE_OP(0xf4, {
				RDR_LOG_ERROR("Invalid opcode 0xF4");
				});
			HANDLE_OP(0xf5, {
				cycles = push(AF.b16);
				});
			HANDLE_OP(0xf6, {
				cycles = (opor<uint8_t, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xf7, {
				cycles = rst<0x30>();
				});
			HANDLE_OP(0xf8, {
				cycles = ldsp(HL.b16);
				});
			HANDLE_OP(0xf9, {
				cycles = mv(sp, HL.b16);
				});
			HANDLE_OP(0xfa, {
				uint16_t addr = memory.As<uint16_t>(pc);
				cycles = (ldr(AF.hi, addr)) + 8;
				pc += 2;
				});
			HANDLE_OP(0xfb, {
				cycles = ei();
				});
			HANDLE_OP(0xfc, {
				RDR_LOG_ERROR("Invalid opcode 0xFC");
				});
			HANDLE_OP(0xfd, {
				RDR_LOG_ERROR("Invalid opcode 0xFD");
				});
			HANDLE_OP(0xfe, {
				cycles = (cp<uint8_t, true>(AF.hi, pc++));
				});
			HANDLE_OP(0xff, {
				cycles = rst<0x38>();
				});
		}

		return cycles;
	}

	uint32_t CPU::prefix()
	{
		uint8_t opcode = memory[pc++];
		uint32_t cycles = 0;

		switch (opcode)
		{
			HANDLE_OP(0x00, {
				cycles = rlc(BC.hi);
				});
			HANDLE_OP(0x01, {
				cycles = rlc(BC.lo);
				});
			HANDLE_OP(0x02, {
				cycles = rlc(DE.hi);
				});
			HANDLE_OP(0x03, {
				cycles = rlc(DE.lo);
				});
			HANDLE_OP(0x04, {
				cycles = rlc(HL.hi);
				});
			HANDLE_OP(0x05, {
				cycles = rlc(HL.lo);
				});
			HANDLE_OP(0x06, {
				cycles = (rlc<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x07, {
				cycles = rlc(AF.hi);
				});
			HANDLE_OP(0x08, {
				cycles = rrc(BC.hi);
				});
			HANDLE_OP(0x09, {
				cycles = rrc(BC.lo);
				});
			HANDLE_OP(0x0a, {
				cycles = rrc(DE.hi);
				});
			HANDLE_OP(0x0b, {
				cycles = rrc(DE.lo);
				});
			HANDLE_OP(0x0c, {
				cycles = rrc(HL.hi);
				});
			HANDLE_OP(0x0d, {
				cycles = rrc(HL.lo);
				});
			HANDLE_OP(0x0e, {
				cycles = (rrc<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x0f, {
				cycles = rrc(AF.hi);
				});

			HANDLE_OP(0x10, {
				cycles = rl(BC.hi);
				});
			HANDLE_OP(0x11, {
				cycles = rl(BC.lo);
				});
			HANDLE_OP(0x12, {
				cycles = rl(DE.hi);
				});
			HANDLE_OP(0x13, {
				cycles = rl(DE.lo);
				});
			HANDLE_OP(0x14, {
				cycles = rl(HL.hi);
				});
			HANDLE_OP(0x15, {
				cycles = rl(HL.lo);
				});
			HANDLE_OP(0x16, {
				cycles = (rl<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x17, {
				cycles = rl(AF.hi);
				});
			HANDLE_OP(0x18, {
				cycles = rr(BC.hi);
				});
			HANDLE_OP(0x19, {
				cycles = rr(BC.lo);
				});
			HANDLE_OP(0x1a, {
				cycles = rr(DE.hi);
				});
			HANDLE_OP(0x1b, {
				cycles = rr(DE.lo);
				});
			HANDLE_OP(0x1c, {
				cycles = rr(HL.hi);
				});
			HANDLE_OP(0x1d, {
				cycles = rr(HL.lo);
				});
			HANDLE_OP(0x1e, {
				cycles = (rr<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x1f, {
				cycles = rr(AF.hi);
				});

			HANDLE_OP(0x20, {
				cycles = sla(BC.hi);
				});
			HANDLE_OP(0x21, {
				cycles = sla(BC.lo);
				});
			HANDLE_OP(0x22, {
				cycles = sla(DE.hi);
				});
			HANDLE_OP(0x23, {
				cycles = sla(DE.lo);
				});
			HANDLE_OP(0x24, {
				cycles = sla(HL.hi);
				});
			HANDLE_OP(0x25, {
				cycles = sla(HL.lo);
				});
			HANDLE_OP(0x26, {
				cycles = (sla<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x27, {
				cycles = sla(AF.hi);
				});
			HANDLE_OP(0x28, {
				cycles = sra(BC.hi);
				});
			HANDLE_OP(0x29, {
				cycles = sra(BC.lo);
				});
			HANDLE_OP(0x2a, {
				cycles = sra(DE.hi);
				});
			HANDLE_OP(0x2b, {
				cycles = sra(DE.lo);
				});
			HANDLE_OP(0x2c, {
				cycles = sra(HL.hi);
				});
			HANDLE_OP(0x2d, {
				cycles = sra(HL.lo);
				});
			HANDLE_OP(0x2e, {
				cycles = (sra<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x2f, {
				cycles = sra(AF.hi);
				});

			HANDLE_OP(0x30, {
				cycles = swap(BC.hi);
				});
			HANDLE_OP(0x31, {
				cycles = swap(BC.lo);
				});
			HANDLE_OP(0x32, {
				cycles = swap(DE.hi);
				});
			HANDLE_OP(0x33, {
				cycles = swap(DE.lo);
				});
			HANDLE_OP(0x34, {
				cycles = swap(HL.hi);
				});
			HANDLE_OP(0x35, {
				cycles = swap(HL.lo);
				});
			HANDLE_OP(0x36, {
				cycles = (swap<uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x37, {
				cycles = swap(AF.hi);
				});
			HANDLE_OP(0x38, {
				cycles = srl(BC.hi);
				});
			HANDLE_OP(0x39, {
				cycles = srl(BC.lo);
				});
			HANDLE_OP(0x3a, {
				cycles = srl(DE.hi);
				});
			HANDLE_OP(0x3b, {
				cycles = srl(DE.lo);
				});
			HANDLE_OP(0x3c, {
				cycles = srl(HL.hi);
				});
			HANDLE_OP(0x3d, {
				cycles = srl(HL.lo);
				});
			HANDLE_OP(0x3e, {
				cycles = (srl<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x3f, {
				cycles = srl(AF.hi);
				});

			HANDLE_OP(0x40, {
				cycles = testbit<0>(BC.hi);
				});
			HANDLE_OP(0x41, {
				cycles = testbit<0>(BC.lo);
				});
			HANDLE_OP(0x42, {
				cycles = testbit<0>(DE.hi);
				});
			HANDLE_OP(0x43, {
				cycles = testbit<0>(DE.lo);
				});
			HANDLE_OP(0x44, {
				cycles = testbit<0>(HL.hi);
				});
			HANDLE_OP(0x45, {
				cycles = testbit<0>(HL.lo);
				});
			HANDLE_OP(0x46, {
				cycles = (testbit<0, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x47, {
				cycles = testbit<0>(AF.hi);
				});
			HANDLE_OP(0x48, {
				cycles = testbit<1>(BC.hi);
				});
			HANDLE_OP(0x49, {
				cycles = testbit<1>(BC.lo);
				});
			HANDLE_OP(0x4a, {
				cycles = testbit<1>(DE.hi);
				});
			HANDLE_OP(0x4b, {
				cycles = testbit<1>(DE.lo);
				});
			HANDLE_OP(0x4c, {
				cycles = testbit<1>(HL.hi);
				});
			HANDLE_OP(0x4d, {
				cycles = testbit<1>(HL.lo);
				});
			HANDLE_OP(0x4e, {
				cycles = (testbit<1, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x4f, {
				cycles = testbit<1>(AF.hi);
				});

			HANDLE_OP(0x50, {
				cycles = testbit<2>(BC.hi);
				});
			HANDLE_OP(0x51, {
				cycles = testbit<2>(BC.lo);
				});
			HANDLE_OP(0x52, {
				cycles = testbit<2>(DE.hi);
				});
			HANDLE_OP(0x53, {
				cycles = testbit<2>(DE.lo);
				});
			HANDLE_OP(0x54, {
				cycles = testbit<2>(HL.hi);
				});
			HANDLE_OP(0x55, {
				cycles = testbit<2>(HL.lo);
				});
			HANDLE_OP(0x56, {
				cycles = (testbit<2, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x57, {
				cycles = testbit<2>(AF.hi);
				});
			HANDLE_OP(0x58, {
				cycles = testbit<3>(BC.hi);
				});
			HANDLE_OP(0x59, {
				cycles = testbit<3>(BC.lo);
				});
			HANDLE_OP(0x5a, {
				cycles = testbit<3>(DE.hi);
				});
			HANDLE_OP(0x5b, {
				cycles = testbit<3>(DE.lo);
				});
			HANDLE_OP(0x5c, {
				cycles = testbit<3>(HL.hi);
				});
			HANDLE_OP(0x5d, {
				cycles = testbit<3>(HL.lo);
				});
			HANDLE_OP(0x5e, {
				cycles = (testbit<3, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x5f, {
				cycles = testbit<3>(AF.hi);
				});

			HANDLE_OP(0x60, {
				cycles = testbit<4>(BC.hi);
				});
			HANDLE_OP(0x61, {
				cycles = testbit<4>(BC.lo);
				});
			HANDLE_OP(0x62, {
				cycles = testbit<4>(DE.hi);
				});
			HANDLE_OP(0x63, {
				cycles = testbit<4>(DE.lo);
				});
			HANDLE_OP(0x64, {
				cycles = testbit<4>(HL.hi);
				});
			HANDLE_OP(0x65, {
				cycles = testbit<4>(HL.lo);
				});
			HANDLE_OP(0x66, {
				cycles = (testbit<4, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x67, {
				cycles = testbit<4>(AF.hi);
				});
			HANDLE_OP(0x68, {
				cycles = testbit<5>(BC.hi);
				});
			HANDLE_OP(0x69, {
				cycles = testbit<5>(BC.lo);
				});
			HANDLE_OP(0x6a, {
				cycles = testbit<5>(DE.hi);
				});
			HANDLE_OP(0x6b, {
				cycles = testbit<5>(DE.lo);
				});
			HANDLE_OP(0x6c, {
				cycles = testbit<5>(HL.hi);
				});
			HANDLE_OP(0x6d, {
				cycles = testbit<5>(HL.lo);
				});
			HANDLE_OP(0x6e, {
				cycles = (testbit<5, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x6f, {
				cycles = testbit<5>(AF.hi);
				});

			HANDLE_OP(0x70, {
				cycles = testbit<6>(BC.hi);
				});
			HANDLE_OP(0x71, {
				cycles = testbit<6>(BC.lo);
				});
			HANDLE_OP(0x72, {
				cycles = testbit<6>(DE.hi);
				});
			HANDLE_OP(0x73, {
				cycles = testbit<6>(DE.lo);
				});
			HANDLE_OP(0x74, {
				cycles = testbit<6>(HL.hi);
				});
			HANDLE_OP(0x75, {
				cycles = testbit<6>(HL.lo);
				});
			HANDLE_OP(0x76, {
				cycles = (testbit<6, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x77, {
				cycles = testbit<6>(AF.hi);
				});
			HANDLE_OP(0x78, {
				cycles = testbit<7>(BC.hi);
				});
			HANDLE_OP(0x79, {
				cycles = testbit<7>(BC.lo);
				});
			HANDLE_OP(0x7a, {
				cycles = testbit<7>(DE.hi);
				});
			HANDLE_OP(0x7b, {
				cycles = testbit<7>(DE.lo);
				});
			HANDLE_OP(0x7c, {
				cycles = testbit<7>(HL.hi);
				});
			HANDLE_OP(0x7d, {
				cycles = testbit<7>(HL.lo);
				});
			HANDLE_OP(0x7e, {
				cycles = (testbit<7, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x7f, {
				cycles = testbit<7>(AF.hi);
				});

			HANDLE_OP(0x80, {
				cycles = resetbit<0>(BC.hi);
				});
			HANDLE_OP(0x81, {
				cycles = resetbit<0>(BC.lo);
				});
			HANDLE_OP(0x82, {
				cycles = resetbit<0>(DE.hi);
				});
			HANDLE_OP(0x83, {
				cycles = resetbit<0>(DE.lo);
				});
			HANDLE_OP(0x84, {
				cycles = resetbit<0>(HL.hi);
				});
			HANDLE_OP(0x85, {
				cycles = resetbit<0>(HL.lo);
				});
			HANDLE_OP(0x86, {
				cycles = (resetbit<0, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x87, {
				cycles = resetbit<0>(AF.hi);
				});
			HANDLE_OP(0x88, {
				cycles = resetbit<1>(BC.hi);
				});
			HANDLE_OP(0x89, {
				cycles = resetbit<1>(BC.lo);
				});
			HANDLE_OP(0x8a, {
				cycles = resetbit<1>(DE.hi);
				});
			HANDLE_OP(0x8b, {
				cycles = resetbit<1>(DE.lo);
				});
			HANDLE_OP(0x8c, {
				cycles = resetbit<1>(HL.hi);
				});
			HANDLE_OP(0x8d, {
				cycles = resetbit<1>(HL.lo);
				});
			HANDLE_OP(0x8e, {
				cycles = (resetbit<1, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x8f, {
				cycles = resetbit<1>(AF.hi);
				});

			HANDLE_OP(0x90, {
				cycles = resetbit<2>(BC.hi);
				});
			HANDLE_OP(0x91, {
				cycles = resetbit<2>(BC.lo);
				});
			HANDLE_OP(0x92, {
				cycles = resetbit<2>(DE.hi);
				});
			HANDLE_OP(0x93, {
				cycles = resetbit<2>(DE.lo);
				});
			HANDLE_OP(0x94, {
				cycles = resetbit<2>(HL.hi);
				});
			HANDLE_OP(0x95, {
				cycles = resetbit<2>(HL.lo);
				});
			HANDLE_OP(0x96, {
				cycles = (resetbit<2, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x97, {
				cycles = resetbit<2>(AF.hi);
				});
			HANDLE_OP(0x98, {
				cycles = resetbit<3>(BC.hi);
				});
			HANDLE_OP(0x99, {
				cycles = resetbit<3>(BC.lo);
				});
			HANDLE_OP(0x9a, {
				cycles = resetbit<3>(DE.hi);
				});
			HANDLE_OP(0x9b, {
				cycles = resetbit<3>(DE.lo);
				});
			HANDLE_OP(0x9c, {
				cycles = resetbit<3>(HL.hi);
				});
			HANDLE_OP(0x9d, {
				cycles = resetbit<3>(HL.lo);
				});
			HANDLE_OP(0x9e, {
				cycles = (resetbit<3, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x9f, {
				cycles = resetbit<3>(AF.hi);
				});

			HANDLE_OP(0xa0, {
				cycles = resetbit<4>(BC.hi);
				});
			HANDLE_OP(0xa1, {
				cycles = resetbit<4>(BC.lo);
				});
			HANDLE_OP(0xa2, {
				cycles = resetbit<4>(DE.hi);
				});
			HANDLE_OP(0xa3, {
				cycles = resetbit<4>(DE.lo);
				});
			HANDLE_OP(0xa4, {
				cycles = resetbit<4>(HL.hi);
				});
			HANDLE_OP(0xa5, {
				cycles = resetbit<4>(HL.lo);
				});
			HANDLE_OP(0xa6, {
				cycles = (resetbit<4, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xa7, {
				cycles = resetbit<4>(AF.hi);
				});
			HANDLE_OP(0xa8, {
				cycles = resetbit<5>(BC.hi);
				});
			HANDLE_OP(0xa9, {
				cycles = resetbit<5>(BC.lo);
				});
			HANDLE_OP(0xaa, {
				cycles = resetbit<5>(DE.hi);
				});
			HANDLE_OP(0xab, {
				cycles = resetbit<5>(DE.lo);
				});
			HANDLE_OP(0xac, {
				cycles = resetbit<5>(HL.hi);
				});
			HANDLE_OP(0xad, {
				cycles = resetbit<5>(HL.lo);
				});
			HANDLE_OP(0xae, {
				cycles = (resetbit<5, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xaf, {
				cycles = resetbit<5>(AF.hi);
				});

			HANDLE_OP(0xb0, {
				cycles = resetbit<6>(BC.hi);
				});
			HANDLE_OP(0xb1, {
				cycles = resetbit<6>(BC.lo);
				});
			HANDLE_OP(0xb2, {
				cycles = resetbit<6>(DE.hi);
				});
			HANDLE_OP(0xb3, {
				cycles = resetbit<6>(DE.lo);
				});
			HANDLE_OP(0xb4, {
				cycles = resetbit<6>(HL.hi);
				});
			HANDLE_OP(0xb5, {
				cycles = resetbit<6>(HL.lo);
				});
			HANDLE_OP(0xb6, {
				cycles = (resetbit<6, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xb7, {
				cycles = resetbit<6>(AF.hi);
				});
			HANDLE_OP(0xb8, {
				cycles = resetbit<7>(BC.hi);
				});
			HANDLE_OP(0xb9, {
				cycles = resetbit<7>(BC.lo);
				});
			HANDLE_OP(0xba, {
				cycles = resetbit<7>(DE.hi);
				});
			HANDLE_OP(0xbb, {
				cycles = resetbit<7>(DE.lo);
				});
			HANDLE_OP(0xbc, {
				cycles = resetbit<7>(HL.hi);
				});
			HANDLE_OP(0xbd, {
				cycles = resetbit<7>(HL.lo);
				});
			HANDLE_OP(0xbe, {
				cycles = (resetbit<7, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xbf, {
				cycles = resetbit<7>(AF.hi);
				});

			HANDLE_OP(0xc0, {
				cycles = setbit<0>(BC.hi);
				});
			HANDLE_OP(0xc1, {
				cycles = setbit<0>(BC.lo);
				});
			HANDLE_OP(0xc2, {
				cycles = setbit<0>(DE.hi);
				});
			HANDLE_OP(0xc3, {
				cycles = setbit<0>(DE.lo);
				});
			HANDLE_OP(0xc4, {
				cycles = setbit<0>(HL.hi);
				});
			HANDLE_OP(0xc5, {
				cycles = setbit<0>(HL.lo);
				});
			HANDLE_OP(0xc6, {
				cycles = (setbit<0, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xc7, {
				cycles = setbit<0>(AF.hi);
				});
			HANDLE_OP(0xc8, {
				cycles = setbit<1>(BC.hi);
				});
			HANDLE_OP(0xc9, {
				cycles = setbit<1>(BC.lo);
				});
			HANDLE_OP(0xca, {
				cycles = setbit<1>(DE.hi);
				});
			HANDLE_OP(0xcb, {
				cycles = setbit<1>(DE.lo);
				});
			HANDLE_OP(0xcc, {
				cycles = setbit<1>(HL.hi);
				});
			HANDLE_OP(0xcd, {
				cycles = setbit<1>(HL.lo);
				});
			HANDLE_OP(0xce, {
				cycles = (setbit<1, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xcf, {
				cycles = setbit<1>(AF.hi);
				});

			HANDLE_OP(0xd0, {
				cycles = setbit<2>(BC.hi);
				});
			HANDLE_OP(0xd1, {
				cycles = setbit<2>(BC.lo);
				});
			HANDLE_OP(0xd2, {
				cycles = setbit<2>(DE.hi);
				});
			HANDLE_OP(0xd3, {
				cycles = setbit<2>(DE.lo);
				});
			HANDLE_OP(0xd4, {
				cycles = setbit<2>(HL.hi);
				});
			HANDLE_OP(0xd5, {
				cycles = setbit<2>(HL.lo);
				});
			HANDLE_OP(0xd6, {
				cycles = (setbit<2, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xd7, {
				cycles = setbit<2>(AF.hi);
				});
			HANDLE_OP(0xd8, {
				cycles = setbit<3>(BC.hi);
				});
			HANDLE_OP(0xd9, {
				cycles = setbit<3>(BC.lo);
				});
			HANDLE_OP(0xda, {
				cycles = setbit<3>(DE.hi);
				});
			HANDLE_OP(0xdb, {
				cycles = setbit<3>(DE.lo);
				});
			HANDLE_OP(0xdc, {
				cycles = setbit<3>(HL.hi);
				});
			HANDLE_OP(0xdd, {
				cycles = setbit<3>(HL.lo);
				});
			HANDLE_OP(0xde, {
				cycles = (setbit<3, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xdf, {
				cycles = setbit<3>(AF.hi);
				});

			HANDLE_OP(0xe0, {
				cycles = setbit<4>(BC.hi);
				});
			HANDLE_OP(0xe1, {
				cycles = setbit<4>(BC.lo);
				});
			HANDLE_OP(0xe2, {
				cycles = setbit<4>(DE.hi);
				});
			HANDLE_OP(0xe3, {
				cycles = setbit<4>(DE.lo);
				});
			HANDLE_OP(0xe4, {
				cycles = setbit<4>(HL.hi);
				});
			HANDLE_OP(0xe5, {
				cycles = setbit<4>(HL.lo);
				});
			HANDLE_OP(0xe6, {
				cycles = (setbit<4, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xe7, {
				cycles = setbit<4>(AF.hi);
				});
			HANDLE_OP(0xe8, {
				cycles = setbit<5>(BC.hi);
				});
			HANDLE_OP(0xe9, {
				cycles = setbit<5>(BC.lo);
				});
			HANDLE_OP(0xea, {
				cycles = setbit<5>(DE.hi);
				});
			HANDLE_OP(0xeb, {
				cycles = setbit<5>(DE.lo);
				});
			HANDLE_OP(0xec, {
				cycles = setbit<5>(HL.hi);
				});
			HANDLE_OP(0xed, {
				cycles = setbit<5>(HL.lo);
				});
			HANDLE_OP(0xee, {
				cycles = (setbit<5, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xef, {
				cycles = setbit<5>(AF.hi);
				});

			HANDLE_OP(0xf0, {
				cycles = setbit<6>(BC.hi);
				});
			HANDLE_OP(0xf1, {
				cycles = setbit<6>(BC.lo);
				});
			HANDLE_OP(0xf2, {
				cycles = setbit<6>(DE.hi);
				});
			HANDLE_OP(0xf3, {
				cycles = setbit<6>(DE.lo);
				});
			HANDLE_OP(0xf4, {
				cycles = setbit<6>(HL.hi);
				});
			HANDLE_OP(0xf5, {
				cycles = setbit<6>(HL.lo);
				});
			HANDLE_OP(0xf6, {
				cycles = (setbit<6, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xf7, {
				cycles = setbit<6>(AF.hi);
				});
			HANDLE_OP(0xf8, {
				cycles = setbit<7>(BC.hi);
				});
			HANDLE_OP(0xf9, {
				cycles = setbit<7>(BC.lo);
				});
			HANDLE_OP(0xfa, {
				cycles = setbit<7>(DE.hi);
				});
			HANDLE_OP(0xfb, {
				cycles = setbit<7>(DE.lo);
				});
			HANDLE_OP(0xfc, {
				cycles = setbit<7>(HL.hi);
				});
			HANDLE_OP(0xfd, {
				cycles = setbit<7>(HL.lo);
				});
			HANDLE_OP(0xfe, {
				cycles = (setbit<7, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xff, {
				cycles = setbit<7>(AF.hi);
				});
		}

		return cycles;
	}
}