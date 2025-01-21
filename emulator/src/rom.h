#pragma once

namespace emu
{
	struct ROM
	{
		struct ExternalRam
		{
			std::vector<uint8_t> ram;
			uint32_t size = 0;
			size_t offset = 0;
			uint32_t numRams = 0;
			bool enabled = false;
		};

		enum class BankingType { MBC0, MBC1, MBC2, MBC3, MBC5 };

		std::vector<uint8_t> data;

		BankingType bankType = BankingType::MBC0;
		size_t romBankOffset = 0x4000;
		ExternalRam ram;

		ROM(const char* file);
		ROM(const void* data, uint64_t size);
		~ROM();

		void Init();

	};
}