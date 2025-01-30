#pragma once

#include "rom.h"
#include "ppu.h"

namespace emu
{
	union Register
	{
		uint16_t b16;
		struct { uint8_t lo, hi;  };
	};

	struct Flags
	{
		uint8_t& data;
		bool ime = false;
		bool halt = false;
		Flags(uint8_t& data)
			: data(data)
		{

		}

		void setz(bool val)
		{
			data &= ~(0x80);
			data |= (val << 7);
		}

		bool z() const
		{
			return data & 0x80;
		}
		void setn(bool val)
		{
			data &= ~(0x40);
			data |= (val << 6);
		}

		bool n() const
		{
			return data & 0x40;
		}

		void seth(bool val)
		{
			data &= ~(0x20);
			data |= (val << 5);
		}

		bool h() const
		{
			return data & 0x20;
		}

		void setc(bool val)
		{
			data &= ~(0x10);
			data |= (val << 4);
		}

		bool c() const
		{
			return data & 0x10;
		}
	};

	class CPU
	{
	public:
		static inline constexpr uint64_t frequency = 4194304;

		CPU();
		~CPU();

		void Update();

		void LoadRom(const char* file);
		rdr::Texture* GetDisplayTexture() { return ppu.GetDisplayTexture(); }

		void Pause();
		void Resume();
		void Toggle();
		bool isRunning() { return running; }
		void Reset(bool withBootRom = true);
		void LoadAndStart(const char* file, bool withBootRom = true);

		void InputPressed(Input ip);
		void InputReleased(Input ip);
		bool GetInputState(Input ip);

		std::string SerialOut();

		void SaveRAM() { memory.SaveRAM(); }
		void SetColorPalette(const ColorPalette& palette) { ppu.SetColorPalette(palette); }

	private:
		uint32_t SingleStepUpdate();
		uint32_t step();
		uint32_t prefix();

		void handleInterrupts();
		void updateTimer(uint32_t cycles);

		template <typename Word>
		uint32_t mv(Word& target, Word& source)
		{
			target = source;
			return 4 * sizeof(Word);
		}

		template <typename Word>
		uint32_t ld(Word& reg)
		{
			reg = memory.As<Word>(pc);
			pc += sizeof(Word);

			if constexpr (sizeof(Word) == 2)
				return 12;
			else if constexpr (sizeof(Word) == 1)
				return 8;
		}

		template <typename Word, bool inc = false, bool dec = false>
		uint32_t stc(Word& reg)
		{
			memory(reg, memory[pc++]);

			if constexpr (inc)
				reg++;
			else if constexpr (dec)
				reg--;

			return 12;
		}

		template <typename Word, bool inc = false, bool dec = false>
		uint32_t str(uint16_t& addr, Word& reg)
		{
			memory.SetAs<Word>(addr, reg);

			if constexpr (inc)
				addr++;
			else if constexpr (dec)
				addr--;

			return 8;
		}

		template <typename Word, bool inc = false, bool dec = false>
		uint32_t ldr(Word& reg, uint16_t& addr)
		{
			static_assert(sizeof(Word) == 1);
			reg = memory[addr];

			if constexpr (inc)
				addr++;
			else if constexpr (dec)
				addr--;

			return 8;
		}

		template <typename Word, bool mem = false>
		uint32_t inc(Word& reg)
		{ 
			if constexpr (sizeof(Word) == 1)
			{
				static_assert(!mem);
				Word prev = reg++;
				flags.setz((reg == 0));
				flags.setn(0);
				flags.seth(((prev & 0x10) ^ (reg & 0x10)));
				
				return 4;
			}
			else
			{
				static_assert(sizeof(Word) == 2);
				
				if constexpr (mem)
				{
					auto prev = memory[reg];
					memory(reg, memory[reg] + 1);
					flags.setz((memory[reg] == 0));
					flags.setn(0);
					flags.seth(((prev & 0x10) ^ (memory[reg] & 0x10)));
					return 12;
				}
				else
				{
					reg++;
					return 8;
				}
			}			
		}

