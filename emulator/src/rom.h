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

		struct RTCRegister
		{
			union
			{
				uint8_t s, m, h, dl, dh;
				uint8_t e[5];
			} data = {};
			uint32_t selected = 0;
			bool enabled = false;
			bool latched = false;
			bool latchingStarted = false;
		};

		ExternalRam cartrigeRam;
		RTCRegister rtc;

		enum class BankingType { MBC0, MBC1, MBC2, MBC3, MBC5 };

		std::function<bool(Input)> getInputState;
		BankingType bankType = BankingType::MBC0;
		size_t romBankOffset = 0x4000;
		uint16_t mbc5RomBankNumber = 0;

		uint8_t memory[std::numeric_limits<uint16_t>::max() + 1] = {};
		uint8_t* rom = nullptr;
		std::filesystem::path romFileName, saveFileName;

		bool romSelect = true;

		void Init(const char* file);
		void Reset();

		// write
		template <typename Integer>
		uint8_t operator()(Integer i, uint8_t value)
		{
			if (i == 0xff50 && value)
			{
				LockBootRom();
				return memory[i] = value;
			}

			// OAM transfer
			if (i == 0xff46)
			{
				uint32_t start = value * 0x100;
				memcpy_s(memory + 0xfe00, 160, memory + start, 160);

				return memory[i] = value;
			}

			// Set Joypad Register
			if (i == 0xff00)
				return memory[i] = SetInputState(value);

			HandleWriteMBC((uint32_t)i, value);

			return memory[i];
		}

		// read
		template <typename Integer>
		const uint8_t& operator[](Integer i) const
		{
			return HandleReadMBC((uint32_t)i);
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
		const T As(Integer i) const
		{
			return *(reinterpret_cast<const T*>(&(*this)[i]));
		}

		void LoadBootRom();
		void LockBootRom();
		uint8_t SetInputState(uint8_t value);
		void HandleWriteMBC(uint32_t address, uint8_t value);
		const uint8_t& HandleReadMBC(uint32_t address) const;

		void SaveRAM();
	};

}