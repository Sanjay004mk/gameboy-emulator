#include "emupch.h"

#include "cpu.h"

namespace emu
{
	void CPU::LoadState(const EmulationState& state, bool strict)
	{
		if (state.rom != memory.romFileName)
		{
			RDR_LOG_ERROR("Loading save state of a different rom. May lead to unexpected behaviour!");
			if (strict)
				return;
		}

		memcpy_s(memory.memory, sizeof(memory.memory), state.memory, sizeof(state.memory));

		AF.b16 = state.af;
		BC.b16 = state.bc;
		DE.b16 = state.bc;
		HL.b16 = state.hl;
		sp = state.sp;
		pc = state.pc;

		memory.romBankOffset = state.romBankOffset;
		memory.romSelect = state.romSelect;
		memory.cartrigeRam.enabled = state.ramEnabled;
		memory.cartrigeRam.offset = state.ramOffset;
	}

	void CPU::SaveState(EmulationState& state)
	{
		memcpy_s(state.memory, sizeof(state.memory), memory.memory, sizeof(memory.memory));

		state.af = AF.b16;
		state.bc = BC.b16;
		state.bc = DE.b16;
		state.hl = HL.b16;
		state.sp = sp;
		state.pc = pc;

		state.rom = memory.romFileName;

		state.romBankOffset = memory.romBankOffset;
		state.romSelect = memory.romSelect;
		state.ramEnabled = memory.cartrigeRam.enabled;
		state.ramOffset = memory.cartrigeRam.offset;

		state.valid = true;
	}
}