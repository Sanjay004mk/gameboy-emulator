#pragma once

#include "rom.h"
#include <miniaudio.h>

namespace emu
{
	class SoundChannel
	{
	public:
		SoundChannel() = default;
		virtual ~SoundChannel() = default;
	protected:
	};

	class SoundChannel1 : public SoundChannel
	{

	};

	class SPU
	{
	public:
		SPU(Memory& memory);
		~SPU();

		void step(uint32_t cycles);

	private:
		Memory& memory;
		ma_device audioDevice = {};
		SoundChannel* sc[4] = {};
	};
}