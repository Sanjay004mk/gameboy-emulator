#include "emupch.h"

#include "rom.h"

namespace emu
{
	ROM::ROM(const char* file)
	{
		RDR_ASSERT_MSG_BREAK(std::filesystem::exists(file), "Rom file doesn't exist: {}", file);
			
		std::ifstream romfile(file, std::ios::binary | std::ios::in);

		data.resize(std::filesystem::file_size(file));

		romfile.read((char*)data.data(), data.size());

		Init();
	}

	ROM::ROM(const void* data, uint64_t size)
	{
		this->data = std::vector<uint8_t>((const uint8_t*)data, (const uint8_t*)data + size);

		Init();
	}

	void ROM::Init()
	{
		// memory banking type
		uint32_t type = data[0x147];
		if ((type & 3) && !(type & ~3))
			bankType = BankingType::MBC1;
		else if (type == 5 || type == 6)
			bankType = BankingType::MBC2;
		else if (type >= 0xf && type <= 0x13)
			bankType = BankingType::MBC3;
		else if (type >= 0x19 && type <= 0x1e)
			bankType = BankingType::MBC5;

		// external ram
		type = data[0x149];
		static uint32_t nbanks[] = { 1, 1, 4, 16, 8 };
		static uint32_t nbanksize[] = { 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
		if (type && type <= 5)
		{
			ram.numRams = nbanks[type - 1];
			ram.size = type == 1 ? 0x800 : 0x2000;
			ram.ram = std::vector<uint8_t>(nbanksize[type - 1]);
		}

	}

	ROM::~ROM()
	{

	}
}