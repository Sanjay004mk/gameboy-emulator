#pragma once

#include "rom.h"

namespace emu
{
	class SPU
	{
	public:
		SPU(Memory& memory);
		~SPU();

		void step(uint32_t cycles);

	private:
		Memory& memory;
	};
}