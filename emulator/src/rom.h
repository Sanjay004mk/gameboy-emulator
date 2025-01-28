#pragma once

namespace emu
{
	enum Input
	{
		Input_Button_A = 0,
		Input_Button_B,
		Input_Button_Select,
		Input_Button_Start,
		Input_Button_Right,
		Input_Button_Left,
		Input_Button_Up,
		Input_Button_Down
	};

	struct Memory
	{
		Memory(std::function<bool(Input)> getInputState) : getInputState(getInputState) {}
		~Memory();

		struct ExternalRam
		{
			std::vector<uint8_t> ram;
			uint32_t size = 0;
			size_t offset = 0;
			uint32_t numRams = 0;
			bool enabled = false;
		};

		enum class BankingType { MBC0, MBC1, MBC2, MBC3, MBC5 };

		std::function<bool(Input)> getInputState;
		BankingType bankType = BankingType::MBC0;
		size_t romBankOffset = 0x4000;
		ExternalRam cartrigeRam;

		uint8_t memory[std::numeric_limits<uint16_t>::max() + 1] = {};
		uint8_t* rom = nullptr;

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
				memcpy_s(memory + 0xfe00, 160, memory + start, 160);
			}

			// Set Joypad Register
			if (i == 0xff00)
			{
				memory[i] = value;
				SetInputState();
			}

			if (bankType == BankingType::MBC0)
			{
				if (i >= 0x8000)
					memory[i] = value;
			}
			else if (bankType == BankingType::MBC1)
			{
				if (i < 0x2000)
					cartrigeRam.enabled = value;
				else if (i < 0x4000)
				{
					if (value == 0x20 || value == 0x60 || value == 0x40 || value == 0x0)
						value += 1;

					romBankOffset = 0x4000ull * value;
				}
				else if (i < 0x6000)
				{
					if (romSelect)
						romBankOffset = 0x4000ull * ((value & 0x3ull) << 5);
					else
						cartrigeRam.offset = 0x2000ull * (value & 3);
				}
				else if (i < 0x8000)
					romSelect = !value;
				else if (i >= 0xa000 && i < 0xc000 && cartrigeRam.enabled)
				{
					if ((size_t)(i - 0xa000) < cartrigeRam.size)
						memory[i] = cartrigeRam.ram[i - 0xa000] = value;
				}
				else
					memory[i] = value;
			}
			else if (bankType == BankingType::MBC2)
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
			if (bankType == BankingType::MBC1)
			{
				if (i >= 0x4000 && i < 0x8000)
					return rom[romBankOffset + (i - 0x4000)];

				if (i >= 0xa000 && i < 0xc000 && cartrigeRam.enabled)
					if ((size_t)(i - 0xa000) < cartrigeRam.size)
						return cartrigeRam.ram[i - 0xa000];
			}
			else if (bankType == BankingType::MBC2)
			{

			}
			else if (bankType == BankingType::MBC3)
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
		void SetInputState();
	};

}