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
	RegMem,
	Reg,
	SR,
	FixedPort,
	VariablePort,
	Jmp,

	SingleByte,

	Special,
	Special_Imm_RegMem,
	Special_RegMem_1,
	Special_RegMem_CL,
	Special_RegMem,
	Special_Mem,
};

struct Instruction {
	std::string name;
	InstructionType type;
	int special_instr_idx;
};

void register_instructions();
Instruction get_instruction(uint8_t opcode);
Instruction get_special_instruction(const Instruction& ins, uint8_t snd_byte);

}