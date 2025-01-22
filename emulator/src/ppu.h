#pragma once

#include <renderer/rdr.h>

namespace emu
{
	class PPU
	{
	public:
		PPU(const Memory& memory);
		~PPU();

		void step(uint32_t cycles);

		rdr::Texture* GetDisplayTexture() { return displayTexture; }

	private:
		void SetTextureData();

		const Memory& memory;

		rdr::Texture* displayTexture = nullptr;
		std::vector<uint32_t> cpuBuffer;
		rdr::Buffer* stagingBuffer = nullptr;

		uint32_t cycAcc = 0;
	};
}