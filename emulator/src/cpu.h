#pragma once

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
		int16_t enableImeCountdown = -1;
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

	struct Memory
	{
		uint8_t memory[std::numeric_limits<uint16_t>::max()] = {};

		static inline uint8_t temp = 0x90;

		template <typename Integer>
		uint8_t& operator[](Integer i)
		{
			if (i == 0xff44)
				return temp;

			return memory[i];
		}


		// write
		template <typename Integer>
		uint8_t& operator()(Integer i, uint8_t value) 
		{
			return (*this)[i] = value;
		}

		// read
		template <typename Integer>
		const uint8_t& operator[](Integer i) const
		{
			if (i == 0xff44)
				return temp;

			return memory[i];
		}

		// write
		template <typename T, typename Integer>
		T& SetAs(Integer idx, T value)
		{
			for (size_t i = 0; i < sizeof(T); i++)
				(*this)(idx + i, ((uint8_t*)(&value))[i]);

			return *(reinterpret_cast<T*>(&(*this)[idx]));
		}

		template <typename T, typename Integer>
		T& As(Integer i)
		{
			return *(reinterpret_cast<T*>(&(*this)[i]));
		}

		// read
		template <typename T, typename Integer>
		const T& As(Integer i) const
		{
			return *(reinterpret_cast<T*>(&(*this)[i]));
		}
	};

	class CPU
	{
	public:
		static inline constexpr uint64_t frequency = 4194304;

		CPU();
		~CPU();

		bool Update();

	private:
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
				Word prev = reg++;
				flags.setz((reg == 0));
				flags.setn(0);
				flags.seth(((prev & 0x10) ^ (reg & 0x10)));
				
				if constexpr (mem)
					return 12;
				else
					return 4;
			}
			else
			{
				static_assert(sizeof(Word) == 2);
				reg++;
				
				return 8;
			}			
		}

		template <typename Word, bool mem = false>
		uint32_t dec(Word& reg)
		{
			if constexpr (sizeof(Word) == 1)
			{
				Word prev = reg--;
				flags.setz((reg == 0));
				flags.setn(1);

				flags.seth(((prev & 0x0f) < 1));
				
				if constexpr (mem)
					return 12;
				else
					return 4;
			}
			else
			{
				static_assert(sizeof(Word) == 2);
				reg--;

				return 8;
			}
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t rlc(Word& reg)
		{
			static_assert(sizeof(Word) == 1);
			flags.setc(reg & 0x80);
			reg = reg << 1;
			reg |= flags.c() ? 0x01 : 0x00;

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);
			else
				flags.setz(reg == 0);

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
			static_assert(sizeof(Word) == 1);
			uint8_t c = flags.c();
			flags.setc(reg & 0x80);
			reg = reg << 1;
			reg |= c;

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);
			else
				flags.setz(reg == 0);

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
			static_assert(sizeof(Word) == 1);
			flags.setc(reg & 0x01);
			reg = reg >> 1;
			reg |= flags.c() ? 0x80 : 0x00;

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);
			else
				flags.setz(reg == 0);

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
			static_assert(sizeof(Word) == 1);
			uint8_t c = flags.c();
			flags.setc(reg & 0x01);
			reg = reg >> 1;
			reg |= (c << 7);

			flags.setn(false);
			flags.seth(false);

			if constexpr (acc)
				flags.setz(false);
			else
				flags.setz(reg == 0);

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
			flags.setc(reg & 0x80);
			reg = reg << 1;
			flags.setn(false);
			flags.seth(false);
			flags.setz(reg == 0);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t sra(Word& reg)
		{
			flags.setc(reg & 0x01);
			reg = (reg >> 1) | (reg & 0x80);
			flags.setn(false);
			flags.seth(false);
			flags.setz(reg == 0);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool acc = false, bool mem = false>
		uint32_t srl(Word& reg)
		{
			flags.setc(reg & 0x01);
			reg = (reg >> 1);
			flags.setn(false);
			flags.seth(false);
			flags.setz(reg == 0);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool mem = false>
		uint32_t swap(Word& reg)
		{
			reg = ((reg & 0xf0) >> 4) | ((reg & 0x0f) << 4);
			flags.setz(reg == 0);
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
			flags.setn(false);
			flags.seth(true);
			flags.setz(!(reg & (0x1 << bit)));

			if constexpr (mem)
				return 12;
			else
				return 8;
		}

		template <uint32_t bit, typename Word, bool mem = false>
		uint32_t setbit(Word& reg)
		{
			reg |= (1 << bit);

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <uint32_t bit, typename Word, bool mem = false>
		uint32_t resetbit(Word& reg)
		{
			reg &= (~(1 << bit));

			if constexpr (mem)
				return 16;
			else
				return 8;
		}

		template <typename Word, bool carry = false, bool mem = false>
		uint32_t add(Word& target, Word& other)
		{
			static constexpr uint32_t carry_bit = sizeof(Word) == 1 ? 0x00000100 : 0x00010000;
			static constexpr uint32_t hflag = sizeof(Word) == 1 ? 0xf : 0x0fff;
			static constexpr uint32_t hbit = sizeof(Word) == 1 ? 0x10 : 0x1000;

			int32_t res = (int32_t)target + (int32_t)other;
			
			flags.setn(false);
			
			if constexpr (carry)
				if (flags.c())
					res += 1;
				
			if constexpr (carry)
			{
				flags.seth((((target & hflag) + (other & hflag) + flags.c()) & hbit));
			}
			else
				flags.seth((((target & hflag) + (other & hflag)) & hbit));

			flags.setc(res & carry_bit);
			target = (Word)res;

			if constexpr (sizeof(Word) == 2)
				return 8;
			else
			{
				flags.setz(target == 0);

				if constexpr (mem)
					return 8;
				else
					return 4;
			}
		}

		template <typename Word, bool carry = false, bool mem = false>
		uint32_t sub(Word& target, Word& other)
		{
			static_assert(sizeof(Word) == 1);

			int32_t res = (int32_t)target - (int32_t)other;
			
			if constexpr (carry)
				if (flags.c())
					res -= 1;

			flags.setn(true);

			if constexpr (carry)
				flags.seth((target & 0xf) < (((other) & 0xf) + flags.c()));
			else
				flags.seth((target & 0xf) < (other & 0xf));

			flags.setc(res < 0);
			
			target = (Word)res;
			flags.setz(target == 0);

			if constexpr (mem)
				return 8;
			else
				return 4;
		}

		template <typename Word, bool mem = false>
		uint32_t opand(Word& target, Word& other)
		{
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

		template <typename Word, bool mem = false>
		uint32_t opor(Word& target, Word& other)
		{
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

		template <typename Word, bool mem = false>
		uint32_t opxor(Word& target, Word& other)
		{
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

		template <typename Word, bool mem = false>
		uint32_t cp(Word& target, Word& reg)
		{
			static_assert(sizeof(Word) == 1);
			
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
			flags.enableImeCountdown = -1;

			return 4;
		}

		uint32_t ei()
		{
			flags.enableImeCountdown = 2;
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

		Memory memory = {};
		Register AF = { 0x01b0 };
		Register BC = { 0x0013 };
		Register DE = { 0x00d8 };
		Register HL = { 0x014d };
		uint16_t sp = 0xfffe;
		uint16_t pc = 0x0000;
		Flags flags;
	};
}