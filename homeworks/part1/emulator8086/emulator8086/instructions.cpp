#include "instructions.h"

#include <bit>

namespace emu8086 {

#include "scripts/instr_table.inl"

Instruction get_instruction(uint8_t opcode) {
	return instructions[opcode];
}

Instruction get_special_instruction(const Instruction& ins, uint8_t second_byte) {
	return special_instructions[ins.special_instr_idx][(second_byte & SB_REG_MASK)>>3];
}

}