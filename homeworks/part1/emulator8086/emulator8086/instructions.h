#pragma once

#include "emu8086.h"

#include <string>

namespace emu8086 {

// First byte(FB)
constexpr uint8_t FB_REG_MASK = 0x07;
constexpr uint8_t SR_MASK = 0x38; // Segment register
constexpr uint8_t IMM_W_MASK = 0x08;
constexpr uint8_t W_MASK = 0x01;
constexpr uint8_t Z_MASK = 0x01;
constexpr uint8_t D_MASK = 0x02;
constexpr uint8_t S_MASK = 0x02;
constexpr uint8_t V_MASK = 0x02;

// Second byte(SB)
constexpr uint8_t MOD_MASK = 0xC0;
constexpr uint8_t SB_REG_MASK = 0x38;
constexpr uint8_t REGMEM_MASK = 0x07;

enum class InstructionOpcode {
	Unknown,

	mov, push, pop, xchg, in, out, xlat, lea,
	lds, les, lahf, sahf, pushf, popf, add, inc,
	aaa, daa, sub, sbb, dec, cmp, aas, das,
	aad, cbw, and_, test, or_, xor_, repne, rep,
	movs, cmps, scas, lods, stds, call, jmp, ret,
	je, jl, jle, jb, jbe, jp, jo, js,
	jne, jnl, jnle, jnbe, jnb, jnp, jno, jns,
	loop, loopz, loopnz, jcxz, int_, int3, into, iret,
	clc, cmc, stc, cld, std, cli, sti, hlt,
	wait, esc, lock, segment, cwd, aam, adc,
};

enum class InstructionType : int {
	Unknown = 0,

	RegMem_Reg,
	Imm_RegMem,
	Imm_Reg,
	Imm_Acc,
	Mem_Acc,
	Acc_Mem,
	Reg_Acc,
	RegMem_SR,
	SR_RegMem,
	SPImm_Seg,
	Seg,
	SPImm_InterSeg,
	InterSeg,
	Imm,
	RegMem,
	Reg,
	SR,
	FixedPort,
	VariablePort,
	Jmp,
	DirSeg,
	IndirSeg,
	DirInterSeg,
	IndirInterSeg,
	DirSegShort,
	Esc,
	SegmentPrefix,
	SingleByte,
	SkipSecond,
	RegMem_1,
	RegMem_CL,

	Special,
};

struct Instruction {
	std::string name;
	InstructionType type;
	InstructionOpcode opcode;
	int special_instr_idx;
};

Instruction get_instruction(uint8_t opcode);
Instruction get_special_instruction(const Instruction& ins, uint8_t snd_byte);

}