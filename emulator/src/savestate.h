#pragma once

namespace emu
{
	struct EmulationState
	{
		std::filesystem::path rom;
		uint8_t memory[0x10000] = {};
		uint16_t af = 0, bc = 0, de = 0, hl = 0, sp = 0, pc = 0;
		uint32_t romBankOffset = 0, ramOffset = 0;
		bool ramEnabled = false, romSelect = false;
		bool valid = false;
	};
}