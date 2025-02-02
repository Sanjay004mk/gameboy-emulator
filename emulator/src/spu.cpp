#include "emupch.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "spu.h"

namespace emu
{
	SPU::SPU(Memory& memory)
		: memory(memory)
	{

	}

	SPU::~SPU()
	{

	}

	void SPU::step(uint32_t cycles)
	{

	}
}