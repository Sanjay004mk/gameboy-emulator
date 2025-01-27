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

	struct Memory
	{
		uint8_t memory[std::numeric_limits<uint16_t>::max() + 1] = {};
		std::shared_ptr<ROM> rom;
		bool romSelect = true;

		void Init(const char* file);
		void Reset();

		// write
		template <typename Integer>
		uint8_t operator()(Integer i, uint8_t value)
		{
			if (i == 0xff50 && value)
				LockBootRom();

			// OAM transfer
			if (i == 0xff46)
			{
				memory[i] = value;
				uint32_t start = value * 0x100;
				memcpy_s(memory + 0xfe00, 160, memory + value, 160);
			}

			if (rom->bankType == ROM::BankingType::MBC0)
			{
				if (i >= 0x8000)
					memory[i] = value;
			}
			else if (rom->bankType == ROM::BankingType::MBC1)
			{
				if (i < 0x2000)
					rom->ram.enabled = value;
				else if (i < 0x4000)
				{
					if (value == 0x20 || value == 0x60 || value == 0x40 || value == 0x0)
						value += 1;

					rom->romBankOffset = 0x4000ull * value;
				}
				else if (i < 0x6000)
				{
					if (romSelect)
						rom->romBankOffset = 0x4000ull * ((value & 0x3ull) << 5);
					else
						rom->ram.offset = 0x2000ull * (value & 3);
				}
				else if (i < 0x8000)
					romSelect = !value;
				else if (i >= 0xa000 && i < 0xc000 && rom->ram.enabled)
				{
					if ((size_t)(i - 0xa000) < rom->ram.size)
						memory[i] = rom->ram.ram[i - 0xa000] = value;
				}
				else
					memory[i] = value;
			}
			else if (rom->bankType == ROM::BankingType::MBC2)
			{
				// TODO : implement mbc 2, 3 and 5
				RDR_ASSERT_NO_MSG_BREAK(false);
			}

			return memory[i];
		}

		// read
		template <typename Integer>
		const uint8_t& operator[](Integer i) const
		{
			if (rom->bankType == ROM::BankingType::MBC1)
			{
				if (i >= 0x4000 && i < 0x8000)
					return rom->data[rom->romBankOffset + (i - 0x4000)];

				if (i >= 0xa000 && i < 0xc000 && rom->ram.enabled)
					if ((size_t)(i - 0xa000) < rom->ram.size)
						return rom->ram.ram[i - 0xa000];
			}
			else if (rom->bankType == ROM::BankingType::MBC2)
			{

			}
			else if (rom->bankType == ROM::BankingType::MBC3)
			{

			}

			return memory[i];
		}

		// write
		template <typename T, typename Integer>
		T SetAs(Integer idx, T value)
		{
			for (size_t i = 0; i < sizeof(T); i++)
				(*this)(idx + i, ((uint8_t*)(&value))[i]);

			return *(reinterpret_cast<T*>(&(memory[idx])));
		}

		// read
		template <typename T, typename Integer>
		const T& As(Integer i) const
		{
			return *(reinterpret_cast<const T*>(&(*this)[i]));
		}

		void LoadBootRom();
		void LockBootRom();
	};

}