#include "emupch.h"

#include "rom.h"

namespace emu
{
	static uint8_t gameboyBootROM[256] = {
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

	Memory::~Memory()
	{
		SaveRAM();

		if (rom)
			delete[] rom;
	}

	void Memory::Init(const char* file)
	{
		if (rom)
			delete[] rom;

		romFileName = file;
		RDR_ASSERT_MSG_BREAK(std::filesystem::exists(file), "Rom file doesn't exist: {}", file);

		std::ifstream romfile(file, std::ios::binary | std::ios::in);

		size_t size = std::filesystem::file_size(file);
		RDR_ASSERT_MSG_BREAK(size >= 0x8000, "Rom size too small. Check if the rom is correct");

		rom = new uint8_t[size];

		romfile.read((char*)rom, size);

		// memory banking type
		uint32_t type = rom[0x147];
		if ((type & 3) && !(type & ~3))
			bankType = BankingType::MBC1;
		else if (type == 5 || type == 6)
			bankType = BankingType::MBC2;
		else if (type >= 0xf && type <= 0x13)
			bankType = BankingType::MBC3;
		else if (type >= 0x19 && type <= 0x1e)
			bankType = BankingType::MBC5;

		// external ram
		type = rom[0x149];
		static uint32_t nbanks[] = { 1, 1, 1, 4, 16, 8 };
		static uint32_t nbanksize[] = { 0x200, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
		if (type && type <= 5 || (type == 0 && bankType == BankingType::MBC2))
		{
			cartrigeRam.numRams = nbanks[type];
			cartrigeRam.size = type == 1 ? 0x800 : type == 0 ? 0x200 : 0x2000;
			cartrigeRam.ram = std::vector<uint8_t>(nbanksize[type]);

			saveFileName = romFileName;
			saveFileName.replace_extension(".sav");
			LoadRAM();
		}

		memset(memory, 0, sizeof(memory));
		memcpy_s(memory, 0x8000ull, rom, 0x8000ull);
	}

	void Memory::Reset()
	{
		memset(memory, 0, sizeof(memory));

		if (rom)
			memcpy_s(memory, 0x8000ull, rom, 0x8000ull);

		memory[0xff00] = 0xff;

		memory[0xFF05] = 0x00;
		memory[0xFF06] = 0x00;
		memory[0xFF07] = 0x00;
		memory[0xFF10] = 0x80;
		memory[0xFF11] = 0xBF;
		memory[0xFF12] = 0xF3;
		memory[0xFF14] = 0xBF;
		memory[0xFF16] = 0x3F;
		memory[0xFF17] = 0x00;
		memory[0xFF19] = 0xBF;
		memory[0xFF1A] = 0x7F;
		memory[0xFF1B] = 0xFF;
		memory[0xFF1C] = 0x9F;
		memory[0xFF1E] = 0xBF;
		memory[0xFF20] = 0xFF;
		memory[0xFF21] = 0x00;
		memory[0xFF22] = 0x00;
		memory[0xFF23] = 0xBF;
		memory[0xFF24] = 0x77;
		memory[0xFF25] = 0xF3;
		memory[0xFF26] = 0xF1;
		memory[0xFF40] = 0x91;
		memory[0xFF42] = 0x00;
		memory[0xFF43] = 0x00;
		memory[0xFF44] = 0x90;
		memory[0xFF45] = 0x00;
		memory[0xFF47] = 0xFC;
		memory[0xFF48] = 0xFF;
		memory[0xFF49] = 0xFF;
		memory[0xFF4A] = 0x00;
		memory[0xFF4B] = 0x00;
		memory[0xFFFF] = 0x00;

		memory[0xff30] = 0x84;
		memory[0xff31] = 0x40;
		memory[0xff32] = 0x43;
		memory[0xff33] = 0xaa;
		memory[0xff34] = 0x2d;
		memory[0xff35] = 0x78;
		memory[0xff36] = 0x92;
		memory[0xff37] = 0x3c;
		memory[0xff38] = 0x60;
		memory[0xff39] = 0x59;
		memory[0xff3a] = 0x59;
		memory[0xff3b] = 0xb0;
		memory[0xff3c] = 0x34;
		memory[0xff3d] = 0xb8;
		memory[0xff3e] = 0x2e;
		memory[0xff3f] = 0xda;
	}

	void Memory::SaveRAM()
	{
		if (saveFileName.empty() || cartrigeRam.ram.size() == 0)
			return;

		std::ofstream saveFile(saveFileName, std::ios::binary);

		saveFile.write((char*)cartrigeRam.ram.data(), cartrigeRam.ram.size());
	}

	void Memory::LoadRAM(const char* file)
	{
		if (file)
			saveFileName = file;

		if (std::filesystem::exists(saveFileName))
		{
			std::ifstream saveFile(saveFileName, std::ios::binary);
			size_t saveFileSize = std::filesystem::file_size(saveFileName);
			RDR_ASSERT_MSG(saveFileSize == cartrigeRam.ram.size(), "Save file is incorrect or corrupted");

			saveFile.read((char*)cartrigeRam.ram.data(), saveFileSize);
		}
	}

	void Memory::LoadBootRom()
	{
		memcpy_s(memory, 256, gameboyBootROM, 256);
	}

	void Memory::LockBootRom()
	{
		memcpy_s(memory, 256, rom, 256);
	}

	uint8_t Memory::SetInputState(uint8_t value)
	{
		uint8_t joypad = 0x0f;

		bool dirEnable = (value & (1 << 4));
		bool buttonEnable = (value & (1 << 5));

		uint32_t index = buttonEnable ? 4 : 0;

		if (buttonEnable || dirEnable)
		{
			for (uint32_t i = 0; i < 4; i++)
			{
				if (getInputState((Input)(index + i)))
					joypad &= ~(1 << i);
			}
		}

		value &= 0xf0;

		return (value | joypad);
	}

	void Memory::HandleWriteMBC(uint32_t address, uint8_t value)
	{
		if (address >= 0x8000)
		{
			switch (bankType)
			{
			case BankingType::MBC0:
				memory[address] = value;
				break;

			case BankingType::MBC1:
			case BankingType::MBC2:
			case BankingType::MBC3:
			case BankingType::MBC5:
				memory[address] = value;
				if (address >= 0xa000 && address < 0xc000 && cartrigeRam.enabled)
				{
					if (bankType == BankingType::MBC3 && rtc.enabled)
					{
						if (!rtc.latched)
							rtc.data.e[rtc.selected] = value;
					}
					else if (((size_t)address - 0xa000) < cartrigeRam.size)
					{
						if (bankType == BankingType::MBC2)
							value &= 0x0f;
						cartrigeRam.ram[address - 0xa000] = value;
					}
				}
				break;
			}
		}

		if (bankType == BankingType::MBC0)
			return;

		if (address < 0x2000)
		{
			switch (bankType)
			{
			case BankingType::MBC2:
				if ((0x0100 & address))
					break;
				[[fallthrough]];

			case BankingType::MBC1:
			case BankingType::MBC3:
			case BankingType::MBC5:
				cartrigeRam.enabled = value;
				break;
			}
		}
		else if (address < 0x4000)
		{
			switch (bankType)
			{
			case BankingType::MBC2:
				if (!(0x0100 & address))
					break;

				value &= 0x0f;
				[[fallthrough]];

			case BankingType::MBC1:
				if (value == 0x20 || value == 0x60 || value == 0x40)
					value += 1;
				[[fallthrough]];

			case BankingType::MBC3:
				if (value == 0x0)
					value += 1;
				
				romBankOffset = 0x4000ull * value;
				break;

			case BankingType::MBC5:
				uint16_t mask = address < 0x3000 ? 0xff00 : 0x00ff;
				mbc5RomBankNumber &= mask;
				mbc5RomBankNumber |= mask == 0xff00 ? value : value << 8;
				romBankOffset = 0x4000ull * mbc5RomBankNumber;
				break;
			}
		}
		else if (address < 0x6000)
		{
			switch (bankType)
			{
			case BankingType::MBC1:
				if (romSelect)
					romBankOffset = 0x4000ull * ((value & 0x60ull));
				else
					cartrigeRam.offset = 0x2000ull * (value & 3);
				break;

			case BankingType::MBC3:
				if (value >= 0x8)
				{
					rtc.enabled = true;
					rtc.selected = (value & 0x0f) - 8;
					break;
				}
				else
					rtc.enabled = false;
				[[fallthrough]];

			case BankingType::MBC5:
				cartrigeRam.offset = 0x2000ull * (value & 0x0f);
				break;
			}
		}
		else if (address < 0x8000)
		{
			switch (bankType)
			{
			case emu::Memory::BankingType::MBC1:
				romSelect = !value;
				break;
			case emu::Memory::BankingType::MBC3:
				if (rtc.latchingStarted && value == 0x1)
				{
					rtc.latchingStarted = false;
					rtc.latched = !rtc.latched;
				}
				else if (value == 0x0)
					rtc.latchingStarted = true;
				break;
			}
		}
	}

	const uint8_t& Memory::HandleReadMBC(uint32_t address) const
	{
		if (bankType == BankingType::MBC0)
			return memory[address];

		if (address >= 0x4000 && address < 0x8000)
			return rom[romBankOffset + (address - 0x4000)];
		else if (address >= 0xa000 && address < 0xc000 && cartrigeRam.enabled)
		{
			if (bankType == BankingType::MBC3 && rtc.enabled)
				return rtc.data.e[rtc.selected];
			else if ((size_t)(address - 0xa000) < cartrigeRam.size)
				return cartrigeRam.ram[address - 0xa000];
		}

		return memory[address];
	}
}