		template <typename Word, bool mem = false>
		uint32_t dec(Word& reg)
		{
			if constexpr (sizeof(Word) == 1)
			{
				static_assert(!mem);
				Word prev = reg--;
				flags.setz((reg == 0));
				flags.setn(1);

				flags.seth(((prev & 0x0f) < 1));
				
				return 4;
			}
			else
			{
				static_assert(sizeof(Word) == 2);
				if constexpr (mem)
				{
					Word prev = memory[reg];
					memory(reg, memory[reg] - 1);
					flags.setz((memory[reg] == 0));
					flags.setn(1);

					flags.seth(((prev & 0x0f) < 1));
					return 12;
				}
				else
				{
					reg--;
					return 8;
				}

			}
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t rlc(Word& reg)
		{
			if constexpr (mem)
			{
				flags.setc(memory[reg] & 0x80);
				memory(reg, (memory[reg] << 1) | (flags.c() ? 0x01 : 0x00));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				flags.setc(reg & 0x80);
				reg = reg << 1;
				reg |= flags.c() ? 0x01 : 0x00;
				flags.setz(reg == 0);
			}

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);

			if constexpr (acc)
				return 4;
			else if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t rl(Word& reg)
		{
			if constexpr (mem)
			{
				uint8_t c = flags.c();
				flags.setc(memory[reg] & 0x80);
				memory(reg, (memory[reg] << 1) | c);
				flags.setz(memory[reg] == 0);
			}
			else
			{
				uint8_t c = flags.c();
				flags.setc(reg & 0x80);
				reg = reg << 1;
				reg |= c;
				flags.setz(reg == 0);
			}

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);

			if constexpr (acc)
				return 4;
			else if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t rrc(Word& reg)
		{
			if constexpr (mem)
			{
				flags.setc(memory[reg] & 0x01);
				memory(reg, (memory[reg] >> 1) | (flags.c() ? 0x80 : 0x00));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				flags.setc(reg & 0x01);
				reg = reg >> 1;
				reg |= flags.c() ? 0x80 : 0x00;
				flags.setz(reg == 0);
			}

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);

			if constexpr (acc)
				return 4;
			else if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t rr(Word& reg)
		{
			if constexpr (mem)
			{
				uint8_t c = flags.c();
				flags.setc(memory[reg] & 0x01);
				memory(reg, (memory[reg] >> 1) | (c << 7));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				uint8_t c = flags.c();
				flags.setc(reg & 0x01);
				reg = reg >> 1;
				reg |= (c << 7);
				flags.setz(reg == 0);
			}			

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);

			if constexpr (acc)
				return 4;
			else if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t sla(Word& reg)
		{
			if constexpr (mem)
			{
				flags.setc(memory[reg] & 0x80);
				memory(reg, (memory[reg] << 1));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				flags.setc(reg & 0x80);
				reg = reg << 1;
				flags.setz(reg == 0);
			}
			flags.setn(false);
			flags.seth(false);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t sra(Word& reg)
		{
			if constexpr (mem)
			{
				flags.setc(memory[reg] & 0x01);
				memory(reg, (memory[reg] >> 1) | (memory[reg] & 0x80));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				flags.setc(reg & 0x01);
				reg = (reg >> 1) | (reg & 0x80);
				flags.setz(reg == 0);
			}
			flags.setn(false);
			flags.seth(false);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t srl(Word& reg)
		{
			if constexpr (mem)
			{
				flags.setc(memory[reg] & 0x01);
				memory(reg, (memory[reg] >> 1));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				flags.setc(reg & 0x01);
				reg = (reg >> 1);
				flags.setz(reg == 0);
			}
			flags.setn(false);
			flags.seth(false);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool mem = false>
		uint32_t swap(Word& reg)
		{
			if constexpr (mem)
			{
				memory(reg, ((memory[reg] & 0xf0) >> 4) | ((memory[reg] & 0x0f) << 4));
				flags.setz(memory[reg] == 0);
			}
			else
			{
				reg = ((reg & 0xf0) >> 4) | ((reg & 0x0f) << 4);
				flags.setz(reg == 0);
			}
			
			flags.setn(false);
			flags.seth(false);
			flags.setc(false);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <uint32_t bit, typename Word, bool mem = false>
		uint32_t testbit(Word& reg)
		{
			if constexpr (mem)
				flags.setz(!(memory[reg] & (0x1 << bit)));
			else
				flags.setz(!(reg & (0x1 << bit)));
			flags.setn(false);
			flags.seth(true);

			if constexpr (mem)
				return 12;
			else
				return 8;
		}

		template <uint32_t bit, typename Word, bool mem = false>
		uint32_t setbit(Word& reg)
		{
			if constexpr (mem)
				memory(reg, memory[reg] | (1 << bit));
			else
				reg |= (1 << bit);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <uint32_t bit, typename Word, bool mem = false>
		uint32_t resetbit(Word& reg)
		{
			if constexpr (mem)
				memory(reg, memory[reg] & (~(1 << bit)));
			else
				reg &= (~(1 << bit));

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool carry = false, bool mem = false, typename WordOrMem = Word>
		uint32_t add(Word& targetReg, WordOrMem otherReg)
		{
			static constexpr uint32_t carry_bit = sizeof(Word) == 1 ? 0x00000100 : 0x00010000;
			static constexpr uint32_t hflag = sizeof(Word) == 1 ? 0xf : 0x0fff;
			static constexpr uint32_t hbit = sizeof(Word) == 1 ? 0x10 : 0x1000;

			int32_t target = 0, other = 0;
			target = targetReg;
			if constexpr (mem)
				other = (int32_t)memory.As<Word>(otherReg);
			else
				other = (int32_t)otherReg;

			int32_t res = target + other;
			
			flags.setn(false);
			
			if constexpr (carry)
				if (flags.c())
					res += 1;
				
			if constexpr (carry)
				flags.seth((((target & hflag) + (other & hflag) + flags.c()) & hbit));
			else
				flags.seth((((target & hflag) + (other & hflag)) & hbit));

			flags.setc(res & carry_bit);
			targetReg = (Word)res;

			if constexpr (sizeof(Word) == 2)
				return 8;
			else
			{
				flags.setz(targetReg == 0);

				if constexpr (mem)
					return 8;
				else
					return 4;
			}
		}

		template <typename Word, bool carry = false, bool mem = false, typename WordOrMem = Word>
		uint32_t sub(Word& targetReg, WordOrMem otherReg)
		{
			static_assert(sizeof(Word) == 1);

			int32_t target = targetReg, other = 0;
			if constexpr (mem)
				other = (int32_t)memory[otherReg];
			else
				other = (int32_t)otherReg;

			int32_t res = target - other;
			
			if constexpr (carry)
				if (flags.c())
					res -= 1;

			flags.setn(true);

			if constexpr (carry)
				flags.seth((target & 0xf) < (((other) & 0xf) + flags.c()));
			else
				flags.seth((target & 0xf) < (other & 0xf));

			flags.setc(res < 0);
			
			targetReg = (Word)res;
			flags.setz(targetReg == 0);

			if constexpr (mem)
				return 8;
			else
				return 4;
		}

		template <typename Word, bool mem = false, typename WordOrMem = Word>
		uint32_t opand(Word& target, WordOrMem other)
		{
			if constexpr (mem)
				target = target & memory[other];
			else
				target = target & other;

			flags.setz(target == 0);
			flags.seth(true);
			flags.setn(false);
			flags.setc(false);

			if constexpr (mem)
				return 8;
			else
				return 4;
		}

		template <typename Word, bool mem = false, typename WordOrMem = Word>
		uint32_t opor(Word& target, WordOrMem other)
		{
			if constexpr (mem)
				target = target | memory[other];
			else
				target = target | other;
			flags.setz(target == 0);
			flags.seth(false);
			flags.setn(false);
			flags.setc(false);

			if constexpr (mem)
				return 8;
			else
				return 4;
		}

		template <typename Word, bool mem = false, typename WordOrMem = Word>
		uint32_t opxor(Word& target, WordOrMem other)
		{
			if constexpr (mem)
				target = target ^ memory[other];
			else
				target = target ^ other;

			flags.setz(target == 0);
			flags.seth(false);
			flags.setn(false);
			flags.setc(false);

			if constexpr (mem)
				return 8;
			else
				return 4;
		}

		template <typename Word, bool mem = false, typename WordOrMem = Word>
		uint32_t cp(Word& targetReg, WordOrMem otherReg)
		{
			static_assert(sizeof(Word) == 1);
			int32_t target = targetReg, reg = 0;
			if constexpr (mem)
				reg = memory[otherReg];
			else
				reg = otherReg;
			int32_t res = (int32_t)target - (int32_t)reg;

			flags.setn(true);
			flags.setz(res == 0);
			flags.setc(res < 0);
			flags.seth((target & 0xf) < (reg & 0xf));

			if constexpr (mem)
				return 8;
			else
				return 4;
		}

		template <bool condition = true>
		uint32_t jr()
		{
			if constexpr (condition)
			{
				pc += (memory.As<int8_t>(pc) + 1);
				return 12;
			}
			else
			{
				pc++;
				return 8;
			}
		}

		template <bool condition = true>
		uint32_t jp()
		{
			if constexpr (condition)
			{
				pc = memory.As<uint16_t>(pc);
				return 16;
			}
			else
			{
				pc += 2;
				return 12;
			}
		}

		template <bool condition = true>
		uint32_t jpreg(uint16_t& reg)
		{
			static_assert(condition);
			pc = reg;

			return 4;
		}

		template <bool condition = true>
		uint32_t call()
		{
			if constexpr (condition)
			{
				sp -= 2;
				memory.SetAs<uint16_t>(sp, pc + 2);
				pc = memory.As<uint16_t>(pc);

				return 24;
			}
			else
			{
				pc += 2;
				return 12;
			}
		}

		template <bool condition = true, bool check = true>
		uint32_t ret()
		{
			if constexpr (condition)
			{
				pc = memory.As<uint16_t>(sp);
				sp += 2;

				if constexpr (check)
					return 20;
				else
					return 16;
			}
			else
			{
				return 8;
			}
		}

		uint32_t di()
		{
			flags.ime = false;
			return 4;
		}

		uint32_t ei()
		{
			flags.ime = true;
			return 4;
		}

		uint32_t reti()
		{
			ei();
			ret<true, false>();
			return 16;
		}

		template <typename Word>
		uint32_t pop(Word& reg)
		{
			static_assert(sizeof(Word) == 2);
			reg = memory.As<Word>(sp);
			sp += 2;

			return 12;
		}

		template <typename Word>
		uint32_t push(Word& reg)
		{
			static_assert(sizeof(Word) == 2);
			sp -= 2;
			memory.SetAs<Word>(sp, reg);

			return 16;
		}

		uint32_t addsp()
		{
			int8_t offs = memory.As<int8_t>(pc++);

			flags.setz(false);
			flags.setn(false);

			flags.seth(((sp & 0xf) + (memory[pc - 1] & 0xf)) > 0xf);
			flags.setc(((sp & 0xff) + (memory[pc - 1])) > 0xff);

			sp += offs;
			return 16;
		}

		template <typename Word>
		uint32_t ldsp(Word& reg)
		{
			int8_t offs = memory.As<int8_t>(pc++);
			flags.setz(false);
			flags.setn(false);

			flags.seth(((sp & 0xf) + (memory[pc - 1] & 0xf)) > 0xf);
			flags.setc(((sp & 0xff) + (memory[pc - 1])) > 0xff);

			reg = (sp + offs);

			return 12;
		}

		uint32_t daa()
		{

			if (!flags.n())
			{
				if (flags.c() || ((AF.hi) > (0x99)))
				{
					AF.hi += 0x60;
					flags.setc(true);
				}
				if (flags.h() || ((AF.hi & 0x0f) > 0x09))
					AF.hi += 0x06;
			}
			else
			{
				if (flags.c())
				{
					AF.hi -= 0x60;
				}
				if (flags.h())
					AF.hi -= 0x06;
			}

			flags.setz((AF.hi == 0));
			flags.seth(false);

			return 4;
		}

		uint32_t cpl()
		{
			flags.seth(true);
			flags.setn(true);
			AF.hi = ~AF.hi;

			return 4;
		}

		uint32_t scf()
		{
			flags.setc(true);
			flags.seth(false);
			flags.setn(false);

			return 4;
		}

		uint32_t ccf()
		{
			flags.setc(!flags.c());
			flags.seth(false);
			flags.setn(false);

			return 4;
		}

		template <uint16_t addr>
		uint32_t rst()
		{
			sp -= 2;
			memory.SetAs<uint16_t>(sp, pc);
			pc = addr;
			return 16;
		}

		Memory memory;
		Register AF = { 0x01b0 };
		Register BC = { 0x0013 };
		Register DE = { 0x00d8 };
		Register HL = { 0x014d };
		uint16_t sp = 0xfffe;
		uint16_t pc = 0x0000;
		Flags flags;

		PPU ppu;

		bool running = false;

		uint32_t divCycle = 0, timerCycles = 0;
		std::string serialData;
		bool serialPresent = false;

		uint32_t inputState = 0;

		friend class Debugger;
	};
}