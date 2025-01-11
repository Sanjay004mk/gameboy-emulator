#include "emupch.h"
#include "cpu.h"

namespace emu
{
	uint32_t CPU::step()
	{
		uint8_t opcode = memory[pc++];
		uint32_t cycles = 0;

#define HANDLE_OP(opcode, fn) \
case opcode:\
{\
fn \
}\
break;

		switch (opcode)
		{
			HANDLE_OP(0xcb, {
				cycles = prefix();
				});

			HANDLE_OP(0x00, {
				cycles = 4;
				});
			HANDLE_OP(0x01, {
				cycles = ld(BC.b16);
				});
			HANDLE_OP(0x02, {
				cycles = str(BC.b16, AF.hi);
				});
			HANDLE_OP(0x03, {
				cycles = inc(BC.b16);
				});
			HANDLE_OP(0x04, {
				cycles = inc(BC.hi);
				});
			HANDLE_OP(0x05, {
				cycles = dec(BC.hi);
				});
			HANDLE_OP(0x06, {
				cycles = ld(BC.hi);
				});
			HANDLE_OP(0x07, {
				cycles = (rlc<uint8_t, true>(AF.hi));
				});
			HANDLE_OP(0x08, {
				memory.SetAs<uint16_t>(memory.As<uint16_t>(pc), sp);
				pc += 2;
				cycles = 20;
				});
			HANDLE_OP(0x09, {
				cycles = add(HL.b16, BC.b16);
				});
			HANDLE_OP(0x0a, {
				cycles = ldr(AF.hi, BC.b16);
				});
			HANDLE_OP(0x0b, {
				cycles = dec(BC.b16);
				});
			HANDLE_OP(0x0c, {
				cycles = inc(BC.lo);
				});
			HANDLE_OP(0x0d, {
				cycles = dec(BC.lo);
				});
			HANDLE_OP(0x0e, {
				cycles = ld(BC.lo);
				});
			HANDLE_OP(0x0f, {
				cycles = (rrc<uint8_t, true>(AF.hi));
				});

			HANDLE_OP(0x10, {
				// TODO stop emulation (maybe?) istr: STOP
				cycles = 4;
				});
			HANDLE_OP(0x11, {
				cycles = ld(DE.b16);
				});
			HANDLE_OP(0x12, {
				cycles = str(DE.b16, AF.hi);
				});
			HANDLE_OP(0x13, {
				cycles = inc(DE.b16);
				});
			HANDLE_OP(0x14, {
				cycles = inc(DE.hi);
				});
			HANDLE_OP(0x15, {
				cycles = dec(DE.hi);
				});
			HANDLE_OP(0x16, {
				cycles = ld(DE.hi);
				});
			HANDLE_OP(0x17, {
				cycles = (rl<uint8_t, true>(AF.hi));
				});
			HANDLE_OP(0x18, {
				cycles = jr();
				});
			HANDLE_OP(0x19, {
				cycles = add(HL.b16, DE.b16);
				});
			HANDLE_OP(0x1a, {
				cycles = ldr(AF.hi, DE.b16);
				});
			HANDLE_OP(0x1b, {
				cycles = dec(DE.b16);
				});
			HANDLE_OP(0x1c, {
				cycles = inc(DE.lo);
				});
			HANDLE_OP(0x1d, {
				cycles = dec(DE.lo);
				});
			HANDLE_OP(0x1e, {
				cycles = ld(DE.lo);
				});
			HANDLE_OP(0x1f, {
				cycles = (rr<uint8_t, true>(AF.hi));
				});

			HANDLE_OP(0x20, {
				cycles = flags.z() ? jr<false>() : jr();
				});
			HANDLE_OP(0x21, {
				cycles = ld(HL.b16);
				});
			HANDLE_OP(0x22, {
				cycles = (str<uint8_t, true, false>(HL.b16, AF.hi));
				});
			HANDLE_OP(0x23, {
				cycles = inc(HL.b16);
				});
			HANDLE_OP(0x24, {
				cycles = inc(HL.hi);
				});
			HANDLE_OP(0x025, {
				cycles = dec(HL.hi);
				});
			HANDLE_OP(0x26, {
				cycles = ld(HL.hi);
				});
			HANDLE_OP(0x27, {
				cycles = daa();
				});
			HANDLE_OP(0x28, {
				cycles = flags.z() ? jr() : jr<false>();
				});
			HANDLE_OP(0x29, {
				cycles = add(HL.b16, HL.b16);
				});
			HANDLE_OP(0x2a, {
				cycles = (ldr<uint8_t, true, false>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x2b, {
				cycles = dec(HL.b16);
				});
			HANDLE_OP(0x2c, {
				cycles = inc(HL.lo);
				});
			HANDLE_OP(0x2d, {
				cycles = dec(HL.lo);
				});
			HANDLE_OP(0x2e, {
				cycles = ld(HL.lo);
				});
			HANDLE_OP(0x2f, {
				cycles = cpl();
				});

			HANDLE_OP(0x30, {
				cycles = flags.c() ? jr<false>() : jr();
				});
			HANDLE_OP(0x31, {
				cycles = ld(sp);
				});
			HANDLE_OP(0x32, {
				cycles = (str<uint8_t, false, true>(HL.b16, AF.hi));
				});
			HANDLE_OP(0x33, {
				cycles = inc(sp);
				});
			HANDLE_OP(0x34, {
				cycles = (inc<uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x35, {
				cycles = (dec<uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x36, {
				cycles = stc(HL.b16);
				});
			HANDLE_OP(0x37, {
				cycles = scf();
				});
			HANDLE_OP(0x38, {
				cycles = flags.c() ? jr() : jr<false>();
				});
			HANDLE_OP(0x39, {
				cycles = add(HL.b16, sp);
				});
			HANDLE_OP(0x3a, {
				cycles = (ldr<uint8_t, false, true>(AF.hi, HL.b16));
				});
			HANDLE_OP(0x3b, {
				cycles = dec(sp);
				});
			HANDLE_OP(0x3c, {
				cycles = inc(AF.hi);
				});
			HANDLE_OP(0x3d, {
				cycles = dec(AF.hi);
				});
			HANDLE_OP(0x3e, {
				cycles = ld(AF.hi);
				});
			HANDLE_OP(0x3f, {
				cycles = ccf();
				});

			HANDLE_OP(0x40, {
				cycles = mv(BC.hi, BC.hi);
				});
			HANDLE_OP(0x41, {
				cycles = mv(BC.hi, BC.lo);
				});
			HANDLE_OP(0x42, {
				cycles = mv(BC.hi, DE.hi);
				});
			HANDLE_OP(0x43, {
				cycles = mv(BC.hi, DE.lo);
				});
			HANDLE_OP(0x44, {
				cycles = mv(BC.hi, HL.hi);
				});
			HANDLE_OP(0x45, {
				cycles = mv(BC.hi, HL.lo);
				});
			HANDLE_OP(0x46, {
				cycles = ldr(BC.hi, HL.b16);
				});
			HANDLE_OP(0x47, {
				cycles = mv(BC.hi, AF.hi);
				});
			HANDLE_OP(0x48, {
				cycles = mv(BC.lo, BC.hi);
				});
			HANDLE_OP(0x49, {
				cycles = mv(BC.lo, BC.lo);
				});
			HANDLE_OP(0x4a, {
				cycles = mv(BC.lo, DE.hi);
				});
			HANDLE_OP(0x4b, {
				cycles = mv(BC.lo, DE.lo);
				});
			HANDLE_OP(0x4c, {
				cycles = mv(BC.lo, HL.hi);
				});
			HANDLE_OP(0x4d, {
				cycles = mv(BC.lo, HL.lo);
				});
			HANDLE_OP(0x4e, {
				cycles = ldr(BC.lo, HL.b16);
				});
			HANDLE_OP(0x4f, {
				cycles = mv(BC.lo, AF.hi);
				});

			HANDLE_OP(0x50, {
				cycles = mv(DE.hi, BC.hi);
				});
			HANDLE_OP(0x51, {
				cycles = mv(DE.hi, BC.lo);
				});
			HANDLE_OP(0x52, {
				cycles = mv(DE.hi, DE.hi);
				});
			HANDLE_OP(0x53, {
				cycles = mv(DE.hi, DE.lo);
				});
			HANDLE_OP(0x54, {
				cycles = mv(DE.hi, HL.hi);
				});
			HANDLE_OP(0x55, {
				cycles = mv(DE.hi, HL.lo);
				});
			HANDLE_OP(0x56, {
				cycles = ldr(DE.hi, HL.b16);
				});
			HANDLE_OP(0x57, {
				cycles = mv(DE.hi, AF.hi);
				});
			HANDLE_OP(0x58, {
				cycles = mv(DE.lo, BC.hi);
				});
			HANDLE_OP(0x59, {
				cycles = mv(DE.lo, BC.lo);
				});
			HANDLE_OP(0x5a, {
				cycles = mv(DE.lo, DE.hi);
				});
			HANDLE_OP(0x5b, {
				cycles = mv(DE.lo, DE.lo);
				});
			HANDLE_OP(0x5c, {
				cycles = mv(DE.lo, HL.hi);
				});
			HANDLE_OP(0x5d, {
				cycles = mv(DE.lo, HL.lo);
				});
			HANDLE_OP(0x5e, {
				cycles = ldr(DE.lo, HL.b16);
				});
			HANDLE_OP(0x5f, {
				cycles = mv(DE.lo, AF.hi);
				});

			HANDLE_OP(0x60, {
				cycles = mv(HL.hi, BC.hi);
				});
			HANDLE_OP(0x61, {
				cycles = mv(HL.hi, BC.lo);
				});
			HANDLE_OP(0x62, {
				cycles = mv(HL.hi, DE.hi);
				});
			HANDLE_OP(0x63, {
				cycles = mv(HL.hi, DE.lo);
				});
			HANDLE_OP(0x64, {
				cycles = mv(HL.hi, HL.hi);
				});
			HANDLE_OP(0x65, {
				cycles = mv(HL.hi, HL.lo);
				});
			HANDLE_OP(0x66, {
				cycles = ldr(HL.hi, HL.b16);
				});
			HANDLE_OP(0x67, {
				cycles = mv(HL.hi, AF.hi);
				});
			HANDLE_OP(0x68, {
				cycles = mv(HL.lo, BC.hi);
				});
			HANDLE_OP(0x69, {
				cycles = mv(HL.lo, BC.lo);
				});
			HANDLE_OP(0x6a, {
				cycles = mv(HL.lo, DE.hi);
				});
			HANDLE_OP(0x6b, {
				cycles = mv(HL.lo, DE.lo);
				});
			HANDLE_OP(0x6c, {
				cycles = mv(HL.lo, HL.hi);
				});
			HANDLE_OP(0x6d, {
				cycles = mv(HL.lo, HL.lo);
				});
			HANDLE_OP(0x6e, {
				cycles = ldr(HL.lo, HL.b16);
				});
			HANDLE_OP(0x6f, {
				cycles = mv(HL.lo, AF.hi);
				});

			HANDLE_OP(0x70, {
				cycles = str(HL.b16, BC.hi);
				});
			HANDLE_OP(0x71, {
				cycles = str(HL.b16, BC.lo);
				});
			HANDLE_OP(0x72, {
				cycles = str(HL.b16, DE.hi);
				});
			HANDLE_OP(0x73, {
				cycles = str(HL.b16, DE.lo);
				});
			HANDLE_OP(0x74, {
				cycles = str(HL.b16, HL.hi);
				});
			HANDLE_OP(0x75, {
				cycles = str(HL.b16, HL.lo);
				});
			HANDLE_OP(0x76, {
				flags.halt = true;
				cycles = 4;
				});
			HANDLE_OP(0x77, {
				cycles = str(HL.b16, AF.hi);
				});
			HANDLE_OP(0x78, {
				cycles = mv(AF.hi, BC.hi);
				});
			HANDLE_OP(0x79, {
				cycles = mv(AF.hi, BC.lo);
				});
			HANDLE_OP(0x7a, {
				cycles = mv(AF.hi, DE.hi);
				});
			HANDLE_OP(0x7b, {
				cycles = mv(AF.hi, DE.lo);
				});
			HANDLE_OP(0x7c, {
				cycles = mv(AF.hi, HL.hi);
				});
			HANDLE_OP(0x7d, {
				cycles = mv(AF.hi, HL.lo);
				});
			HANDLE_OP(0x7e, {
				cycles = ldr(AF.hi, HL.b16);
				});
			HANDLE_OP(0x7f, {
				cycles = mv(AF.hi, AF.hi);
				});

			HANDLE_OP(0x80, {
				cycles = add(AF.hi, BC.hi);
				});
			HANDLE_OP(0x81, {
				cycles = add(AF.hi, BC.lo);
				});
			HANDLE_OP(0x82, {
				cycles = add(AF.hi, DE.hi);
				});
			HANDLE_OP(0x83, {
				cycles = add(AF.hi, DE.lo);
				})
				HANDLE_OP(0x84, {
					cycles = add(AF.hi, HL.hi);
					});
				HANDLE_OP(0x85, {
					cycles = add(AF.hi, HL.lo);
					});
				HANDLE_OP(0x86, {
					cycles = (add<uint8_t, false, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0x87, {
					cycles = add(AF.hi, AF.hi);
					});
				HANDLE_OP(0x88, {
					cycles = (add<uint8_t, true>(AF.hi, BC.hi));
					});
				HANDLE_OP(0x89, {
					cycles = (add<uint8_t, true>(AF.hi, BC.lo));
					});
				HANDLE_OP(0x8a, {
					cycles = (add<uint8_t, true>(AF.hi, DE.hi));
					});
				HANDLE_OP(0x8b, {
					cycles = (add<uint8_t, true>(AF.hi, DE.lo));
					});
				HANDLE_OP(0x8c, {
					cycles = (add<uint8_t, true>(AF.hi, HL.hi));
					});
				HANDLE_OP(0x8d, {
					cycles = (add<uint8_t, true>(AF.hi, HL.lo));
					});
				HANDLE_OP(0x8e, {
					cycles = (add<uint8_t, true, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0x8f, {
					cycles = (add<uint8_t, true>(AF.hi, AF.hi));
					});

				HANDLE_OP(0x90, {
					cycles = sub(AF.hi, BC.hi);
					});
				HANDLE_OP(0x91, {
					cycles = sub(AF.hi, BC.lo);
					});
				HANDLE_OP(0x92, {
					cycles = sub(AF.hi, DE.hi);
					});
				HANDLE_OP(0x93, {
					cycles = sub(AF.hi, DE.lo);
					});
				HANDLE_OP(0x94, {
					cycles = sub(AF.hi, HL.hi);
					});
				HANDLE_OP(0x95, {
					cycles = sub(AF.hi, HL.lo);
					});
				HANDLE_OP(0x96, {
					cycles = (sub<uint8_t, false, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0x97, {
					cycles = sub(AF.hi, AF.hi);
					});
				HANDLE_OP(0x98, {
					cycles = (sub<uint8_t, true>(AF.hi, BC.hi));
					});
				HANDLE_OP(0x99, {
					cycles = (sub<uint8_t, true>(AF.hi, BC.lo));
					});
				HANDLE_OP(0x9a, {
					cycles = (sub<uint8_t, true>(AF.hi, DE.hi));
					});
				HANDLE_OP(0x9b, {
					cycles = (sub<uint8_t, true>(AF.hi, DE.lo));
					});
				HANDLE_OP(0x9c, {
					cycles = (sub<uint8_t, true>(AF.hi, HL.hi));
					});
				HANDLE_OP(0x9d, {
					cycles = (sub<uint8_t, true>(AF.hi, HL.lo));
					});
				HANDLE_OP(0x9e, {
					cycles = (sub<uint8_t, true, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0x9f, {
					cycles = (sub<uint8_t, true>(AF.hi, AF.hi));
					});

				HANDLE_OP(0xa0, {
					cycles = opand(AF.hi, BC.hi);
					});
				HANDLE_OP(0xa1, {
					cycles = opand(AF.hi, BC.lo);
					});
				HANDLE_OP(0xa2, {
					cycles = opand(AF.hi, DE.hi);
					});
				HANDLE_OP(0xa3, {
					cycles = opand(AF.hi, DE.lo);
					});
				HANDLE_OP(0xa4, {
					cycles = opand(AF.hi, HL.hi);
					});
				HANDLE_OP(0xa5, {
					cycles = opand(AF.hi, HL.lo);
					});
				HANDLE_OP(0xa6, {
					cycles = (opand<uint8_t, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0xa7, {
					cycles = opand(AF.hi, AF.hi);
					});
				HANDLE_OP(0xa8, {
					cycles = opxor(AF.hi, BC.hi);
					});
				HANDLE_OP(0xa9, {
					cycles = opxor(AF.hi, BC.lo);
					});
				HANDLE_OP(0xaa, {
					cycles = opxor(AF.hi, DE.hi);
					});
				HANDLE_OP(0xab, {
					cycles = opxor(AF.hi, DE.lo);
					});
				HANDLE_OP(0xac, {
					cycles = opxor(AF.hi, HL.hi);
					});
				HANDLE_OP(0xad, {
					cycles = opxor(AF.hi, HL.lo);
					});
				HANDLE_OP(0xae, {
					cycles = (opxor<uint8_t, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0xaf, {
					cycles = opxor(AF.hi, AF.hi);
					});

				HANDLE_OP(0xb0, {
					cycles = opor(AF.hi, BC.hi);
					});
				HANDLE_OP(0xb1, {
					cycles = opor(AF.hi, BC.lo);
					});
				HANDLE_OP(0xb2, {
					cycles = opor(AF.hi, DE.hi);
					});
				HANDLE_OP(0xb3, {
					cycles = opor(AF.hi, DE.lo);
					});
				HANDLE_OP(0xb4, {
					cycles = opor(AF.hi, HL.hi);
					});
				HANDLE_OP(0xb5, {
					cycles = opor(AF.hi, HL.lo);
					});
				HANDLE_OP(0xb6, {
					cycles = (opor<uint8_t, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0xb7, {
					cycles = opor(AF.hi, AF.hi);
					});
				HANDLE_OP(0xb8, {
					cycles = cp(AF.hi, BC.hi);
					});
				HANDLE_OP(0xb9, {
					cycles = cp(AF.hi, BC.lo);
					});
				HANDLE_OP(0xba, {
					cycles = cp(AF.hi, DE.hi);
					});
				HANDLE_OP(0xbb, {
					cycles = cp(AF.hi, DE.lo);
					});
				HANDLE_OP(0xbc, {
					cycles = cp(AF.hi, HL.hi);
					});
				HANDLE_OP(0xbd, {
					cycles = cp(AF.hi, HL.lo);
					});
				HANDLE_OP(0xbe, {
					cycles = (cp<uint8_t, true>(AF.hi, HL.b16));
					});
				HANDLE_OP(0xbf, {
					cycles = cp(AF.hi, AF.hi);
					});

				HANDLE_OP(0xc0, {
					cycles = flags.z() ? ret<false>() : ret();
					});
				HANDLE_OP(0xc1, {
					cycles = pop(BC.b16);
					});
				HANDLE_OP(0xc2, {
					cycles = flags.z() ? jp<false>() : jp();
					});
				HANDLE_OP(0xc3, {
					cycles = jp();
					});
				HANDLE_OP(0xc4, {
					cycles = flags.z() ? call<false>() : call();
					});
				HANDLE_OP(0xc5, {
					cycles = push(BC.b16);
					});
				HANDLE_OP(0xc6, {
					cycles = (add<uint8_t, false, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xc7, {
					cycles = rst<0x00>();
					});
				HANDLE_OP(0xc8, {
					cycles = flags.z() ? ret() : ret<false>();
					});
				HANDLE_OP(0xc9, {
					cycles = (ret<true, false>());
					});
				HANDLE_OP(0xca, {
					cycles = flags.z() ? jp() : jp<false>();
					});
				HANDLE_OP(0xcc, {
					cycles = flags.z() ? call() : call<false>();
					});
				HANDLE_OP(0xcd, {
					cycles = call();
					});
				HANDLE_OP(0xce, {
					cycles = (add<uint8_t, true, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xcf, {
					cycles = rst<0x08>();
					});

				HANDLE_OP(0xd0, {
					cycles = flags.c() ? ret<false>() : ret();
					});
				HANDLE_OP(0xd1, {
					cycles = pop(DE.b16);
					});
				HANDLE_OP(0xd2, {
					cycles = flags.c() ? jp<false>() : jp();
					});
				HANDLE_OP(0xd3, {
					RDR_LOG_ERROR("Invalid opcode 0xD3");
					});
				HANDLE_OP(0xd4, {
					cycles = flags.c() ? call<false>() : call();
					});
				HANDLE_OP(0xd5, {
					cycles = push(DE.b16);
					});
				HANDLE_OP(0xd6, {
					cycles = (sub<uint8_t, false, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xd7, {
					cycles = rst<0x10>();
					});
				HANDLE_OP(0xd8, {
					cycles = flags.c() ? ret() : ret<false>();
					});
				HANDLE_OP(0xd9, {
					cycles = reti();
					});
				HANDLE_OP(0xda, {
					cycles = flags.c() ? jp() : jp<false>();
					});
				HANDLE_OP(0xdb, {
					RDR_LOG_ERROR("Invalid opcode 0xDB");
					});
				HANDLE_OP(0xdc, {
					cycles = flags.c() ? call() : call<false>();
					});
				HANDLE_OP(0xdd, {
					RDR_LOG_ERROR("Invalid opcode 0xDD");
					});
				HANDLE_OP(0xde, {
					cycles = (sub<uint8_t, true, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xdf, {
					cycles = rst<0x18>();
					});

				HANDLE_OP(0xe0, {
					uint16_t addr = 0xff00 + memory[pc++];
					cycles = (str(addr, AF.hi)) + 4;
					});
				HANDLE_OP(0xe1, {
					cycles = pop(HL.b16);
					});
				HANDLE_OP(0xe2, {
					uint16_t addr = 0xff00 + BC.lo;
					cycles = str(addr, AF.hi);
					});
				HANDLE_OP(0xe3, {
					RDR_LOG_ERROR("Invalid opcode 0xE3");
					});
				HANDLE_OP(0xe4, {
					RDR_LOG_ERROR("Invalid opcode 0xE4");
					});
				HANDLE_OP(0xe5, {
					cycles = push(HL.b16);
					});
				HANDLE_OP(0xe6, {
					cycles = (opand<uint8_t, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xe7, {
					cycles = rst<0x20>();
					});
				HANDLE_OP(0xe8, {
					cycles = addsp();
					});
				HANDLE_OP(0xe9, {
					cycles = jpreg(HL.b16);
					});
				HANDLE_OP(0xea, {
					uint16_t addr = memory.As<uint16_t>(pc);
					cycles = (str(addr, AF.hi)) + 8;
					pc += 2;
					});
				HANDLE_OP(0xeb, {
					RDR_LOG_ERROR("Invalid opcode 0xEB");
					});
				HANDLE_OP(0xec, {
					RDR_LOG_ERROR("Invalid opcode 0xEC");
					});
				HANDLE_OP(0xed, {
					RDR_LOG_ERROR("Invalid opcode 0xED");
					});
				HANDLE_OP(0xee, {
					cycles = (opxor<uint8_t, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xef, {
					cycles = rst<0x28>();
					});

				HANDLE_OP(0xf0, {
					uint16_t addr = 0xff00 + memory[pc++];
					cycles = (ldr(AF.hi, addr)) + 4;
					});
				HANDLE_OP(0xf1, {
					cycles = pop(AF.b16);
					AF.lo &= 0xf0;
					});
				HANDLE_OP(0xf2, {
					uint16_t addr = 0xff00 + BC.lo;
					cycles = ldr(AF.hi, addr);
					});
				HANDLE_OP(0xf3, {
					cycles = di();
					});
				HANDLE_OP(0xf4, {
					RDR_LOG_ERROR("Invalid opcode 0xF4");
					});
				HANDLE_OP(0xf5, {
					cycles = push(AF.b16);
					});
				HANDLE_OP(0xf6, {
					cycles = (opor<uint8_t, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xf7, {
					cycles = rst<0x30>();
					});
				HANDLE_OP(0xf8, {
					cycles = ldsp(HL.b16);
					});
				HANDLE_OP(0xf9, {
					cycles = mv(sp, HL.b16);
					});
				HANDLE_OP(0xfa, {
					uint16_t addr = memory.As<uint16_t>(pc);
					cycles = (ldr(AF.hi, addr)) + 8;
					pc += 2;
					});
				HANDLE_OP(0xfb, {
					cycles = ei();
					});
				HANDLE_OP(0xfc, {
					RDR_LOG_ERROR("Invalid opcode 0xFC");
					});
				HANDLE_OP(0xfd, {
					RDR_LOG_ERROR("Invalid opcode 0xFD");
					});
				HANDLE_OP(0xfe, {
					cycles = (cp<uint8_t, true>(AF.hi, pc++));
					});
				HANDLE_OP(0xff, {
					cycles = rst<0x38>();
					});
		}

		return cycles;
	}

	uint32_t CPU::prefix()
	{
		uint8_t opcode = memory[pc++];
		uint32_t cycles = 0;

		switch (opcode)
		{
			HANDLE_OP(0x00, {
				cycles = rlc(BC.hi);
				});
			HANDLE_OP(0x01, {
				cycles = rlc(BC.lo);
				});
			HANDLE_OP(0x02, {
				cycles = rlc(DE.hi);
				});
			HANDLE_OP(0x03, {
				cycles = rlc(DE.lo);
				});
			HANDLE_OP(0x04, {
				cycles = rlc(HL.hi);
				});
			HANDLE_OP(0x05, {
				cycles = rlc(HL.lo);
				});
			HANDLE_OP(0x06, {
				cycles = (rlc<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x07, {
				cycles = rlc(AF.hi);
				});
			HANDLE_OP(0x08, {
				cycles = rrc(BC.hi);
				});
			HANDLE_OP(0x09, {
				cycles = rrc(BC.lo);
				});
			HANDLE_OP(0x0a, {
				cycles = rrc(DE.hi);
				});
			HANDLE_OP(0x0b, {
				cycles = rrc(DE.lo);
				});
			HANDLE_OP(0x0c, {
				cycles = rrc(HL.hi);
				});
			HANDLE_OP(0x0d, {
				cycles = rrc(HL.lo);
				});
			HANDLE_OP(0x0e, {
				cycles = (rrc<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x0f, {
				cycles = rrc(AF.hi);
				});

			HANDLE_OP(0x10, {
				cycles = rl(BC.hi);
				});
			HANDLE_OP(0x11, {
				cycles = rl(BC.lo);
				});
			HANDLE_OP(0x12, {
				cycles = rl(DE.hi);
				});
			HANDLE_OP(0x13, {
				cycles = rl(DE.lo);
				});
			HANDLE_OP(0x14, {
				cycles = rl(HL.hi);
				});
			HANDLE_OP(0x15, {
				cycles = rl(HL.lo);
				});
			HANDLE_OP(0x16, {
				cycles = (rl<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x17, {
				cycles = rl(AF.hi);
				});
			HANDLE_OP(0x18, {
				cycles = rr(BC.hi);
				});
			HANDLE_OP(0x19, {
				cycles = rr(BC.lo);
				});
			HANDLE_OP(0x1a, {
				cycles = rr(DE.hi);
				});
			HANDLE_OP(0x1b, {
				cycles = rr(DE.lo);
				});
			HANDLE_OP(0x1c, {
				cycles = rr(HL.hi);
				});
			HANDLE_OP(0x1d, {
				cycles = rr(HL.lo);
				});
			HANDLE_OP(0x1e, {
				cycles = (rr<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x1f, {
				cycles = rr(AF.hi);
				});

			HANDLE_OP(0x20, {
				cycles = sla(BC.hi);
				});
			HANDLE_OP(0x21, {
				cycles = sla(BC.lo);
				});
			HANDLE_OP(0x22, {
				cycles = sla(DE.hi);
				});
			HANDLE_OP(0x23, {
				cycles = sla(DE.lo);
				});
			HANDLE_OP(0x24, {
				cycles = sla(HL.hi);
				});
			HANDLE_OP(0x25, {
				cycles = sla(HL.lo);
				});
			HANDLE_OP(0x26, {
				cycles = (sla<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x27, {
				cycles = sla(AF.hi);
				});
			HANDLE_OP(0x28, {
				cycles = sra(BC.hi);
				});
			HANDLE_OP(0x29, {
				cycles = sra(BC.lo);
				});
			HANDLE_OP(0x2a, {
				cycles = sra(DE.hi);
				});
			HANDLE_OP(0x2b, {
				cycles = sra(DE.lo);
				});
			HANDLE_OP(0x2c, {
				cycles = sra(HL.hi);
				});
			HANDLE_OP(0x2d, {
				cycles = sra(HL.lo);
				});
			HANDLE_OP(0x2e, {
				cycles = (sra<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x2f, {
				cycles = sra(AF.hi);
				});

			HANDLE_OP(0x30, {
				cycles = swap(BC.hi);
				});
			HANDLE_OP(0x31, {
				cycles = swap(BC.lo);
				});
			HANDLE_OP(0x32, {
				cycles = swap(DE.hi);
				});
			HANDLE_OP(0x33, {
				cycles = swap(DE.lo);
				});
			HANDLE_OP(0x34, {
				cycles = swap(HL.hi);
				});
			HANDLE_OP(0x35, {
				cycles = swap(HL.lo);
				});
			HANDLE_OP(0x36, {
				cycles = (swap<uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x37, {
				cycles = swap(AF.hi);
				});
			HANDLE_OP(0x38, {
				cycles = srl(BC.hi);
				});
			HANDLE_OP(0x39, {
				cycles = srl(BC.lo);
				});
			HANDLE_OP(0x3a, {
				cycles = srl(DE.hi);
				});
			HANDLE_OP(0x3b, {
				cycles = srl(DE.lo);
				});
			HANDLE_OP(0x3c, {
				cycles = srl(HL.hi);
				});
			HANDLE_OP(0x3d, {
				cycles = srl(HL.lo);
				});
			HANDLE_OP(0x3e, {
				cycles = (srl<uint16_t, false, true>(HL.b16));
				});
			HANDLE_OP(0x3f, {
				cycles = srl(AF.hi);
				});

			HANDLE_OP(0x40, {
				cycles = testbit<0>(BC.hi);
				});
			HANDLE_OP(0x41, {
				cycles = testbit<0>(BC.lo);
				});
			HANDLE_OP(0x42, {
				cycles = testbit<0>(DE.hi);
				});
			HANDLE_OP(0x43, {
				cycles = testbit<0>(DE.lo);
				});
			HANDLE_OP(0x44, {
				cycles = testbit<0>(HL.hi);
				});
			HANDLE_OP(0x45, {
				cycles = testbit<0>(HL.lo);
				});
			HANDLE_OP(0x46, {
				cycles = (testbit<0, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x47, {
				cycles = testbit<0>(AF.hi);
				});
			HANDLE_OP(0x48, {
				cycles = testbit<1>(BC.hi);
				});
			HANDLE_OP(0x49, {
				cycles = testbit<1>(BC.lo);
				});
			HANDLE_OP(0x4a, {
				cycles = testbit<1>(DE.hi);
				});
			HANDLE_OP(0x4b, {
				cycles = testbit<1>(DE.lo);
				});
			HANDLE_OP(0x4c, {
				cycles = testbit<1>(HL.hi);
				});
			HANDLE_OP(0x4d, {
				cycles = testbit<1>(HL.lo);
				});
			HANDLE_OP(0x4e, {
				cycles = (testbit<1, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x4f, {
				cycles = testbit<1>(AF.hi);
				});

			HANDLE_OP(0x50, {
				cycles = testbit<2>(BC.hi);
				});
			HANDLE_OP(0x51, {
				cycles = testbit<2>(BC.lo);
				});
			HANDLE_OP(0x52, {
				cycles = testbit<2>(DE.hi);
				});
			HANDLE_OP(0x53, {
				cycles = testbit<2>(DE.lo);
				});
			HANDLE_OP(0x54, {
				cycles = testbit<2>(HL.hi);
				});
			HANDLE_OP(0x55, {
				cycles = testbit<2>(HL.lo);
				});
			HANDLE_OP(0x56, {
				cycles = (testbit<2, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x57, {
				cycles = testbit<2>(AF.hi);
				});
			HANDLE_OP(0x58, {
				cycles = testbit<3>(BC.hi);
				});
			HANDLE_OP(0x59, {
				cycles = testbit<3>(BC.lo);
				});
			HANDLE_OP(0x5a, {
				cycles = testbit<3>(DE.hi);
				});
			HANDLE_OP(0x5b, {
				cycles = testbit<3>(DE.lo);
				});
			HANDLE_OP(0x5c, {
				cycles = testbit<3>(HL.hi);
				});
			HANDLE_OP(0x5d, {
				cycles = testbit<3>(HL.lo);
				});
			HANDLE_OP(0x5e, {
				cycles = (testbit<3, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x5f, {
				cycles = testbit<3>(AF.hi);
				});

			HANDLE_OP(0x60, {
				cycles = testbit<4>(BC.hi);
				});
			HANDLE_OP(0x61, {
				cycles = testbit<4>(BC.lo);
				});
			HANDLE_OP(0x62, {
				cycles = testbit<4>(DE.hi);
				});
			HANDLE_OP(0x63, {
				cycles = testbit<4>(DE.lo);
				});
			HANDLE_OP(0x64, {
				cycles = testbit<4>(HL.hi);
				});
			HANDLE_OP(0x65, {
				cycles = testbit<4>(HL.lo);
				});
			HANDLE_OP(0x66, {
				cycles = (testbit<4, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x67, {
				cycles = testbit<4>(AF.hi);
				});
			HANDLE_OP(0x68, {
				cycles = testbit<5>(BC.hi);
				});
			HANDLE_OP(0x69, {
				cycles = testbit<5>(BC.lo);
				});
			HANDLE_OP(0x6a, {
				cycles = testbit<5>(DE.hi);
				});
			HANDLE_OP(0x6b, {
				cycles = testbit<5>(DE.lo);
				});
			HANDLE_OP(0x6c, {
				cycles = testbit<5>(HL.hi);
				});
			HANDLE_OP(0x6d, {
				cycles = testbit<5>(HL.lo);
				});
			HANDLE_OP(0x6e, {
				cycles = (testbit<5, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x6f, {
				cycles = testbit<5>(AF.hi);
				});

			HANDLE_OP(0x70, {
				cycles = testbit<6>(BC.hi);
				});
			HANDLE_OP(0x71, {
				cycles = testbit<6>(BC.lo);
				});
			HANDLE_OP(0x72, {
				cycles = testbit<6>(DE.hi);
				});
			HANDLE_OP(0x73, {
				cycles = testbit<6>(DE.lo);
				});
			HANDLE_OP(0x74, {
				cycles = testbit<6>(HL.hi);
				});
			HANDLE_OP(0x75, {
				cycles = testbit<6>(HL.lo);
				});
			HANDLE_OP(0x76, {
				cycles = (testbit<6, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x77, {
				cycles = testbit<6>(AF.hi);
				});
			HANDLE_OP(0x78, {
				cycles = testbit<7>(BC.hi);
				});
			HANDLE_OP(0x79, {
				cycles = testbit<7>(BC.lo);
				});
			HANDLE_OP(0x7a, {
				cycles = testbit<7>(DE.hi);
				});
			HANDLE_OP(0x7b, {
				cycles = testbit<7>(DE.lo);
				});
			HANDLE_OP(0x7c, {
				cycles = testbit<7>(HL.hi);
				});
			HANDLE_OP(0x7d, {
				cycles = testbit<7>(HL.lo);
				});
			HANDLE_OP(0x7e, {
				cycles = (testbit<7, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x7f, {
				cycles = testbit<7>(AF.hi);
				});

			HANDLE_OP(0x80, {
				cycles = resetbit<0>(BC.hi);
				});
			HANDLE_OP(0x81, {
				cycles = resetbit<0>(BC.lo);
				});
			HANDLE_OP(0x82, {
				cycles = resetbit<0>(DE.hi);
				});
			HANDLE_OP(0x83, {
				cycles = resetbit<0>(DE.lo);
				});
			HANDLE_OP(0x84, {
				cycles = resetbit<0>(HL.hi);
				});
			HANDLE_OP(0x85, {
				cycles = resetbit<0>(HL.lo);
				});
			HANDLE_OP(0x86, {
				cycles = (resetbit<0, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x87, {
				cycles = resetbit<0>(AF.hi);
				});
			HANDLE_OP(0x88, {
				cycles = resetbit<1>(BC.hi);
				});
			HANDLE_OP(0x89, {
				cycles = resetbit<1>(BC.lo);
				});
			HANDLE_OP(0x8a, {
				cycles = resetbit<1>(DE.hi);
				});
			HANDLE_OP(0x8b, {
				cycles = resetbit<1>(DE.lo);
				});
			HANDLE_OP(0x8c, {
				cycles = resetbit<1>(HL.hi);
				});
			HANDLE_OP(0x8d, {
				cycles = resetbit<1>(HL.lo);
				});
			HANDLE_OP(0x8e, {
				cycles = (resetbit<1, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x8f, {
				cycles = resetbit<1>(AF.hi);
				});

			HANDLE_OP(0x90, {
				cycles = resetbit<2>(BC.hi);
				});
			HANDLE_OP(0x91, {
				cycles = resetbit<2>(BC.lo);
				});
			HANDLE_OP(0x92, {
				cycles = resetbit<2>(DE.hi);
				});
			HANDLE_OP(0x93, {
				cycles = resetbit<2>(DE.lo);
				});
			HANDLE_OP(0x94, {
				cycles = resetbit<2>(HL.hi);
				});
			HANDLE_OP(0x95, {
				cycles = resetbit<2>(HL.lo);
				});
			HANDLE_OP(0x96, {
				cycles = (resetbit<2, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x97, {
				cycles = resetbit<2>(AF.hi);
				});
			HANDLE_OP(0x98, {
				cycles = resetbit<3>(BC.hi);
				});
			HANDLE_OP(0x99, {
				cycles = resetbit<3>(BC.lo);
				});
			HANDLE_OP(0x9a, {
				cycles = resetbit<3>(DE.hi);
				});
			HANDLE_OP(0x9b, {
				cycles = resetbit<3>(DE.lo);
				});
			HANDLE_OP(0x9c, {
				cycles = resetbit<3>(HL.hi);
				});
			HANDLE_OP(0x9d, {
				cycles = resetbit<3>(HL.lo);
				});
			HANDLE_OP(0x9e, {
				cycles = (resetbit<3, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0x9f, {
				cycles = resetbit<3>(AF.hi);
				});

			HANDLE_OP(0xa0, {
				cycles = resetbit<4>(BC.hi);
				});
			HANDLE_OP(0xa1, {
				cycles = resetbit<4>(BC.lo);
				});
			HANDLE_OP(0xa2, {
				cycles = resetbit<4>(DE.hi);
				});
			HANDLE_OP(0xa3, {
				cycles = resetbit<4>(DE.lo);
				});
			HANDLE_OP(0xa4, {
				cycles = resetbit<4>(HL.hi);
				});
			HANDLE_OP(0xa5, {
				cycles = resetbit<4>(HL.lo);
				});
			HANDLE_OP(0xa6, {
				cycles = (resetbit<4, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xa7, {
				cycles = resetbit<4>(AF.hi);
				});
			HANDLE_OP(0xa8, {
				cycles = resetbit<5>(BC.hi);
				});
			HANDLE_OP(0xa9, {
				cycles = resetbit<5>(BC.lo);
				});
			HANDLE_OP(0xaa, {
				cycles = resetbit<5>(DE.hi);
				});
			HANDLE_OP(0xab, {
				cycles = resetbit<5>(DE.lo);
				});
			HANDLE_OP(0xac, {
				cycles = resetbit<5>(HL.hi);
				});
			HANDLE_OP(0xad, {
				cycles = resetbit<5>(HL.lo);
				});
			HANDLE_OP(0xae, {
				cycles = (resetbit<5, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xaf, {
				cycles = resetbit<5>(AF.hi);
				});

			HANDLE_OP(0xb0, {
				cycles = resetbit<6>(BC.hi);
				});
			HANDLE_OP(0xb1, {
				cycles = resetbit<6>(BC.lo);
				});
			HANDLE_OP(0xb2, {
				cycles = resetbit<6>(DE.hi);
				});
			HANDLE_OP(0xb3, {
				cycles = resetbit<6>(DE.lo);
				});
			HANDLE_OP(0xb4, {
				cycles = resetbit<6>(HL.hi);
				});
			HANDLE_OP(0xb5, {
				cycles = resetbit<6>(HL.lo);
				});
			HANDLE_OP(0xb6, {
				cycles = (resetbit<6, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xb7, {
				cycles = resetbit<6>(AF.hi);
				});
			HANDLE_OP(0xb8, {
				cycles = resetbit<7>(BC.hi);
				});
			HANDLE_OP(0xb9, {
				cycles = resetbit<7>(BC.lo);
				});
			HANDLE_OP(0xba, {
				cycles = resetbit<7>(DE.hi);
				});
			HANDLE_OP(0xbb, {
				cycles = resetbit<7>(DE.lo);
				});
			HANDLE_OP(0xbc, {
				cycles = resetbit<7>(HL.hi);
				});
			HANDLE_OP(0xbd, {
				cycles = resetbit<7>(HL.lo);
				});
			HANDLE_OP(0xbe, {
				cycles = (resetbit<7, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xbf, {
				cycles = resetbit<7>(AF.hi);
				});

			HANDLE_OP(0xc0, {
				cycles = setbit<0>(BC.hi);
				});
			HANDLE_OP(0xc1, {
				cycles = setbit<0>(BC.lo);
				});
			HANDLE_OP(0xc2, {
				cycles = setbit<0>(DE.hi);
				});
			HANDLE_OP(0xc3, {
				cycles = setbit<0>(DE.lo);
				});
			HANDLE_OP(0xc4, {
				cycles = setbit<0>(HL.hi);
				});
			HANDLE_OP(0xc5, {
				cycles = setbit<0>(HL.lo);
				});
			HANDLE_OP(0xc6, {
				cycles = (setbit<0, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xc7, {
				cycles = setbit<0>(AF.hi);
				});
			HANDLE_OP(0xc8, {
				cycles = setbit<1>(BC.hi);
				});
			HANDLE_OP(0xc9, {
				cycles = setbit<1>(BC.lo);
				});
			HANDLE_OP(0xca, {
				cycles = setbit<1>(DE.hi);
				});
			HANDLE_OP(0xcb, {
				cycles = setbit<1>(DE.lo);
				});
			HANDLE_OP(0xcc, {
				cycles = setbit<1>(HL.hi);
				});
			HANDLE_OP(0xcd, {
				cycles = setbit<1>(HL.lo);
				});
			HANDLE_OP(0xce, {
				cycles = (setbit<1, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xcf, {
				cycles = setbit<1>(AF.hi);
				});

			HANDLE_OP(0xd0, {
				cycles = setbit<2>(BC.hi);
				});
			HANDLE_OP(0xd1, {
				cycles = setbit<2>(BC.lo);
				});
			HANDLE_OP(0xd2, {
				cycles = setbit<2>(DE.hi);
				});
			HANDLE_OP(0xd3, {
				cycles = setbit<2>(DE.lo);
				});
			HANDLE_OP(0xd4, {
				cycles = setbit<2>(HL.hi);
				});
			HANDLE_OP(0xd5, {
				cycles = setbit<2>(HL.lo);
				});
			HANDLE_OP(0xd6, {
				cycles = (setbit<2, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xd7, {
				cycles = setbit<2>(AF.hi);
				});
			HANDLE_OP(0xd8, {
				cycles = setbit<3>(BC.hi);
				});
			HANDLE_OP(0xd9, {
				cycles = setbit<3>(BC.lo);
				});
			HANDLE_OP(0xda, {
				cycles = setbit<3>(DE.hi);
				});
			HANDLE_OP(0xdb, {
				cycles = setbit<3>(DE.lo);
				});
			HANDLE_OP(0xdc, {
				cycles = setbit<3>(HL.hi);
				});
			HANDLE_OP(0xdd, {
				cycles = setbit<3>(HL.lo);
				});
			HANDLE_OP(0xde, {
				cycles = (setbit<3, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xdf, {
				cycles = setbit<3>(AF.hi);
				});

			HANDLE_OP(0xe0, {
				cycles = setbit<4>(BC.hi);
				});
			HANDLE_OP(0xe1, {
				cycles = setbit<4>(BC.lo);
				});
			HANDLE_OP(0xe2, {
				cycles = setbit<4>(DE.hi);
				});
			HANDLE_OP(0xe3, {
				cycles = setbit<4>(DE.lo);
				});
			HANDLE_OP(0xe4, {
				cycles = setbit<4>(HL.hi);
				});
			HANDLE_OP(0xe5, {
				cycles = setbit<4>(HL.lo);
				});
			HANDLE_OP(0xe6, {
				cycles = (setbit<4, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xe7, {
				cycles = setbit<4>(AF.hi);
				});
			HANDLE_OP(0xe8, {
				cycles = setbit<5>(BC.hi);
				});
			HANDLE_OP(0xe9, {
				cycles = setbit<5>(BC.lo);
				});
			HANDLE_OP(0xea, {
				cycles = setbit<5>(DE.hi);
				});
			HANDLE_OP(0xeb, {
				cycles = setbit<5>(DE.lo);
				});
			HANDLE_OP(0xec, {
				cycles = setbit<5>(HL.hi);
				});
			HANDLE_OP(0xed, {
				cycles = setbit<5>(HL.lo);
				});
			HANDLE_OP(0xee, {
				cycles = (setbit<5, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xef, {
				cycles = setbit<5>(AF.hi);
				});

			HANDLE_OP(0xf0, {
				cycles = setbit<6>(BC.hi);
				});
			HANDLE_OP(0xf1, {
				cycles = setbit<6>(BC.lo);
				});
			HANDLE_OP(0xf2, {
				cycles = setbit<6>(DE.hi);
				});
			HANDLE_OP(0xf3, {
				cycles = setbit<6>(DE.lo);
				});
			HANDLE_OP(0xf4, {
				cycles = setbit<6>(HL.hi);
				});
			HANDLE_OP(0xf5, {
				cycles = setbit<6>(HL.lo);
				});
			HANDLE_OP(0xf6, {
				cycles = (setbit<6, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xf7, {
				cycles = setbit<6>(AF.hi);
				});
			HANDLE_OP(0xf8, {
				cycles = setbit<7>(BC.hi);
				});
			HANDLE_OP(0xf9, {
				cycles = setbit<7>(BC.lo);
				});
			HANDLE_OP(0xfa, {
				cycles = setbit<7>(DE.hi);
				});
			HANDLE_OP(0xfb, {
				cycles = setbit<7>(DE.lo);
				});
			HANDLE_OP(0xfc, {
				cycles = setbit<7>(HL.hi);
				});
			HANDLE_OP(0xfd, {
				cycles = setbit<7>(HL.lo);
				});
			HANDLE_OP(0xfe, {
				cycles = (setbit<7, uint16_t, true>(HL.b16));
				});
			HANDLE_OP(0xff, {
				cycles = setbit<7>(AF.hi);
				});
		}

		return cycles;
	}
}